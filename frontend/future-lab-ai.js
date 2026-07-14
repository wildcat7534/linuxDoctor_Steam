const MODEL_ID = 'onnx-community/gemma-3-1b-it-ONNX';
const MODEL_REVISION = 'a58439f40017d3b99c7d378ff525e54e0ba08ebf';
const MODEL_DTYPE = 'int8';
const TRANSFORMERS_VERSION = '4.2.0';
const RUNTIME_MODULE = 'vendor/transformers/transformers.web.min.js';
const WASM_DIRECTORY = 'vendor/transformers/';
const REQUIRED_ASSETS = [
  'future-lab-ai-worker.js',
  RUNTIME_MODULE,
  `${WASM_DIRECTORY}ort.webgpu.bundle.min.mjs`,
  `${WASM_DIRECTORY}ort-wasm-simd-threaded.mjs`,
  `${WASM_DIRECTORY}ort-wasm-simd-threaded.wasm`,
  `${WASM_DIRECTORY}ort-wasm-simd-threaded.asyncify.mjs`,
  `${WASM_DIRECTORY}ort-wasm-simd-threaded.asyncify.wasm`,
  `${WASM_DIRECTORY}ort-wasm-simd-threaded.jsep.mjs`,
  `${WASM_DIRECTORY}ort-wasm-simd-threaded.jsep.wasm`,
  `${WASM_DIRECTORY}ort-wasm-simd-threaded.jspi.mjs`,
  `${WASM_DIRECTORY}ort-wasm-simd-threaded.jspi.wasm`,
  `models/${MODEL_ID}/config.json`,
  `models/${MODEL_ID}/generation_config.json`,
  `models/${MODEL_ID}/special_tokens_map.json`,
  `models/${MODEL_ID}/tokenizer.json`,
  `models/${MODEL_ID}/tokenizer_config.json`,
  `models/${MODEL_ID}/onnx/model_int8.onnx`,
  'models/local-ai-manifest.json'
];

const NUMBER = new Intl.NumberFormat('fr-FR', {
  minimumFractionDigits: 1,
  maximumFractionDigits: 1
});

function finite(value) {
  if (value === null || value === undefined || value === '') return null;
  const number = Number(value);
  return Number.isFinite(number) && number >= 0 ? number : null;
}

function formatRate(bytesPerSecond) {
  if (!Number.isFinite(bytesPerSecond)) return '—';
  const units = ['o/s', 'Kio/s', 'Mio/s', 'Gio/s'];
  let value = bytesPerSecond;
  let unit = 0;
  while (value >= 1024 && unit < units.length - 1) {
    value /= 1024;
    unit += 1;
  }
  return `${NUMBER.format(value)} ${units[unit]}`;
}

function pressureLevel(value, moderate, high) {
  if (!Number.isFinite(value)) return { key: 'unknown', label: 'indisponible' };
  if (value >= high) return { key: 'high', label: 'très sollicité' };
  if (value >= moderate) return { key: 'moderate', label: 'sollicité' };
  return { key: 'comfortable', label: 'dans une zone confortable' };
}

export function buildAssistantFacts(telemetry) {
  if (!telemetry || typeof telemetry !== 'object') return [];
  const facts = [];
  const cpu = finite(telemetry.cpuPercent);
  const load = finite(telemetry.loadOne);
  const loadPercent = finite(telemetry.loadPercent);
  const memory = finite(telemetry.memoryPercent);
  const swap = finite(telemetry.swapPercent);
  const rx = finite(telemetry.networkRxBytesPerSecond);
  const tx = finite(telemetry.networkTxBytesPerSecond);
  const reads = finite(telemetry.diskReadsPerSecond);
  const writes = finite(telemetry.diskWritesPerSecond);

  if (cpu !== null) {
    const level = pressureLevel(cpu, 70, 90);
    facts.push({
      metric: 'cpu', level: level.key,
      display: `CPU : ${NUMBER.format(cpu)} % d’activité, ${level.label}.`,
      prompt: `Activité CPU : ${level.label}.`
    });
  }
  if (load !== null) {
    const level = pressureLevel(loadPercent, 70, 100);
    const cpuCount = finite(telemetry.logicalCpuCount);
    const context = cpuCount !== null ? ` pour ${Math.round(cpuCount)} CPU logiques` : '';
    facts.push({
      metric: 'load', level: level.key,
      display: `Charge 1 min : ${NUMBER.format(load)}${context}, ${level.label}.`,
      prompt: `Charge système : ${level.label}.`
    });
  }
  if (memory !== null) {
    const level = pressureLevel(memory, 75, 90);
    facts.push({
      metric: 'memory', level: level.key,
      display: `RAM : ${NUMBER.format(memory)} % utilisés, ${level.label}.`,
      prompt: `Mémoire vive : ${level.label}.`
    });
  }
  if (swap !== null) {
    facts.push({
      metric: 'swap', level: 'neutral',
      display: `Swap : ${NUMBER.format(swap)} % occupés ; cette occupation seule ne mesure pas une pression mémoire.`,
      prompt: 'Swap : occupation observée, sans conclusion sur une pression mémoire.'
    });
  }
  if (rx !== null && tx !== null) {
    facts.push({
      metric: 'network', level: 'neutral',
      display: `Réseau : ${formatRate(rx)} reçus et ${formatRate(tx)} émis ; c’est une activité, pas un score de qualité.`,
      prompt: `Réseau : activité mesurée, sans conclusion sur sa qualité.`
    });
  }
  if (reads !== null && writes !== null) {
    facts.push({
      metric: 'disk', level: 'neutral',
      display: `Disques : ${NUMBER.format(reads)} lectures/s et ${NUMBER.format(writes)} écritures/s ; aucune panne ne peut être déduite de ce seul débit.`,
      prompt: `Stockage : activité mesurée, sans signe de panne déduit.`
    });
  }
  return facts;
}

