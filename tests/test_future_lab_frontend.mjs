import assert from 'node:assert/strict';
import test from 'node:test';

import {
  createTelemetrySample,
  finiteNumber,
  liveSnapshotIsCurrent
} from '../frontend/future-lab-core.js';
import {
  buildAssistantFacts,
  buildModelMessages,
  validateModelAnswer
} from '../frontend/future-lab-ai.js';

const BOOT_ID = '01234567-89ab-cdef-0123-456789abcdef';

function snapshot(time = 1000) {
  return {
    source: 'live',
    schemaName: 'linux-doctor.future-lab.live',
    schemaVersion: 1,
    bootId: BOOT_ID,
    time,
    wallTime: 100000,
    timestampTrusted: true,
    cpu: { logicalCount: 8, busy: 100, total: 1000 },
    load: { one: 16 },
    memory: {
      totalKib: 16000,
      availableKib: 8000,
      usedKib: 8000,
      swapTotalKib: 4000,
      swapFreeKib: 3000
    },
    network: {
      truncated: false,
      interfaces: [{
        name: 'eth0',
        counters: { received_bytes: 1000, transmitted_bytes: 500 }
      }]
    },
    disks: {
      truncated: false,
      physicalDevices: [{
        name: 'sda', major: 8, minor: 0,
        counters: { reads_completed: 100, writes_completed: 50 }
      }]
    }
  };
}

function nextSnapshot() {
  const next = structuredClone(snapshot(2000));
  next.cpu.busy = 120;
  next.cpu.total = 1100;
  next.network.interfaces[0].counters.received_bytes = 2024;
  next.network.interfaces[0].counters.transmitted_bytes = 1012;
  next.disks.physicalDevices[0].counters.reads_completed = 102;
  next.disks.physicalDevices[0].counters.writes_completed = 54;
  return next;
}

test('calcule seulement les deltas de membres identiques', () => {
  const sample = createTelemetrySample(nextSnapshot(), snapshot());
  assert.equal(sample.cpuRate, 20);
  assert.equal(sample.loadPercent, 200);
  assert.equal(sample.networkRxRate, 1024);
  assert.equal(sample.networkTxRate, 512);
  assert.equal(sample.diskReadRate, 2);
  assert.equal(sample.diskWriteRate, 4);
});

test('rejette un changement de boot ou de topologie', () => {
  const changedBoot = nextSnapshot();
  changedBoot.bootId = 'fedcba98-7654-3210-fedc-ba9876543210';
  assert.equal(createTelemetrySample(changedBoot, snapshot()).cpuRate, null);

  const changedNetwork = nextSnapshot();
  changedNetwork.network.interfaces.push({
    name: 'wg0', counters: { received_bytes: 1, transmitted_bytes: 1 }
  });
  assert.equal(createTelemetrySample(changedNetwork, snapshot()).networkRxRate, null);

  const changedDisk = nextSnapshot();
  changedDisk.disks.physicalDevices[0].minor = 1;
  assert.equal(createTelemetrySample(changedDisk, snapshot()).diskReadRate, null);
});

test('rejette les compteurs décroissants et les snapshots live anciens', () => {
  const reset = nextSnapshot();
  reset.network.interfaces[0].counters.received_bytes = 10;
  assert.equal(createTelemetrySample(reset, snapshot()).networkRxRate, null);

  const current = snapshot();
  assert.equal(liveSnapshotIsCurrent(current, 103999), true);
  assert.equal(liveSnapshotIsCurrent(current, 104001), false);
  current.schemaVersion = 2;
  assert.equal(liveSnapshotIsCurrent(current, 100000), false);
  current.schemaVersion = 1;
  current.timestampTrusted = false;
  assert.equal(liveSnapshotIsCurrent(current, 100000), false);
});

test('ne transforme jamais une absence de télémétrie en zéro', () => {
  assert.equal(finiteNumber(null), null);
  assert.equal(finiteNumber(undefined), null);
  const facts = buildAssistantFacts({
    cpuPercent: null,
    memoryPercent: 33,
    swapPercent: null,
    networkRxBytesPerSecond: null,
    networkTxBytesPerSecond: null,
    diskReadsPerSecond: null,
    diskWritesPerSecond: null
  });
  assert.equal(facts.length, 1);
  assert.match(facts[0].display, /RAM/);
});

test('borne les réponses du modèle aux faits qualitatifs', () => {
  const telemetry = { cpuPercent: 20, memoryPercent: 33 };
  const facts = buildAssistantFacts(telemetry);
  const messages = buildModelMessages(telemetry, 'Que retenir ?');
  assert.doesNotMatch(messages[1].content, /20|33/);
  assert.match(validateModelAnswer('Le CPU et la RAM restent dans une zone confortable.', facts), /CPU/);
  assert.throws(() => validateModelAnswer('Le CPU est à 42 %.', facts), /valeurs/);
  assert.throws(() => validateModelAnswer('Pour le CPU, lance sudo apt update.', facts), /commande/);
  assert.throws(() => validateModelAnswer(
    'Le CPU est saturé à cause de Proton et la mémoire provoque des saccades.', facts
  ), /cause|diagnostic/);
  assert.throws(() => validateModelAnswer(
    'Le réseau souffre sûrement d’une panne et le disque est défectueux.', facts
  ), /cause|diagnostic|absente/);
});
