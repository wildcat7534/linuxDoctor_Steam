export const FUTURE_LAB_SCHEMA_NAME = 'linux-doctor.future-lab.live';
export const FUTURE_LAB_SCHEMA_VERSION = 1;

export function finiteNumber(value, minimum = 0) {
  if (value === null || value === undefined || value === '') return null;
  const number = Number(value);
  return Number.isFinite(number) && number >= minimum ? number : null;
}

function clamp(value, minimum, maximum) {
  return Math.min(maximum, Math.max(minimum, value));
}

function monotonicRate(current, previous, seconds) {
  if (!Number.isFinite(current) || !Number.isFinite(previous) || current < previous || seconds <= 0) return null;
  return (current - previous) / seconds;
}

export function memberRate(currentItems, previousItems, counterName, seconds, identity) {
  if (!Array.isArray(currentItems) || !Array.isArray(previousItems) ||
      currentItems.length === 0 || currentItems.length !== previousItems.length) return null;
  const previousByIdentity = new Map();
  for (const item of previousItems) {
    const key = identity(item);
    if (!key || previousByIdentity.has(key)) return null;
    previousByIdentity.set(key, item);
  }
  let total = 0;
  for (const item of currentItems) {
    const key = identity(item);
    const previous = key ? previousByIdentity.get(key) : null;
    if (!previous) return null;
    const rate = monotonicRate(
      finiteNumber(item?.counters?.[counterName]),
      finiteNumber(previous?.counters?.[counterName]),
      seconds
    );
    if (!Number.isFinite(rate)) return null;
    total += rate;
    previousByIdentity.delete(key);
  }
  return previousByIdentity.size === 0 ? total : null;
}

function networkIdentity(item) {
  const name = String(item?.name || '').trim();
  return name || null;
}

function diskIdentity(item) {
  const name = String(item?.name || '').trim();
  const major = finiteNumber(item?.major);
  const minor = finiteNumber(item?.minor);
  return name && major !== null && minor !== null ? `${major}:${minor}:${name}` : null;
}

function percentage(used, total) {
  if (!Number.isFinite(used) || !Number.isFinite(total) || total <= 0) return null;
  return clamp(used * 100 / total, 0, 100);
}

export function liveSnapshotIsCurrent(snapshot, now, staleAfterMs = 4000) {
  if (!snapshot || snapshot.source !== 'live' ||
      snapshot.schemaName !== FUTURE_LAB_SCHEMA_NAME ||
      snapshot.schemaVersion !== FUTURE_LAB_SCHEMA_VERSION ||
      !/^[0-9a-f]{8}(?:-[0-9a-f]{4}){3}-[0-9a-f]{12}$/i.test(snapshot.bootId || '') ||
      snapshot.timestampTrusted !== true || !Number.isFinite(snapshot.wallTime) ||
      !Number.isFinite(now)) return false;
  const age = now - snapshot.wallTime;
  return age <= staleAfterMs && age >= -10000;
}

export function createTelemetrySample(snapshot, previous) {
  const sameLiveIdentity = previous && previous.source === 'live' && snapshot.source === 'live' &&
    previous.schemaName === FUTURE_LAB_SCHEMA_NAME && snapshot.schemaName === FUTURE_LAB_SCHEMA_NAME &&
    previous.schemaVersion === FUTURE_LAB_SCHEMA_VERSION && snapshot.schemaVersion === FUTURE_LAB_SCHEMA_VERSION &&
    previous.bootId && previous.bootId === snapshot.bootId;
  const elapsed = sameLiveIdentity ? (snapshot.time - previous.time) / 1000 : null;
  const comparable = Number.isFinite(elapsed) && elapsed >= .2 && elapsed <= 15;
  const comparableCpu = comparable && previous.cpu?.logicalCount === snapshot.cpu?.logicalCount;
  const cpuBusyRate = comparableCpu
    ? monotonicRate(snapshot.cpu?.busy, previous.cpu?.busy, elapsed)
    : null;
  const cpuTotalRate = comparableCpu
    ? monotonicRate(snapshot.cpu?.total, previous.cpu?.total, elapsed)
    : null;
  const cpuRate = Number.isFinite(cpuBusyRate) && Number.isFinite(cpuTotalRate) && cpuTotalRate > 0
    ? clamp(cpuBusyRate * 100 / cpuTotalRate, 0, 100)
    : null;
  const logicalCount = snapshot.cpu?.logicalCount;
  const loadOne = snapshot.load?.one;
  const loadPercent = Number.isFinite(loadOne) && Number.isFinite(logicalCount) && logicalCount > 0
    ? Math.max(0, loadOne * 100 / logicalCount)
    : null;
  const memory = snapshot.memory;
  const usedKib = Number.isFinite(memory?.usedKib)
    ? memory.usedKib
    : Number.isFinite(memory?.totalKib) && Number.isFinite(memory?.availableKib)
      ? Math.max(0, memory.totalKib - memory.availableKib)
      : null;
  const memoryPercent = percentage(usedKib, memory?.totalKib);
  const swapUsedKib = Number.isFinite(memory?.swapTotalKib) && Number.isFinite(memory?.swapFreeKib)
    ? Math.max(0, memory.swapTotalKib - memory.swapFreeKib)
    : null;
  const swapPercent = memory?.swapTotalKib === 0 ? 0 : percentage(swapUsedKib, memory?.swapTotalKib);

  return {
    time: snapshot.time,
    elapsed,
    comparable,
    cpuRate,
    loadOne,
    loadPercent,
    memoryPercent,
    swapPercent,
    networkRxRate: comparable && snapshot.network && previous.network &&
        !snapshot.network.truncated && !previous.network.truncated
      ? memberRate(snapshot.network.interfaces, previous.network.interfaces,
          'received_bytes', elapsed, networkIdentity)
      : null,
    networkTxRate: comparable && snapshot.network && previous.network &&
        !snapshot.network.truncated && !previous.network.truncated
      ? memberRate(snapshot.network.interfaces, previous.network.interfaces,
          'transmitted_bytes', elapsed, networkIdentity)
      : null,
    diskReadRate: comparable && snapshot.disks && previous.disks &&
        !snapshot.disks.truncated && !previous.disks.truncated
      ? memberRate(snapshot.disks.physicalDevices, previous.disks.physicalDevices,
          'reads_completed', elapsed, diskIdentity)
      : null,
    diskWriteRate: comparable && snapshot.disks && previous.disks &&
        !snapshot.disks.truncated && !previous.disks.truncated
      ? memberRate(snapshot.disks.physicalDevices, previous.disks.physicalDevices,
          'writes_completed', elapsed, diskIdentity)
      : null
  };
}