export function buildModelMessages(telemetry, question) {
  const facts = buildAssistantFacts(telemetry);
  const request = String(question || '').trim() || 'Que retenir de cet instantané pour une session de jeu ?';
  return [
    {
      role: 'system',
      content: 'Tu es le copilote local de Linux Doctor. Tu reformules uniquement des constats déjà calculés par le logiciel. Tu ne poses aucun diagnostic certain, tu n’inventes aucune cause et tu ne proposes aucune commande. Réponds en français simple, avec au plus trois puces.'
    },
    {
      role: 'user',
      content: `Question : ${request}\n\nConstats vérifiés par le logiciel :\n- ${facts.map(fact => fact.prompt).join('\n- ')}\n\nN’ajoute ni chiffre ni mesure. Ne confonds pas activité et panne. Si la question dépasse ces constats, dis simplement que les données affichées ne permettent pas de répondre.`
    }
  ];
}

export function validateModelAnswer(value, facts = []) {
  const text = String(value || '')
    .replace(/<\/?(?:end_of_turn|start_of_turn)>/gi, '')
    .replace(/\n{3,}/g, '\n\n')
    .trim();
  if (text.length < 12) throw new Error('La réponse locale est vide ou incomplète.');
  if (text.length > 1200) throw new Error('La réponse locale a dépassé la limite de sécurité.');
  if (/\d/.test(text)) throw new Error('Le modèle a ajouté des valeurs non vérifiables.');
  if (/(?:^|\s)(?:sudo|apt(?:-get)?|dnf|pacman|snap|flatpak|rm|mv|cp|chmod|chown|systemctl)(?:\s|$)/i.test(text)) {
    throw new Error('Le modèle a proposé une commande non autorisée.');
  }
  if (/(?:à cause|provoqu|entraîn|panne|défect|saccad|surchauff|ralenti|danger|urgent|répar|diagnostic certain)/i.test(text)) {
    throw new Error('Le modèle a ajouté une cause ou un diagnostic non vérifié.');
  }
  if (!/(cpu|processeur|charge|mémoire|ram|swap|réseau|disque|stockage)/i.test(text)) {
    throw new Error('Le modèle s’est écarté des mesures Future Lab.');
  }
  const metricPatterns = new Map([
    ['cpu', /\b(?:cpu|processeur)\b/i],
    ['load', /\bcharge(?:\s+système)?\b/i],
    ['memory', /\b(?:mémoire|ram)\b/i],
    ['swap', /\bswap\b/i],
    ['network', /\bréseau\b/i],
    ['disk', /\b(?:disque|stockage)\b/i]
  ]);
  const factByMetric = new Map(facts.map(fact => [fact.metric, fact]));
  for (const [metric, pattern] of metricPatterns) {
    if (pattern.test(text) && !factByMetric.has(metric)) {
      throw new Error('Le modèle a mentionné une mesure absente de l’instantané.');
    }
  }
  for (const sentence of text.split(/[.!?\n]+/)) {
    const status = /très\s+sollicit|satur/i.test(sentence) ? 'high'
      : /\bsollicit[\wéèéê]*/i.test(sentence) ? 'moderate'
        : /\b(?:confortable|calme|faible|normal(?:e)?)\b/i.test(sentence) ? 'comfortable'
          : null;
    if (!status) continue;
    const mentioned = [...metricPatterns].filter(([, pattern]) => pattern.test(sentence));
    if (!mentioned.length || mentioned.some(([metric]) => factByMetric.get(metric)?.level !== status)) {
      throw new Error('Le modèle a contredit le niveau calculé par Future Lab.');
    }
  }
  return text;
}

