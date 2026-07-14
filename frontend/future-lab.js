import {
  createTelemetrySample,
  finiteNumber,
  liveSnapshotIsCurrent
} from './future-lab-core.js?v=1.1.1';

(() => {
  'use strict';

  const POLL_INTERVAL_MS = 1000;
  const REPORT_RETRY_MS = 5000;
  const STALE_AFTER_MS = 4000;
  const MAX_HISTORY_POINTS = 120;
  const LIVE_URL = 'future-lab-live.json';
  const REPORT_URL = 'report.json';
  const REDUCED_MOTION = window.matchMedia('(prefers-reduced-motion: reduce)');
  const INTEGER = new Intl.NumberFormat('fr-FR', { maximumFractionDigits: 0 });
  const DECIMAL = new Intl.NumberFormat('fr-FR', { minimumFractionDigits: 1, maximumFractionDigits: 1 });
  const LOAD = new Intl.NumberFormat('fr-FR', { minimumFractionDigits: 2, maximumFractionDigits: 2 });

  const elements = {
    streamState: document.querySelector('#stream-state'),
    streamStateLabel: document.querySelector('#stream-state-label'),
    sourceBanner: document.querySelector('#source-banner'),
    sourceTitle: document.querySelector('#source-title'),
    sourceDescription: document.querySelector('#source-description'),
    sourceValue: document.querySelector('#source-value'),
    freshnessValue: document.querySelector('#freshness-value'),
    windowValue: document.querySelector('#window-value'),
    sampleCount: document.querySelector('#sample-count'),
    historyLimit: document.querySelector('#history-limit'),
    pauseStream: document.querySelector('#pause-stream'),
    clearTimeline: document.querySelector('#clear-timeline'),
    telemetrySummary: document.querySelector('#telemetry-summary'),
    screenReaderSummary: document.querySelector('#screen-reader-summary'),
    cpuValue: document.querySelector('#cpu-value'),
    cpuDetail: document.querySelector('#cpu-detail'),
    cpuCount: document.querySelector('#cpu-count'),
    cpuQuality: document.querySelector('#cpu-quality'),
    loadValue: document.querySelector('#load-value'),
    loadDetail: document.querySelector('#load-detail'),
    taskCount: document.querySelector('#task-count'),
    loadQuality: document.querySelector('#load-quality'),
    memoryValue: document.querySelector('#memory-value'),
    memoryDetail: document.querySelector('#memory-detail'),
    swapValue: document.querySelector('#swap-value'),
    swapDetail: document.querySelector('#swap-detail'),
    memoryFill: document.querySelector('#memory-fill'),
    memoryQuality: document.querySelector('#memory-quality'),
    gpuName: document.querySelector('#gpu-name'),
    gpuValue: document.querySelector('#gpu-value'),
    gpuMemoryValue: document.querySelector('#gpu-memory-value'),
    gpuThermal: document.querySelector('#gpu-thermal'),
    gpuQuality: document.querySelector('#gpu-quality'),
    nvtopState: document.querySelector('#nvtop-state'),
    networkRx: document.querySelector('#network-rx'),
    networkTx: document.querySelector('#network-tx'),
    interfaceCount: document.querySelector('#interface-count'),
    networkQuality: document.querySelector('#network-quality'),
    diskRead: document.querySelector('#disk-read'),
    diskWrite: document.querySelector('#disk-write'),
    diskCount: document.querySelector('#disk-count'),
    diskQuality: document.querySelector('#disk-quality'),
    interfacesDetails: document.querySelector('#interfaces-details'),
    interfacesSummary: document.querySelector('#interfaces-summary'),
    interfacesList: document.querySelector('#interfaces-list'),
    disksDetails: document.querySelector('#disks-details'),
    disksSummary: document.querySelector('#disks-summary'),
    disksList: document.querySelector('#disks-list')
  };

  const charts = {
    cpu: {
      canvas: document.querySelector('#cpu-chart'),
      series: [{ key: 'cpuRate', color: '#55f6ff', label: 'activité CPU' }],
      fixedMax: 100,
      suffix: '%'
    },
    load: {
      canvas: document.querySelector('#load-chart'),
      series: [{ key: 'loadPercent', color: '#927cff', label: 'charge par CPU' }],
      minimumMax: 100,
      suffix: '%'
    },
    memory: {
      canvas: document.querySelector('#memory-chart'),
      series: [
        { key: 'memoryPercent', color: '#65f5af', label: 'RAM utilisée' },
        { key: 'swapPercent', color: '#ff68d4', label: 'swap utilisé' }
      ],
      fixedMax: 100,
      suffix: '%'
    },
    gpu: {
      canvas: document.querySelector('#gpu-chart'),
      series: [
        { key: 'gpuPercent', color: '#ff68d4', label: 'activité GPU' },
        { key: 'gpuMemoryPercent', color: '#927cff', label: 'VRAM utilisée' }
      ],
      fixedMax: 100,
      suffix: '%'
    },
    network: {
      canvas: document.querySelector('#network-chart'),
      series: [
        { key: 'networkRxRate', color: '#42a5ff', label: 'réception' },
        { key: 'networkTxRate', color: '#ff68d4', label: 'émission' }
      ],
      valueFormatter: value => formatBytes(value),
      suffix: '/s'
    },
    disk: {
      canvas: document.querySelector('#disk-chart'),
      series: [
        { key: 'diskReadRate', color: '#ffb45e', label: 'lectures' },
        { key: 'diskWriteRate', color: '#ff68d4', label: 'écritures' }
      ],
      valueFormatter: value => DECIMAL.format(value),
      suffix: ' op/s'
    }
  };

  const state = {
    paused: false,
    inFlight: false,
    historyLimit: 60,
    history: [],
    previousSnapshot: null,
    currentSnapshot: null,
    lastFingerprint: null,
    lastAcceptedAt: 0,
    lastReportAttemptAt: 0,
    liveRespondedAt: 0,
    sourceMode: 'starting',
    hasTransportError: false,
    inventoriesInitialized: false,
    lastInventoryRenderAt: 0,
    timer: null
  };

  function clamp(value, minimum, maximum) {
    return Math.min(maximum, Math.max(minimum, value));
  }

  function sectionAvailable(section) {
    return Boolean(section && (section.state === undefined || section.state === 'available'));
  }

  function formatBytes(bytes) {
    if (!Number.isFinite(bytes) || bytes < 0) return '—';
    const units = ['o', 'Kio', 'Mio', 'Gio', 'Tio'];
    let value = bytes;
    let unit = 0;
    while (value >= 1024 && unit < units.length - 1) {
      value /= 1024;
      unit += 1;
    }
    const digits = value >= 100 || unit === 0 ? 0 : value >= 10 ? 1 : 2;
    return `${new Intl.NumberFormat('fr-FR', { maximumFractionDigits: digits }).format(value)} ${units[unit]}`;
  }

  function formatRate(bytesPerSecond) {
    return Number.isFinite(bytesPerSecond) ? `${formatBytes(bytesPerSecond)}/s` : '—';
  }

  function formatOperations(value) {
    return Number.isFinite(value) ? `${DECIMAL.format(value)} op/s` : '—';
  }

  function formatAge(milliseconds) {
    if (!Number.isFinite(milliseconds) || milliseconds < 0) return 'à l’instant';
    const seconds = Math.floor(milliseconds / 1000);
    if (seconds < 2) return 'à l’instant';
    if (seconds < 60) return `il y a ${seconds} s`;
    const minutes = Math.floor(seconds / 60);
    if (minutes < 60) return `il y a ${minutes} min`;
    const hours = Math.floor(minutes / 60);
    if (hours < 24) return `il y a ${hours} h`;
    return new Intl.DateTimeFormat('fr-FR', { dateStyle: 'short', timeStyle: 'short' }).format(new Date(Date.now() - milliseconds));
  }

  function parseWallTimestamp(payload, lab, fallback) {
    const values = [
      payload.sample?.generated_at,
      payload.sample?.unix_milliseconds,
      payload.sampled_at,
      payload.generated_at,
      payload.collected_at,
      payload.timestamp,
      lab.sampled_at,
      lab.generated_at,
      lab.collected_at,
      lab.timestamp
    ];
    for (const value of values) {
      if (typeof value === 'string' && value.trim()) {
        const parsed = Date.parse(value);
        if (Number.isFinite(parsed)) return { value: parsed, trusted: true, raw: value };
      }
      if (Number.isFinite(value)) {
        const parsed = value > 1e12 ? value : value > 1e9 ? value * 1000 : null;
        if (Number.isFinite(parsed)) return { value: parsed, trusted: true, raw: String(value) };
      }
    }
    return { value: fallback, trusted: false, raw: null };
  }

  function parseTiming(payload, lab, fallback) {
    const wall = parseWallTimestamp(payload, lab, fallback);
    const monotonic = finiteNumber(payload.sample?.monotonic_milliseconds);
    if (monotonic !== null) {
      return {
        deltaTime: monotonic,
        wallTime: wall.value,
        trusted: wall.trusted,
        raw: `monotonic:${monotonic}`,
        clock: 'monotonic'
      };
    }
    return {
      deltaTime: wall.value,
      wallTime: wall.value,
      trusted: wall.trusted,
      raw: wall.raw,
      clock: wall.trusted ? 'wall' : 'browser'
    };
  }

  function isPhysicalDisk(name) {
    return /^(?:nvme\d+n\d+|mmcblk\d+|(?:sd|hd|vd|xvd)[a-z]+)$/.test(name);
  }

  function sumCounters(items, key) {
    let total = 0;
    let found = false;
    items.forEach(item => {
      const value = finiteNumber(item?.counters?.[key]);
      if (value !== null) {
        total += value;
        found = true;
      }
    });
    return found ? total : null;
  }

  function extractLab(payload) {
    if (!payload || typeof payload !== 'object') return null;
    if (payload.future_lab && typeof payload.future_lab === 'object') return payload.future_lab;
    if (payload.snapshot?.future_lab && typeof payload.snapshot.future_lab === 'object') return payload.snapshot.future_lab;
    if (payload.snapshot && typeof payload.snapshot === 'object') return payload.snapshot;
    return payload;
  }

  function normalizeSnapshot(payload, source, fetchedAt) {
    const lab = extractLab(payload);
    if (!lab || typeof lab !== 'object') throw new Error('Bloc Future Lab absent');

    const timing = parseTiming(payload, lab, fetchedAt);
    const cpuSource = sectionAvailable(lab.cpu) ? lab.cpu : null;
    const cpuCounters = cpuSource?.counters || {};
    const loadSource = sectionAvailable(lab.load) ? lab.load : null;
    const memorySource = sectionAvailable(lab.memory) ? lab.memory : null;
    const gpuSource = sectionAvailable(lab.gpu) ? lab.gpu : null;
    const networkSource = sectionAvailable(lab.network) ? lab.network : null;
    const networkInterfaces = Array.isArray(networkSource?.interfaces) ? networkSource.interfaces : [];
    const diskSource = sectionAvailable(lab.disks) ? lab.disks : null;
    const allDevices = Array.isArray(diskSource?.devices) ? diskSource.devices : [];
    const physicalDevices = allDevices.filter(device => isPhysicalDisk(String(device?.name || '')));
    const rateDevices = physicalDevices;

    let networkRx = finiteNumber(networkSource?.counters?.received_bytes);
    let networkTx = finiteNumber(networkSource?.counters?.transmitted_bytes);
    if (networkRx === null) networkRx = sumCounters(networkInterfaces, 'received_bytes');
    if (networkTx === null) networkTx = sumCounters(networkInterfaces, 'transmitted_bytes');

    const snapshot = {
      source,
      schemaName: typeof payload.schema?.name === 'string' ? payload.schema.name : null,
      schemaVersion: finiteNumber(payload.schema?.version),
      bootId: typeof payload.sample?.boot_id === 'string' ? payload.sample.boot_id : null,
      time: timing.deltaTime,
      wallTime: timing.wallTime,
      timestampTrusted: timing.trusted,
      timestampRaw: timing.raw,
      clock: timing.clock,
      fetchedAt,
      cpu: cpuSource ? {
        logicalCount: finiteNumber(cpuSource.logical_cpu_count, 1),
        busy: finiteNumber(cpuCounters.busy),
        total: finiteNumber(cpuCounters.total)
      } : null,
      load: loadSource ? {
        one: finiteNumber(loadSource.one_minute),
        five: finiteNumber(loadSource.five_minutes),
        fifteen: finiteNumber(loadSource.fifteen_minutes),
        runningTasks: finiteNumber(loadSource.running_tasks),
        totalTasks: finiteNumber(loadSource.total_tasks)
      } : null,
      memory: memorySource ? {
        totalKib: finiteNumber(memorySource.total, 1),
        availableKib: finiteNumber(memorySource.available),
        usedKib: finiteNumber(memorySource.used),
        swapTotalKib: finiteNumber(memorySource.swap_total),
        swapFreeKib: finiteNumber(memorySource.swap_free)
      } : null,
      gpu: gpuSource ? {
        index: finiteNumber(gpuSource.index),
        name: String(gpuSource.name || '').trim(),
        utilizationPercent: finiteNumber(gpuSource.utilization_percent),
        memoryUsedMib: finiteNumber(gpuSource.memory_used_mib),
        memoryTotalMib: finiteNumber(gpuSource.memory_total_mib, 1),
        temperatureCelsius: finiteNumber(gpuSource.temperature_celsius, -100),
        powerWatts: finiteNumber(gpuSource.power_watts),
        nvtopAvailable: gpuSource.nvtop_available === true
      } : lab.gpu ? { nvtopAvailable: lab.gpu.nvtop_available === true } : null,
      network: networkSource ? {
        receivedBytes: networkRx,
        transmittedBytes: networkTx,
        interfaces: networkInterfaces,
        reportedCount: finiteNumber(networkSource.reported_interface_count) ?? networkInterfaces.length,
        truncated: networkSource.truncated === true
      } : null,
      disks: diskSource ? {
        readsCompleted: sumCounters(rateDevices, 'reads_completed'),
        writesCompleted: sumCounters(rateDevices, 'writes_completed'),
        devices: allDevices,
        physicalDevices,
        rateDeviceCount: rateDevices.length,
        rateScope: physicalDevices.length ? 'physical' : 'none',
        reportedCount: finiteNumber(diskSource.reported_device_count) ?? allDevices.length,
        truncated: diskSource.truncated === true
      } : null
    };

    const hasData = snapshot.cpu || snapshot.load || snapshot.memory || snapshot.gpu || snapshot.network || snapshot.disks;
    if (!hasData) throw new Error('Aucune source Future Lab exploitable');

    const fingerprintParts = [
      source,
      snapshot.schemaName,
      snapshot.schemaVersion,
      snapshot.bootId,
      timing.trusted ? timing.raw : '',
      snapshot.cpu?.busy,
      snapshot.cpu?.total,
      snapshot.load?.one,
      snapshot.memory?.availableKib,
      snapshot.gpu?.utilizationPercent,
      snapshot.gpu?.memoryUsedMib,
      snapshot.gpu?.temperatureCelsius,
      snapshot.gpu?.powerWatts,
      snapshot.network?.receivedBytes,
      snapshot.network?.transmittedBytes,
      snapshot.disks?.readsCompleted,
      snapshot.disks?.writesCompleted
    ];
    snapshot.fingerprint = JSON.stringify(fingerprintParts);
    return snapshot;
  }

  async function fetchJson(url) {
    const separator = url.includes('?') ? '&' : '?';
    const response = await fetch(`${url}${separator}_=${Date.now()}`, {
      cache: 'no-store',
      credentials: 'same-origin'
    });
    if (!response.ok) throw new Error(`${url}: HTTP ${response.status}`);
    return response.json();
  }

  function acceptPayload(payload, source) {
    const now = Date.now();
    const snapshot = normalizeSnapshot(payload, source, now);
    if (source === 'live' && !liveSnapshotIsCurrent(snapshot, now, STALE_AFTER_MS))
      throw new Error('Instantané live ancien ou incompatible');
    if (snapshot.fingerprint === state.lastFingerprint) return false;

    const previous = state.currentSnapshot;
    const sample = createTelemetrySample(snapshot, previous);
    state.previousSnapshot = previous;
    state.currentSnapshot = snapshot;
    state.lastFingerprint = snapshot.fingerprint;
    state.lastAcceptedAt = Date.now();
    state.sourceMode = source;
    state.hasTransportError = false;
    state.history.push(sample);
    state.history = state.history.slice(-Math.min(state.historyLimit, MAX_HISTORY_POINTS));
    renderAll();
    return true;
  }

  async function poll() {
    if (state.paused || state.inFlight) {
      renderConnectionState();
      return;
    }
    state.inFlight = true;
    let liveWorked = false;
    try {
      const livePayload = await fetchJson(LIVE_URL);
      state.liveRespondedAt = Date.now();
      acceptPayload(livePayload, 'live');
      liveWorked = true;
    } catch (_error) {
      liveWorked = false;
    }

    if (!liveWorked && (!state.currentSnapshot || state.currentSnapshot.source !== 'report' || Date.now() - state.lastReportAttemptAt >= REPORT_RETRY_MS)) {
      state.lastReportAttemptAt = Date.now();
      try {
        const reportPayload = await fetchJson(REPORT_URL);
        acceptPayload(reportPayload, 'report');
      } catch (_error) {
        if (!state.currentSnapshot) state.hasTransportError = true;
      }
    }
    state.inFlight = false;
    renderConnectionState();
  }

  function setQuality(element, kind, label) {
    if (!element) return;
    element.className = `quality is-${kind}`;
    element.textContent = label;
  }

  function currentConnectionMode() {
    if (state.paused) return 'paused';
    if (!state.currentSnapshot) return state.hasTransportError ? 'error' : 'starting';
    if (state.currentSnapshot.source === 'report') return 'report';
    if (Date.now() - state.lastAcceptedAt > STALE_AFTER_MS) return 'stale';
    return 'live';
  }

  function renderConnectionState() {
    const mode = currentConnectionMode();
    const labels = {
      starting: 'Initialisation',
      live: 'Flux local actif',
      report: 'Rapport statique',
      stale: 'Flux à actualiser',
      error: 'Source indisponible',
      paused: 'Timeline suspendue'
    };
    elements.streamState.className = `stream-state is-${mode}`;
    elements.streamStateLabel.textContent = labels[mode];
    elements.sourceBanner.className = `source-banner is-${mode}`;

    if (mode === 'live') {
      elements.sourceTitle.textContent = 'Collecteur local synchronisé';
      elements.sourceDescription.textContent = 'Les taux sont calculés entre les instantanés successifs du flux Future Lab. Aucun échantillon ne quitte ce navigateur.';
      elements.sourceValue.textContent = 'Flux local 1 Hz';
      elements.freshnessValue.textContent = formatAge(Date.now() - state.lastAcceptedAt);
    } else if (mode === 'report') {
      elements.sourceTitle.textContent = 'Dernier rapport local chargé';
      elements.sourceDescription.textContent = 'Le flux temps réel est absent. Les valeurs directes restent visibles, mais les taux attendent un nouvel instantané comparable.';
      elements.sourceValue.textContent = 'report.json';
      elements.freshnessValue.textContent = state.currentSnapshot
        ? formatAge(Math.max(0, Date.now() - state.currentSnapshot.wallTime))
        : '—';
    } else if (mode === 'stale') {
      elements.sourceTitle.textContent = 'Le flux local ne change plus';
      elements.sourceDescription.textContent = 'Les dernières mesures sont conservées à l’écran, mais leur fraîcheur ne permet plus de les présenter comme du temps réel.';
      elements.sourceValue.textContent = 'Flux figé';
      elements.freshnessValue.textContent = formatAge(Date.now() - state.lastAcceptedAt);
    } else if (mode === 'paused') {
      elements.sourceTitle.textContent = 'Timeline suspendue par l’utilisateur';
      elements.sourceDescription.textContent = 'Aucun nouveau fichier n’est lu tant que la timeline n’est pas reprise.';
      elements.sourceValue.textContent = 'Pause locale';
      elements.freshnessValue.textContent = state.lastAcceptedAt ? formatAge(Date.now() - state.lastAcceptedAt) : '—';
    } else if (mode === 'error') {
      elements.sourceTitle.textContent = 'Aucune mesure Future Lab disponible';
      elements.sourceDescription.textContent = 'Ni le flux local ni le rapport de repli n’ont fourni un bloc Future Lab exploitable.';
      elements.sourceValue.textContent = 'Indisponible';
      elements.freshnessValue.textContent = '—';
    } else {
      elements.sourceTitle.textContent = 'Connexion au collecteur local…';
      elements.sourceDescription.textContent = 'Future Lab cherche un flux actualisé, puis utilisera le dernier rapport disponible si nécessaire.';
      elements.sourceValue.textContent = 'En attente';
      elements.freshnessValue.textContent = '—';
    }
  }

  function renderCpu(sample, snapshot) {
    const available = snapshot?.cpu;
    elements.cpuCount.textContent = Number.isFinite(available?.logicalCount)
      ? `${INTEGER.format(available.logicalCount)} CPU logiques`
      : 'Nombre de CPU inconnu';
    if (Number.isFinite(sample?.cpuRate)) {
      elements.cpuValue.textContent = DECIMAL.format(sample.cpuRate);
      elements.cpuDetail.textContent = `Delta calculé sur ${DECIMAL.format(sample.elapsed)} s à partir des ticks du noyau.`;
      setQuality(elements.cpuQuality, 'valid', 'Delta valide');
      charts.cpu.canvas.setAttribute('aria-label', `Activité CPU ${DECIMAL.format(sample.cpuRate)} pour cent, calculée entre deux instantanés.`);
    } else {
      elements.cpuValue.textContent = '—';
      elements.cpuDetail.textContent = available ? 'Une seconde mesure comparable est nécessaire.' : 'Compteurs CPU indisponibles.';
      setQuality(elements.cpuQuality, available ? 'waiting' : 'unavailable', available ? 'Synchronisation' : 'Indisponible');
      charts.cpu.canvas.setAttribute('aria-label', 'Activité CPU indisponible : deux instantanés comparables sont nécessaires.');
    }
  }

  function renderLoad(sample, snapshot) {
    const available = snapshot?.load;
    if (Number.isFinite(sample?.loadOne)) {
      elements.loadValue.textContent = LOAD.format(sample.loadOne);
      const normalized = Number.isFinite(sample.loadPercent) ? `${DECIMAL.format(sample.loadPercent)} % de la capacité logique` : 'normalisation indisponible';
      elements.loadDetail.textContent = `Charge 1 min · ${normalized}. Ce n’est pas le pourcentage d’activité CPU.`;
      elements.taskCount.textContent = Number.isFinite(available?.runningTasks) && Number.isFinite(available?.totalTasks)
        ? `${INTEGER.format(available.runningTasks)} / ${INTEGER.format(available.totalTasks)} tâches`
        : 'Tâches inconnues';
      setQuality(elements.loadQuality, 'direct', 'Valeur noyau');
      charts.load.canvas.setAttribute('aria-label', `Charge à une minute ${LOAD.format(sample.loadOne)}, rapportée à ${Number.isFinite(snapshot.cpu?.logicalCount) ? INTEGER.format(snapshot.cpu.logicalCount) : 'un nombre inconnu de'} CPU logiques.`);
    } else {
      elements.loadValue.textContent = '—';
      elements.loadDetail.textContent = 'Charge système indisponible.';
      elements.taskCount.textContent = 'Tâches inconnues';
      setQuality(elements.loadQuality, 'unavailable', 'Indisponible');
      charts.load.canvas.setAttribute('aria-label', 'Charge système indisponible.');
    }
  }

  function renderMemory(sample, snapshot) {
    const memory = snapshot?.memory;
    const memoryPercent = sample?.memoryPercent;
    const swapPercent = sample?.swapPercent;
    if (Number.isFinite(memoryPercent)) {
      elements.memoryValue.textContent = DECIMAL.format(memoryPercent);
      const usedKib = Number.isFinite(memory.usedKib) ? memory.usedKib : memory.totalKib - memory.availableKib;
      elements.memoryDetail.textContent = `${formatBytes(usedKib * 1024)} utilisés sur ${formatBytes(memory.totalKib * 1024)}`;
      elements.memoryFill.style.height = `${memoryPercent}%`;
      setQuality(elements.memoryQuality, 'direct', 'Photo locale');
    } else {
      elements.memoryValue.textContent = '—';
      elements.memoryDetail.textContent = 'RAM indisponible';
      elements.memoryFill.style.height = '0%';
      setQuality(elements.memoryQuality, 'unavailable', 'Indisponible');
    }
    if (Number.isFinite(swapPercent)) {
      elements.swapValue.textContent = DECIMAL.format(swapPercent);
      const swapUsed = Math.max(0, memory.swapTotalKib - memory.swapFreeKib);
      elements.swapDetail.textContent = memory.swapTotalKib > 0
        ? `${formatBytes(swapUsed * 1024)} utilisés sur ${formatBytes(memory.swapTotalKib * 1024)}`
        : 'Aucun espace swap déclaré';
    } else {
      elements.swapValue.textContent = '—';
      elements.swapDetail.textContent = 'Swap indisponible';
    }
    charts.memory.canvas.setAttribute('aria-label', Number.isFinite(memoryPercent)
      ? `Mémoire vive utilisée à ${DECIMAL.format(memoryPercent)} pour cent et swap utilisé à ${Number.isFinite(swapPercent) ? DECIMAL.format(swapPercent) : 'une valeur inconnue'} pour cent.`
      : 'Utilisation de la mémoire indisponible.');
  }

  function renderNetwork(sample, snapshot) {
    const network = snapshot?.network;
    elements.interfaceCount.textContent = network
      ? `${INTEGER.format(network.reportedCount)} interface${network.reportedCount > 1 ? 's' : ''}`
      : 'Interfaces inconnues';
    if (Number.isFinite(sample?.networkRxRate) && Number.isFinite(sample?.networkTxRate)) {
      elements.networkRx.textContent = formatBytes(sample.networkRxRate);
      elements.networkTx.textContent = formatBytes(sample.networkTxRate);
      setQuality(elements.networkQuality, 'valid', 'Delta valide');
      charts.network.canvas.setAttribute('aria-label', `Réseau : ${formatRate(sample.networkRxRate)} reçus et ${formatRate(sample.networkTxRate)} émis.`);
    } else {
      elements.networkRx.textContent = '—';
      elements.networkTx.textContent = '—';
      setQuality(elements.networkQuality, network ? 'waiting' : 'unavailable', network ? 'Synchronisation' : 'Indisponible');
      charts.network.canvas.setAttribute('aria-label', 'Débits réseau indisponibles : deux compteurs comparables sont nécessaires.');
    }
  }

  function renderGpu(sample, snapshot) {
    const gpu = snapshot?.gpu;
    elements.gpuName.textContent = gpu?.name || 'Activité GPU';
    elements.nvtopState.textContent = gpu?.nvtopAvailable ? 'nvtop disponible' : 'nvtop non détecté';
    if (Number.isFinite(sample?.gpuPercent) && Number.isFinite(sample?.gpuMemoryPercent)) {
      elements.gpuValue.textContent = DECIMAL.format(sample.gpuPercent);
      elements.gpuMemoryValue.textContent = DECIMAL.format(sample.gpuMemoryPercent);
      const temperature = Number.isFinite(sample.gpuTemperatureCelsius)
        ? `${DECIMAL.format(sample.gpuTemperatureCelsius)} °C`
        : 'température inconnue';
      const power = Number.isFinite(sample.gpuPowerWatts)
        ? `${DECIMAL.format(sample.gpuPowerWatts)} W`
        : 'puissance inconnue';
      elements.gpuThermal.textContent = `${temperature} · ${power}`;
      setQuality(elements.gpuQuality, 'direct', 'Mesure NVIDIA');
      charts.gpu.canvas.setAttribute('aria-label', `GPU utilisé à ${DECIMAL.format(sample.gpuPercent)} pour cent, VRAM utilisée à ${DECIMAL.format(sample.gpuMemoryPercent)} pour cent, ${temperature}, ${power}.`);
    } else {
      elements.gpuValue.textContent = '—';
      elements.gpuMemoryValue.textContent = '—';
      elements.gpuThermal.textContent = '— °C · — W';
      setQuality(elements.gpuQuality, 'unavailable', gpu ? 'Mesures absentes' : 'GPU NVIDIA absent');
      charts.gpu.canvas.setAttribute('aria-label', 'Activité GPU NVIDIA indisponible.');
    }
  }

  function renderDisks(sample, snapshot) {
    const disks = snapshot?.disks;
    const scope = disks?.rateScope === 'physical'
      ? `${disks.rateDeviceCount} disque${disks.rateDeviceCount > 1 ? 's' : ''} physique${disks.rateDeviceCount > 1 ? 's' : ''}`
      : disks?.rateScope === 'reported'
        ? `${disks.rateDeviceCount} périphérique${disks.rateDeviceCount > 1 ? 's' : ''} rapporté${disks.rateDeviceCount > 1 ? 's' : ''}`
        : 'Disques inconnus';
    elements.diskCount.textContent = scope;
    if (Number.isFinite(sample?.diskReadRate) && Number.isFinite(sample?.diskWriteRate)) {
      elements.diskRead.textContent = DECIMAL.format(sample.diskReadRate);
      elements.diskWrite.textContent = DECIMAL.format(sample.diskWriteRate);
      setQuality(elements.diskQuality, 'valid', 'Delta valide');
      charts.disk.canvas.setAttribute('aria-label', `Disques : ${formatOperations(sample.diskReadRate)} en lecture et ${formatOperations(sample.diskWriteRate)} en écriture.`);
    } else {
      elements.diskRead.textContent = '—';
      elements.diskWrite.textContent = '—';
      setQuality(elements.diskQuality, disks ? 'waiting' : 'unavailable', disks ? 'Synchronisation' : 'Indisponible');
      charts.disk.canvas.setAttribute('aria-label', 'Activité disque indisponible : deux compteurs comparables sont nécessaires.');
    }
  }

  function createDataCell(label, value) {
    const cell = document.createElement('span');
    const title = document.createElement('small');
    title.textContent = label;
    const content = document.createElement('span');
    content.textContent = value;
    cell.append(title, content);
    return cell;
  }

  function renderInventories(snapshot, force = false) {
    const now = Date.now();
    const drawerOpen = elements.interfacesDetails.open || elements.disksDetails.open;
    if (state.inventoriesInitialized && !force && !drawerOpen) return;
    if (!force && state.inventoriesInitialized && now - state.lastInventoryRenderAt < 2000) return;
    state.inventoriesInitialized = true;
    state.lastInventoryRenderAt = now;
    const interfaces = snapshot?.network?.interfaces || [];
    elements.interfacesSummary.textContent = interfaces.length
      ? `${interfaces.length} affichée${interfaces.length > 1 ? 's' : ''}${snapshot.network.truncated ? ' · liste partielle' : ''}`
      : 'Aucune interface rapportée';
    if (interfaces.length) {
      elements.interfacesList.replaceChildren(...interfaces.map(item => {
        const counters = item.counters || {};
        const row = document.createElement('article');
        row.className = 'data-row';
        const name = document.createElement('strong');
        name.textContent = item.name || 'Interface sans nom';
        const errors = (finiteNumber(counters.received_errors) || 0) + (finiteNumber(counters.transmitted_errors) || 0);
        const dropped = (finiteNumber(counters.received_dropped) || 0) + (finiteNumber(counters.transmitted_dropped) || 0);
        row.append(
          name,
          createDataCell('Reçu, cumulatif', formatBytes(finiteNumber(counters.received_bytes))),
          createDataCell('Émis, cumulatif', formatBytes(finiteNumber(counters.transmitted_bytes))),
          createDataCell('Erreurs · pertes', `${INTEGER.format(errors)} · ${INTEGER.format(dropped)}`)
        );
        return row;
      }));
    } else {
      const empty = document.createElement('p');
      empty.className = 'empty-state';
      empty.textContent = 'Aucune donnée réseau reçue.';
      elements.interfacesList.replaceChildren(empty);
    }

    const devices = snapshot?.disks?.devices || [];
    const physicalCount = snapshot?.disks?.physicalDevices.length || 0;
    elements.disksSummary.textContent = devices.length
      ? `${devices.length} rapporté${devices.length > 1 ? 's' : ''} · ${physicalCount} utilisé${physicalCount > 1 ? 's' : ''} pour les taux`
      : 'Aucun périphérique rapporté';
    if (devices.length) {
      elements.disksList.replaceChildren(...devices.map(item => {
        const counters = item.counters || {};
        const row = document.createElement('article');
        row.className = 'data-row';
        const name = document.createElement('strong');
        name.textContent = item.name || 'Périphérique sans nom';
        row.append(
          name,
          createDataCell('Lectures, cumulatif', INTEGER.format(finiteNumber(counters.reads_completed) || 0)),
          createDataCell('Écritures, cumulatif', INTEGER.format(finiteNumber(counters.writes_completed) || 0)),
          createDataCell('Portée du taux', isPhysicalDisk(String(item.name || '')) ? 'disque physique' : 'détail uniquement')
        );
        return row;
      }));
    } else {
      const empty = document.createElement('p');
      empty.className = 'empty-state';
      empty.textContent = 'Aucune donnée disque reçue.';
      elements.disksList.replaceChildren(empty);
    }
  }

  function niceMaximum(maximum) {
    if (!Number.isFinite(maximum) || maximum <= 0) return 1;
    const magnitude = 10 ** Math.floor(Math.log10(maximum));
    const normalized = maximum / magnitude;
    const step = normalized <= 1 ? 1 : normalized <= 2 ? 2 : normalized <= 5 ? 5 : 10;
    return step * magnitude;
  }

  function drawEmptyChart(context, width, height) {
    context.save();
    context.fillStyle = 'rgba(157, 176, 201, .72)';
    context.font = '700 11px system-ui, sans-serif';
    context.textAlign = 'center';
    context.fillText('EN ATTENTE DE DEUX INSTANTANÉS', width / 2, height / 2);
    context.restore();
  }

  function drawChart(chart) {
    const canvas = chart.canvas;
    if (!canvas) return;
    const bounds = canvas.getBoundingClientRect();
    if (bounds.width < 2 || bounds.height < 2) return;
    const pixelRatio = Math.min(window.devicePixelRatio || 1, 2);
    const width = Math.round(bounds.width);
    const height = Math.round(bounds.height);
    canvas.width = Math.round(width * pixelRatio);
    canvas.height = Math.round(height * pixelRatio);
    const context = canvas.getContext('2d');
    context.setTransform(pixelRatio, 0, 0, pixelRatio, 0, 0);
    context.clearRect(0, 0, width, height);

    const padding = { top: 13, right: 12, bottom: 17, left: 44 };
    const plotWidth = Math.max(1, width - padding.left - padding.right);
    const plotHeight = Math.max(1, height - padding.top - padding.bottom);
    const visible = state.history.slice(-state.historyLimit);
    const values = visible.flatMap(sample => chart.series.map(series => sample[series.key])).filter(Number.isFinite);
    const dynamicMaximum = niceMaximum(Math.max(...values, 0));
    const maximum = chart.fixedMax || Math.max(chart.minimumMax || 0, dynamicMaximum);

    context.strokeStyle = 'rgba(126, 173, 211, .16)';
    context.lineWidth = 1;
    context.fillStyle = 'rgba(157, 176, 201, .68)';
    context.font = '600 9px system-ui, sans-serif';
    context.textAlign = 'right';
    for (let line = 0; line <= 4; line += 1) {
      const y = padding.top + plotHeight * line / 4;
      context.beginPath();
      context.moveTo(padding.left, y);
      context.lineTo(width - padding.right, y);
      context.stroke();
      if (line === 0 || line === 4) {
        const numeric = maximum * (4 - line) / 4;
        const formatted = chart.valueFormatter ? chart.valueFormatter(numeric) : INTEGER.format(numeric);
        context.fillText(`${formatted}${chart.suffix || ''}`, padding.left - 6, y + 3);
      }
    }

    if (!values.length) {
      drawEmptyChart(context, width, height);
      return;
    }

    chart.series.forEach((series, seriesIndex) => {
      let started = false;
      context.beginPath();
      visible.forEach((sample, index) => {
        const value = sample[series.key];
        if (!Number.isFinite(value)) {
          started = false;
          return;
        }
        const x = padding.left + plotWidth * index / Math.max(1, state.historyLimit - 1);
        const y = padding.top + plotHeight * (1 - clamp(value / maximum, 0, 1));
        if (!started) {
          context.moveTo(x, y);
          started = true;
        } else {
          context.lineTo(x, y);
        }
      });
      context.strokeStyle = series.color;
      context.lineWidth = seriesIndex === 0 ? 2.2 : 1.8;
      context.lineJoin = 'round';
      context.lineCap = 'round';
      if (!REDUCED_MOTION.matches) {
        context.shadowColor = series.color;
        context.shadowBlur = 8;
      }
      context.stroke();
      context.shadowBlur = 0;
    });

    context.fillStyle = 'rgba(157, 176, 201, .65)';
    context.font = '600 9px system-ui, sans-serif';
    context.textAlign = 'left';
    context.fillText(`${visible.length}/${state.historyLimit}`, padding.left, height - 4);
  }

  function drawAllCharts() {
    Object.values(charts).forEach(drawChart);
  }

  function renderSummary(sample, snapshot) {
    const sampleLabel = `${state.history.length} mesure${state.history.length > 1 ? 's' : ''}`;
    elements.sampleCount.textContent = sampleLabel;
    elements.windowValue.textContent = `${state.historyLimit} points max.`;
    if (!sample) {
      elements.telemetrySummary.textContent = 'Une seconde mesure est nécessaire pour calculer les taux.';
      return;
    }
    const rateCount = [sample.cpuRate, sample.networkRxRate, sample.diskReadRate].filter(Number.isFinite).length;
    elements.telemetrySummary.textContent = rateCount
      ? `${rateCount}/3 familles de taux calculées sur le dernier intervalle. RAM et charge restent des valeurs directes.`
      : 'Valeurs directes disponibles. Les taux attendent encore deux instantanés comparables.';
    const accessibleParts = [
      Number.isFinite(sample.cpuRate) ? `CPU ${DECIMAL.format(sample.cpuRate)} pour cent` : null,
      Number.isFinite(sample.loadOne) ? `charge ${LOAD.format(sample.loadOne)}` : null,
      Number.isFinite(sample.memoryPercent) ? `RAM ${DECIMAL.format(sample.memoryPercent)} pour cent` : null,
      Number.isFinite(sample.gpuPercent) ? `GPU ${DECIMAL.format(sample.gpuPercent)} pour cent` : null,
      Number.isFinite(sample.networkRxRate) ? `réseau reçu ${formatRate(sample.networkRxRate)}` : null,
      Number.isFinite(sample.diskReadRate) ? `lectures disque ${formatOperations(sample.diskReadRate)}` : null
    ].filter(Boolean);
    elements.screenReaderSummary.textContent = accessibleParts.length
      ? `Nouvelle mesure locale : ${accessibleParts.join(', ')}.`
      : `Nouvel instantané ${snapshot?.source === 'live' ? 'du flux local' : 'du rapport'}, taux en attente.`;
  }

  function publishAssistantTelemetry(sample, snapshot) {
    if (!sample || !snapshot) return;
    document.dispatchEvent(new CustomEvent('linux-doctor:future-lab-telemetry', {
      detail: {
        sampledAt: snapshot.wallTime,
        source: snapshot.source,
        cpuPercent: sample.cpuRate,
        logicalCpuCount: snapshot.cpu?.logicalCount ?? null,
        loadOne: sample.loadOne,
        loadPercent: sample.loadPercent,
        memoryPercent: sample.memoryPercent,
        swapPercent: sample.swapPercent,
        gpuPercent: sample.gpuPercent,
        gpuMemoryPercent: sample.gpuMemoryPercent,
        gpuTemperatureCelsius: sample.gpuTemperatureCelsius,
        gpuPowerWatts: sample.gpuPowerWatts,
        networkRxBytesPerSecond: sample.networkRxRate,
        networkTxBytesPerSecond: sample.networkTxRate,
        diskReadsPerSecond: sample.diskReadRate,
        diskWritesPerSecond: sample.diskWriteRate
      }
    }));
  }

  function renderAll() {
    const snapshot = state.currentSnapshot;
    const sample = state.history.at(-1) || null;
    renderCpu(sample, snapshot);
    renderLoad(sample, snapshot);
    renderMemory(sample, snapshot);
    renderGpu(sample, snapshot);
    renderNetwork(sample, snapshot);
    renderDisks(sample, snapshot);
    renderInventories(snapshot);
    renderSummary(sample, snapshot);
    publishAssistantTelemetry(sample, snapshot);
    renderConnectionState();
    window.requestAnimationFrame(drawAllCharts);
  }

  elements.historyLimit.addEventListener('change', () => {
    const requested = finiteNumber(elements.historyLimit.value, 1) || 60;
    state.historyLimit = clamp(Math.round(requested), 1, MAX_HISTORY_POINTS);
    state.history = state.history.slice(-state.historyLimit);
    renderAll();
  });

  elements.pauseStream.addEventListener('click', () => {
    state.paused = !state.paused;
    elements.pauseStream.setAttribute('aria-pressed', String(state.paused));
    elements.pauseStream.replaceChildren();
    const icon = document.createElement('span');
    icon.setAttribute('aria-hidden', 'true');
    icon.textContent = state.paused ? '▶' : 'Ⅱ';
    elements.pauseStream.append(icon, document.createTextNode(state.paused ? ' Reprendre' : ' Suspendre'));
    renderConnectionState();
    if (!state.paused) poll();
  });

  elements.clearTimeline.addEventListener('click', () => {
    state.history = [];
    state.previousSnapshot = null;
    state.currentSnapshot = null;
    state.lastFingerprint = null;
    state.lastAcceptedAt = 0;
    state.sourceMode = 'starting';
    renderAll();
    if (!state.paused) poll();
  });

  [elements.interfacesDetails, elements.disksDetails].forEach(drawer => {
    drawer.addEventListener('toggle', () => {
      if (drawer.open) renderInventories(state.currentSnapshot, true);
    });
  });

  window.addEventListener('resize', () => window.requestAnimationFrame(drawAllCharts));
  document.addEventListener('visibilitychange', () => {
    if (!document.hidden && !state.paused) poll();
  });
  REDUCED_MOTION.addEventListener?.('change', drawAllCharts);

  if ('ResizeObserver' in window) {
    const observer = new ResizeObserver(() => window.requestAnimationFrame(drawAllCharts));
    Object.values(charts).forEach(chart => observer.observe(chart.canvas));
  }

  renderAll();
  poll();
  state.timer = window.setInterval(poll, POLL_INTERVAL_MS);
})();