function initializeAssistant() {
  const elements = {
    panel: document.querySelector('#future-lab-ai'),
    factsTitle: document.querySelector('#ai-facts-title'),
    facts: document.querySelector('#ai-facts'),
    summary: document.querySelector('#ai-summary'),
    install: document.querySelector('#ai-install'),
    consent: document.querySelector('#ai-consent'),
    prompt: document.querySelector('#ai-prompt'),
    run: document.querySelector('#ai-run'),
    check: document.querySelector('#ai-check'),
    badge: document.querySelector('#ai-model-state'),
    progress: document.querySelector('#ai-progress'),
    status: document.querySelector('#ai-status')
  };
  if (Object.values(elements).some(element => !element)) return;

  const state = {
    assetsReady: false,
    checking: false,
    loading: false,
    running: false,
    telemetry: null,
    factsLocked: false,
    modelLoaded: false,
    worker: null,
    workerRequest: null
  };

  function setBadge(kind, label) {
    elements.badge.className = `quality is-${kind}`;
    elements.badge.textContent = label;
  }

  function announce(message) {
    elements.status.textContent = message;
  }

  function updateControls() {
    const busy = state.checking || state.loading || state.running;
    elements.consent.disabled = !state.assetsReady || busy;
    elements.prompt.disabled = !state.assetsReady || !elements.consent.checked || busy;
    elements.run.disabled = !state.assetsReady || !elements.consent.checked || !state.telemetry || busy;
    elements.check.disabled = busy;
    elements.run.textContent = state.modelLoaded ? 'Analyser localement' : 'Charger et analyser';
  }

  function renderFacts(telemetry = state.telemetry) {
    const facts = buildAssistantFacts(telemetry);
    const rows = facts.length ? facts : [{ display: 'En attente d’une mesure exploitable.' }];
    elements.facts.replaceChildren(...rows.map(fact => {
      const item = document.createElement('li');
      item.textContent = fact.display;
      return item;
    }));
  }

  async function assetExists(path) {
    const response = await fetch(new URL(path, import.meta.url), {
      method: 'HEAD',
      cache: 'no-store',
      credentials: 'same-origin'
    });
    return response.ok;
  }

  async function checkAssets() {
    if (state.checking || state.loading || state.running) return;
    state.checking = true;
    elements.panel.dataset.aiState = 'checking';
    setBadge('waiting', 'Vérification locale');
    announce('Vérification des fichiers du modèle local.');
    updateControls();
    try {
      const checks = await Promise.all(REQUIRED_ASSETS.map(assetExists));
      const manifestResponse = await fetch(new URL('models/local-ai-manifest.json', import.meta.url), {
        cache: 'no-store',
        credentials: 'same-origin'
      });
      const manifest = manifestResponse.ok ? await manifestResponse.json() : null;
      state.assetsReady = checks.every(Boolean) &&
        manifest?.schema === 'linux-doctor.local-ai' && manifest?.version === 1 &&
        manifest?.model_id === MODEL_ID && manifest?.revision === MODEL_REVISION &&
        manifest?.dtype === MODEL_DTYPE && manifest?.transformers_js === TRANSFORMERS_VERSION;
    } catch (_error) {
      state.assetsReady = false;
    }
    state.checking = false;
    elements.install.hidden = state.assetsReady;
    if (state.assetsReady) {
      elements.panel.dataset.aiState = state.modelLoaded ? 'ready' : 'available';
      setBadge(state.modelLoaded ? 'valid' : 'direct', state.modelLoaded ? 'Modèle chargé' : 'Prêt à charger');
      announce('Le modèle et le runtime sont disponibles sur cette machine.');
    } else {
      elements.panel.dataset.aiState = 'unavailable';
      elements.consent.checked = false;
      setBadge('unavailable', 'Module à installer');
      announce('Le module IA local est absent. La commande d’installation est affichée.');
    }
    updateControls();
  }

  function updateProgress(progress) {
    const direct = finite(progress?.progress);
    const loaded = finite(progress?.loaded);
    const total = finite(progress?.total);
    const percentage = direct !== null
      ? direct
      : loaded !== null && total !== null && total > 0
        ? loaded * 100 / total
        : null;
    elements.progress.hidden = false;
    if (percentage !== null) {
      elements.progress.value = Math.min(100, percentage);
      setBadge('waiting', `Chargement ${Math.round(percentage)} %`);
    } else {
      elements.progress.removeAttribute('value');
      setBadge('waiting', 'Chargement local');
    }
  }

  function rejectWorkerRequest(error) {
    if (!state.workerRequest) return;
    state.workerRequest.reject(error);
    state.workerRequest = null;
  }

  function ensureWorker() {
    if (state.worker) return state.worker;
    const worker = new Worker(new URL('future-lab-ai-worker.js?v=1.1.0', import.meta.url), {
      type: 'module'
    });
    worker.addEventListener('message', event => {
      const message = event.data || {};
      if (message.type === 'progress') {
        updateProgress(message.progress);
        return;
      }
      if (message.type === 'ready') {
        state.modelLoaded = true;
        state.loading = false;
        elements.progress.hidden = true;
        elements.panel.dataset.aiState = state.running ? 'running' : 'ready';
        setBadge('valid', `Modèle chargé · ${String(message.device || 'local').toUpperCase()}`);
        announce('Le modèle local est chargé et prêt.');
        updateControls();
        return;
      }
      if (message.type === 'result' && state.workerRequest) {
        const request = state.workerRequest;
        state.workerRequest = null;
        request.resolve(message.text || '');
        return;
      }
      if (message.type === 'error') {
        rejectWorkerRequest(new Error(message.message || 'Erreur du moteur local.'));
      }
    });
    worker.addEventListener('error', event => {
      rejectWorkerRequest(new Error(event.message || 'Le moteur local s’est arrêté.'));
      worker.terminate();
      state.worker = null;
      state.modelLoaded = false;
      state.loading = false;
      updateControls();
    });
    state.worker = worker;
    return worker;
  }

  function generateInWorker(messages) {
    if (state.workerRequest) return Promise.reject(new Error('Une analyse locale est déjà en cours.'));
    return new Promise((resolve, reject) => {
      state.workerRequest = { resolve, reject };
      ensureWorker().postMessage({ type: 'generate', messages });
    });
  }

  async function runAssistant() {
    if (!elements.consent.checked || !state.telemetry || state.running) return;
    const telemetry = Object.freeze({ ...state.telemetry });
    const facts = buildAssistantFacts(telemetry);
    const messages = buildModelMessages(telemetry, elements.prompt.value);
    state.factsLocked = true;
    renderFacts(telemetry);
    const sampledAt = finite(telemetry.sampledAt);
    elements.factsTitle.textContent = sampledAt === null
      ? 'Faits utilisés pour cette analyse'
      : `Faits utilisés · ${new Intl.DateTimeFormat('fr-FR', { timeStyle: 'medium' }).format(new Date(sampledAt))}`;
    state.running = true;
    state.loading = !state.modelLoaded;
    elements.panel.dataset.aiState = 'running';
    elements.progress.hidden = state.modelLoaded;
    if (!state.modelLoaded) elements.progress.value = 0;
    elements.summary.textContent = state.modelLoaded
      ? 'Le copilote local prépare une reformulation qualitative…'
      : 'Chargement du modèle quantifié depuis le disque local…';
    announce(state.modelLoaded ? 'Génération locale en cours.' : 'Chargement du modèle local en cours.');
    updateControls();
    try {
      const generated = await generateInWorker(messages);
      const answer = validateModelAnswer(generated, facts);
      elements.summary.textContent = `${answer}\n\nReformulation locale expérimentale — vérifiez toujours les cartes ci-dessus.`;
      elements.panel.dataset.aiState = 'ready';
      setBadge('valid', 'Analyse locale terminée');
      announce('La reformulation locale est terminée.');
    } catch (error) {
      elements.panel.dataset.aiState = 'error';
      elements.summary.textContent = `${error.message || 'Le copilote local n’a pas pu produire de réponse fiable.'}\n\nLa réponse a été écartée : utilisez la lecture factuelle ci-dessus.`;
      setBadge('unavailable', 'Réponse écartée');
      announce('La réponse du modèle a été écartée.');
    } finally {
      state.running = false;
      state.loading = false;
      updateControls();
    }
  }

  document.addEventListener('linux-doctor:future-lab-telemetry', event => {
    state.telemetry = event.detail;
    if (!state.factsLocked) renderFacts();
    updateControls();
  });
  elements.consent.addEventListener('change', updateControls);
  elements.check.addEventListener('click', checkAssets);
  elements.run.addEventListener('click', runAssistant);
  window.addEventListener('pagehide', () => {
    state.worker?.postMessage({ type: 'dispose' });
    state.worker?.terminate();
    state.worker = null;
  });

  renderFacts();
  updateControls();
  checkAssets();
}

if (typeof document !== 'undefined') initializeAssistant();
