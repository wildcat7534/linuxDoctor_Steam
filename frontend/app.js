const dialog = document.querySelector('#insight-dialog');
const scoreRing = document.querySelector('#score-ring');
const analyzeButton = document.querySelector('#analyze');
const progressBar = document.querySelector('#analysis-progress');
const categoriesTarget = document.querySelector('#categories');
const diagnosticsTarget = document.querySelector('#diagnostics');
const diagnosticsStatus = document.querySelector('#diagnostics-status');
const goodNewsTarget = document.querySelector('#good-news');
const historyPanel = document.querySelector('#history');
const greetingTarget = document.querySelector('#greeting');
const lastAnalysisTarget = document.querySelector('#last-analysis');
const overviewTarget = document.querySelector('#overview');
const countsTarget = document.querySelector('#counts');
const selectedCategoryTitle = document.querySelector('#selected-category-title');
const selectedCategorySummary = document.querySelector('#selected-category-summary');
const historyMessage = document.querySelector('#history-message');
const historyValues = document.querySelector('#history-values');
const previousMarker = document.querySelector('#previous-marker');
const currentMarker = document.querySelector('#current-marker');
const historyChanges = document.querySelector('#history-changes');
const dialogMode = document.querySelector('#dialog-mode');
const dialogTitle = document.querySelector('#dialog-title');
const dialogSummary = document.querySelector('#dialog-summary');
const dialogObserved = document.querySelector('#dialog-observed');
const dialogWhy = document.querySelector('#dialog-why');
const dialogImpact = document.querySelector('#dialog-impact');
const dialogNextStep = document.querySelector('#dialog-next-step');
const dialogActionsList = document.querySelector('#dialog-actions-list');
const electricLink = document.querySelector('#electric-link');
const scrollTopButton = document.querySelector('#scroll-top');
const APT_REFRESH_COMMAND = './scripts/refresh-updates.sh';
const APT_UPDATE_STATES = new Set(['ready', 'phased', 'deferred', 'unknown']);
const CATEGORY_ICONS = { storage: '💾', gaming: '🎮', graphics: '⚡', updates: '📦', apps: '🛠️' };
const KNOWLEDGE_ICONS = { game: '🕹️', steam: '♨️', controller: '🎮', gfn: '☁️', ubuntu: '🐧' };
const CONTROLLER_BRANDS = {
  steam: { label: 'Steam / Valve', mark: 'STEAM' },
  xbox: { label: 'Xbox', mark: 'X' },
  playstation: { label: 'PlayStation', mark: 'PS' },
  nintendo: { label: 'Nintendo', mark: 'N' },
  '8bitdo': { label: '8BitDo', mark: '8B' },
  generic: { label: 'Manette générique', mark: '🎮' }
};

const requestedCategoryId = decodeURIComponent(window.location.hash.slice(1));
let selectedCategoryId = /^[a-z0-9_-]+$/.test(requestedCategoryId) ? requestedCategoryId : null;
let analysisTimer = null;

function setText(target, value) {
  if (target) target.textContent = value;
}

function statusClass(severity) {
  return `status-${severity || 'unknown'}`;
}

function statusLabel(severity) {
  return { ok: 'OK', info: 'Info', warning: 'À surveiller', problem: 'Problème', unknown: 'Indisponible' }[severity] || 'Indisponible';
}

function formatCount(count, singular, plural) {
  return `${count} ${count === 1 ? singular : plural}`;
}

function formatBytes(bytes) {
  if (!Number.isFinite(bytes)) return '';
  const units = ['o', 'Ko', 'Mo', 'Go', 'To'];
  let value = bytes;
  let unit = 0;
  while (value >= 1024 && unit < units.length - 1) {
    value /= 1024;
    unit += 1;
  }
  return `${value >= 10 || unit === 0 ? Math.round(value) : value.toFixed(1)} ${units[unit]}`;
}

function evidenceText(item) {
  const value = item.value || formatBytes(item.bytes);
  return value || item.detail
    ? `${item.label} : ${[value, item.detail].filter(Boolean).join(' · ')}`
    : item.label;
}

function physicalDiskKey(volume) {
  if (volume.parent_path) return volume.parent_path;
  return volume.path.replace(/p?\d+$/, '');
}

function aptUpdateState(update, selectionAvailable) {
  if (update.held === true) {
    return { label: 'Retenue manuellement', className: 'status-unknown' };
  }
  if (!selectionAvailable || !APT_UPDATE_STATES.has(update.state) || update.state === 'unknown') {
    return { label: 'État non confirmé', className: 'status-unknown' };
  }
  if (update.state === 'ready') {
    return { label: 'Prête selon APT', className: 'status-info' };
  }
  if (update.state === 'phased') {
    const percentage = Number.isInteger(update.phased_percentage) && update.phased_percentage >= 0 && update.phased_percentage <= 100
      ? ` · ${update.phased_percentage} %` : '';
    return { label: `Déploiement progressif${percentage}`, className: 'status-info' };
  }
  if (update.state === 'deferred') {
    return { label: 'Différée par APT', className: 'status-unknown' };
  }
  return { label: 'État non confirmé', className: 'status-unknown' };
}

function legacyCopyText(value) {
  const field = document.createElement('textarea');
  field.value = value;
  field.readOnly = true;
  field.className = 'clipboard-fallback';
  document.body.appendChild(field);
  field.focus();
  field.select();
  field.setSelectionRange(0, value.length);
  let copied = false;
  try {
    copied = document.execCommand('copy');
  } catch (_error) {
    copied = false;
  }
  field.remove();
  return copied;
}

async function copyText(value, status) {
  let copied = false;
  if (window.isSecureContext && navigator.clipboard?.writeText) {
    try {
      await navigator.clipboard.writeText(value);
      copied = true;
    } catch (_error) {
      copied = false;
    }
  }
  if (!copied) copied = legacyCopyText(value);
  status.textContent = copied
    ? '✓ Commande copiée dans le presse-papiers.'
    : 'Sélectionnez la commande affichée puis utilisez Ctrl+C.';
}

function validateUpdatesInventory(inventory) {
  if (inventory === undefined) return;
  if (!inventory || typeof inventory !== 'object' || Array.isArray(inventory)) {
    throw new Error('Le bloc des mises à jour APT est invalide.');
  }
  ['cache_available', 'inventory_available', 'selection_available', 'hold_information_available',
    'metadata_available', 'truncated'].forEach(field => {
    if (typeof inventory[field] !== 'boolean') throw new Error(`Champ APT invalide : ${field}.`);
  });
  if (!Array.isArray(inventory.packages)) {
    throw new Error('La liste des paquets APT est invalide.');
  }
  if (!inventory.counts || typeof inventory.counts !== 'object' || Array.isArray(inventory.counts)) {
    throw new Error('Les compteurs APT sont invalides.');
  }
  ['candidates', 'ready', 'phased', 'deferred', 'unknown', 'security', 'held', 'with_metadata'].forEach(field => {
    if (!Number.isInteger(inventory.counts[field]) || inventory.counts[field] < 0) {
      throw new Error(`Compteur APT invalide : ${field}.`);
    }
  });
  if (inventory.counts.candidates !== inventory.packages.length ||
      inventory.counts.ready + inventory.counts.phased + inventory.counts.deferred + inventory.counts.unknown !== inventory.counts.candidates ||
      inventory.counts.security > inventory.counts.candidates || inventory.counts.held > inventory.counts.candidates ||
      inventory.counts.with_metadata > inventory.counts.candidates) {
    throw new Error('Les compteurs APT ne correspondent pas à la liste des paquets.');
  }
  if ((inventory.cache_available && (!Number.isInteger(inventory.cache_age_days) || inventory.cache_age_days < 0)) ||
      (!inventory.cache_available && inventory.cache_age_days !== null)) {
    throw new Error('L’âge du cache APT est invalide.');
  }
  inventory.packages.forEach(update => {
    if (!update || typeof update !== 'object' || Array.isArray(update) ||
        typeof update.security_origin !== 'boolean' || typeof update.held !== 'boolean' ||
        typeof update.metadata_available !== 'boolean' || typeof update.state !== 'string') {
      throw new Error('Une entrée de mise à jour APT est invalide.');
    }
    if (update.phased_percentage !== null &&
        (!Number.isInteger(update.phased_percentage) || update.phased_percentage < 0 || update.phased_percentage > 100)) {
      throw new Error('Un pourcentage de déploiement APT est invalide.');
    }
  });
}

function safeHttpsUrl(value) {
  try {
    const url = new URL(value);
    return url.protocol === 'https:' ? url.href : null;
  } catch (_error) {
    return null;
  }
}

function safeImageDataUri(value) {
  return typeof value === 'string' && /^data:image\/(?:jpeg|png);base64,[a-z0-9+/=]+$/i.test(value)
    ? value : null;
}

function categoryIcon(category) {
  return category.icon || CATEGORY_ICONS[category.id] || '🔎';
}

function controllerBrand(kind) {
  return CONTROLLER_BRANDS[kind] || CONTROLLER_BRANDS.generic;
}

function createControllerMark(controller) {
  const kind = CONTROLLER_BRANDS[controller.kind] ? controller.kind : 'generic';
  const mark = document.createElement('span');
  mark.className = `controller-mark kind-${kind}`;
  mark.setAttribute('aria-hidden', 'true');
  if (kind === 'steam') {
    const image = document.createElement('img');
    image.src = 'assets/steam.svg';
    image.alt = '';
    mark.appendChild(image);
  } else {
    mark.textContent = controllerBrand(kind).mark;
  }
  return mark;
}

function usageBar(usedPercent, label) {
  const bar = document.createElement('div');
  bar.className = `capacity-bar ${usedPercent >= 95 ? 'critical' : usedPercent >= 85 ? 'warning' : ''}`;
  const fill = document.createElement('i');
  fill.style.width = `${Math.max(0, Math.min(100, usedPercent))}%`;
  bar.append(fill);
  const text = document.createElement('small');
  text.className = 'muted';
  text.textContent = label;
  const group = document.createElement('div');
  group.className = 'capacity';
  group.append(bar, text);
  return group;
}

function renderEnergyEstimate(cards) {
  const panel = document.createElement('article');
  panel.className = 'energy-panel';
  const title = document.createElement('h3');
  title.className = 'energy-title';
  const titlePrefix = document.createElement('span');
  titlePrefix.textContent = 'Comparatif énergie sur';
  const titleDuration = document.createElement('strong');
  titleDuration.textContent = '100 h de jeu';
  title.append(titlePrefix, titleDuration);
  const baseline = document.createElement('p');
  baseline.className = 'energy-baseline';
  baseline.textContent = 'La même durée est appliquée aux trois profils pour rendre la comparaison immédiate.';
  const controls = document.createElement('div');
  controls.className = 'energy-controls';
  const rateLabel = document.createElement('label');
  rateLabel.textContent = 'Tarif du pays / contrat (€ / kWh)';
  const rate = document.createElement('input');
  rate.type = 'number';
  rate.min = '0';
  rate.step = '0.001';
  rate.value = '0.194';
  rate.setAttribute('aria-label', 'Tarif électricité en euros par kWh');
  rateLabel.appendChild(rate);
  const hoursLabel = document.createElement('label');
  hoursLabel.textContent = 'Heures de jeu';
  const hours = document.createElement('input');
  hours.type = 'number';
  hours.min = '1';
  hours.step = '1';
  hours.value = '100';
  hoursLabel.appendChild(hours);
  controls.append(rateLabel, hoursLabel);
  const estimates = document.createElement('div');
  estimates.className = 'energy-estimates';
  const profiles = [
    { id: 'gfn', icon: '☁️', name: 'GeForce NOW', watts: 110, detail: 'PC en décodage + écran' },
    { id: 'mid', icon: '🖥️', name: 'PC gamer moyen', watts: 420, detail: 'RTX 5070 / Ryzen 5 + 32 Go + écran 27″ 120 Hz' },
    { id: 'uber', icon: '🚀', name: 'Uber PC', watts: 620, detail: 'RTX 4080 Super / CPU haut de gamme + 32 Go DDR5 + écran 27″ 120 Hz' }
  ];
  const refresh = () => {
    const price = Number(rate.value) || 0;
    const duration = Number(hours.value) || 0;
    estimates.replaceChildren(...profiles.map(profile => {
      const card = document.createElement('section');
      card.className = `energy-profile ${profile.id}`;
      const kwh = profile.watts * duration / 1000;
      const name = document.createElement('strong');
      name.textContent = `${profile.icon} ${profile.name}`;
      const result = document.createElement('b');
      result.textContent = `${kwh.toFixed(1)} kWh · ${(kwh * price).toFixed(2)} €`;
      const detail = document.createElement('small');
      detail.textContent = `${profile.watts} W moyens supposés · ${profile.detail}`;
      card.append(name, result, detail);
      return card;
    }));
  };
  rate.addEventListener('input', refresh);
  hours.addEventListener('input', refresh);
  refresh();
  const source = document.createElement('small');
  source.className = 'muted';
  source.textContent = 'Référence France : 0,194 €/kWh TTC (Tarif Bleu Base, février 2026). Modifiez ce tarif selon votre pays ou contrat.';
  const membership = document.createElement('p');
  membership.className = 'membership-cost';
  membership.textContent = 'GeForce NOW Ultimate : 219,98 € / 12 mois hors promotion, soit 18,33 € / mois. L’abonnement et l’électricité sont deux coûts distincts.';
  panel.append(title, baseline, controls, estimates, membership, source);
  cards.push(panel);
}

function validateReport(report) {
  if (!report || typeof report !== 'object') {
    throw new Error('Le rapport Linux Doctor est vide ou invalide.');
  }
  if (report.schema_version !== 2) {
    throw new Error(`Version de rapport non prise en charge : ${report.schema_version ?? 'absente'}.`);
  }
  if (!report.system_health || !Number.isFinite(report.system_health.score)) {
    throw new Error('Le rapport ne contient pas de score système exploitable.');
  }
  if (!Array.isArray(report.categories)) {
    throw new Error('Le rapport ne contient pas de catégories exploitables.');
  }
  validateUpdatesInventory(report.updates_inventory);
  return report;
}

function normalizeReport(report) {
  const diagnostics = report.categories?.flatMap(category => category.diagnostics || []) || [];
  const problems = diagnostics.filter(item => item.severity === 'problem').length;
  const warnings = diagnostics.filter(item => item.severity === 'warning').length;
  const ok = diagnostics.filter(item => item.severity === 'ok').length;
  const unknown = diagnostics.filter(item => item.severity === 'unknown' || !item.severity).length;
  const suppliedSummary = report.summary || {};
  const suppliedCounts = suppliedSummary.counts || {};
  const overview = problems > 0
    ? 'Un point prioritaire mérite votre attention.'
    : warnings > 0
      ? 'Votre machine fonctionne, avec quelques points à surveiller.'
      : unknown > 0
        ? 'Certaines vérifications sont indisponibles ; aucun état sain global ne peut être déduit.'
        : 'Votre machine est globalement en bon état.';

  return {
    ...report,
    summary: {
      greeting: suppliedSummary.greeting || 'Bonjour',
      last_analysis_date: suppliedSummary.last_analysis_date || report.generated_at || '',
      overview: suppliedSummary.overview || overview,
      counts: {
        problem: Number.isFinite(suppliedCounts.problem) ? suppliedCounts.problem : problems,
        warning: Number.isFinite(suppliedCounts.warning) ? suppliedCounts.warning : warnings,
        ok: Number.isFinite(suppliedCounts.ok) ? suppliedCounts.ok : ok,
        unknown: Number.isFinite(suppliedCounts.unknown) ? suppliedCounts.unknown : unknown
      }
    },
    good_news: report.good_news || [],
    categories: report.categories || []
  };
}

function formatAnalysisDate(value) {
  if (!value) return 'inconnue';
  const date = new Date(value);
  if (Number.isNaN(date.getTime())) return value;
  return new Intl.DateTimeFormat('fr-FR', {
    dateStyle: 'long',
    timeStyle: 'short'
  }).format(date);
}

function openInsight(diagnostic, mode) {
  const actions = diagnostic.recommendations || [];
  setText(dialogMode, mode);
  setText(dialogTitle, diagnostic.title);
  setText(dialogSummary, diagnostic.summary || '');
  setText(dialogObserved, diagnostic.explanation?.observed || '');
  setText(dialogWhy, diagnostic.explanation?.why || '');
  setText(dialogImpact, diagnostic.explanation?.impact || '');
  setText(dialogNextStep, diagnostic.explanation?.next_step || '');
  dialogActionsList?.replaceChildren(...actions.map(item => {
    const li = document.createElement('li');
    li.textContent = `${item.label}${item.priority ? ` — priorité ${item.priority}` : ''}`;
    return li;
  }));
  dialog.showModal();
}

function renderCounts(summary, goodNewsCount) {
  const counts = [
    { label: `⚠ ${formatCount(summary.counts.warning, 'avertissement', 'avertissements')}`, className: 'status-warning' },
    { label: `⛔ ${formatCount(summary.counts.problem, 'problème', 'problèmes')}`, className: 'status-problem' },
    { label: `✓ ${formatCount(summary.counts.ok, 'vérification réussie', 'vérifications réussies')}`, className: 'status-ok' },
    { label: `? ${formatCount(summary.counts.unknown, 'vérification indisponible', 'vérifications indisponibles')}`, className: 'status-unknown' },
    { label: `◎ ${formatCount(goodNewsCount, 'bonne nouvelle', 'bonnes nouvelles')}`, className: 'status-info' }
  ];
  countsTarget?.replaceChildren(...counts.map(item => {
    const span = document.createElement('span');
    span.className = item.className;
    span.textContent = item.label;
    return span;
  }));
}

function renderCategories(report) {
  const categories = report.categories || [];
  if (!selectedCategoryId || !categories.some(category => category.id === selectedCategoryId)) {
    selectedCategoryId = categories[0] ? categories[0].id : null;
  }
  categoriesTarget.replaceChildren(...categories.map(category => {
    const button = document.createElement('button');
    button.type = 'button';
    button.id = `category-tab-${category.id}`;
    button.setAttribute('role', 'tab');
    button.setAttribute('aria-controls', 'diagnostics');
    button.setAttribute('aria-selected', String(selectedCategoryId === category.id));
    button.tabIndex = selectedCategoryId === category.id ? 0 : -1;
    button.className = `category-card ${selectedCategoryId === category.id ? 'selected' : ''}`;
    button.addEventListener('click', () => {
      selectedCategoryId = category.id;
      window.history.replaceState(null, '', `#${category.id}`);
      render(report);
      document.querySelector(`#category-tab-${CSS.escape(category.id)}`)?.focus();
    });
    button.addEventListener('keydown', event => {
      const current = categories.findIndex(item => item.id === category.id);
      let next = current;
      if (event.key === 'ArrowRight' || event.key === 'ArrowDown') next = (current + 1) % categories.length;
      else if (event.key === 'ArrowLeft' || event.key === 'ArrowUp') next = (current - 1 + categories.length) % categories.length;
      else if (event.key === 'Home') next = 0;
      else if (event.key === 'End') next = categories.length - 1;
      else return;
      event.preventDefault();
      selectedCategoryId = categories[next].id;
      window.history.replaceState(null, '', `#${selectedCategoryId}`);
      render(report);
      document.querySelector(`#category-tab-${CSS.escape(selectedCategoryId)}`)?.focus();
    });

    const top = document.createElement('div');
    top.className = 'top';
    const title = document.createElement('strong');
    title.textContent = `${categoryIcon(category)} ${category.name}`;
    const score = document.createElement('span');
    score.className = 'score-pill';
    const scoreAvailable = category.status !== 'unknown';
    score.textContent = scoreAvailable ? `${category.score}%` : '—';
    score.title = category.score_explanation || (scoreAvailable ? `Score ${category.score} sur 100` : 'Score indisponible');
    score.setAttribute('aria-label', `${scoreAvailable ? `${category.score} sur 100` : 'Score indisponible'}. ${category.score_explanation || ''}`.trim());
    top.append(title, score);

    const summary = document.createElement('p');
    summary.className = 'muted';
    summary.textContent = category.summary || category.diagnostics?.[0]?.title || 'Aucun diagnostic détaillé.';

    const chip = document.createElement('span');
    chip.className = `chip ${statusClass(category.status)}`;
    chip.textContent = statusLabel(category.status);

    button.append(top, summary, chip);
    return button;
  }));
  const activeTab = categoriesTarget.querySelector('.selected');
  if (activeTab && electricLink) {
    const grid = categoriesTarget.getBoundingClientRect();
    const tab = activeTab.getBoundingClientRect();
    electricLink.style.setProperty('--wire-x', `${tab.left - grid.left + tab.width / 2}px`);
  }
}

function categorySnapshotFacts(category, report) {
  if (category.id === 'gaming') {
    const steam = report.steam_inventory || {};
    const gfn = report.gfn_inventory || {};
    return [
      { icon: '🎮', value: String(steam.games?.length || 0), label: 'jeux Steam détectés' },
      { icon: '🗂️', value: String(steam.libraries?.length || 0), label: 'bibliothèques locales' },
      { icon: '🕹️', value: String(steam.controller_count || steam.controllers?.length || 0), label: 'manettes détectées' },
      { icon: '☁️', value: gfn.installed ? 'Installé' : 'Non détecté', label: 'GeForce NOW' }
    ];
  }
  if (category.id === 'updates') {
    const counts = report.updates_inventory?.counts || {};
    return [
      { icon: '📦', value: String(counts.candidates || 0), label: 'mises à jour candidates' },
      { icon: '✅', value: String(counts.ready || 0), label: 'prêtes selon APT' },
      { icon: '🌊', value: String(counts.phased || 0), label: 'déploiements progressifs' },
      { icon: '🔐', value: String(counts.security || 0), label: 'dépôts de sécurité' }
    ];
  }
  if (category.id === 'storage') {
    const volumes = report.storage_inventory?.volumes || [];
    return [
      { icon: '💽', value: String(volumes.length), label: 'volumes détectés' },
      { icon: '✅', value: String(volumes.filter(volume => volume.mounted).length), label: 'volumes montés' },
      { icon: '🪟', value: String(volumes.filter(volume => volume.windows_protected).length), label: 'volumes Windows protégés' },
      { icon: '👀', value: 'Lecture seule', label: 'mode de diagnostic' }
    ];
  }
  if (category.id === 'graphics') {
    const graphics = report.graphics_inventory || {};
    return [
      { icon: '⚡', value: String(graphics.devices?.length || 0), label: 'GPU détectés' },
      { icon: '🧩', value: graphics.vulkan?.loader_available ? 'Présent' : 'À vérifier', label: 'chargeur Vulkan' },
      { icon: '🎨', value: graphics.opengl?.loader_available ? 'Présent' : 'À vérifier', label: 'chargeur OpenGL' },
      { icon: '🖼️', value: graphics.session?.type || 'Inconnue', label: 'session graphique' }
    ];
  }
  if (category.id === 'apps') {
    const apps = report.apps_inventory || {};
    return [
      { icon: '🧰', value: String(apps.recommended?.length || 0), label: 'outils conseillés' },
      { icon: '✅', value: String((apps.recommended || []).filter(app => app.installed).length), label: 'déjà installés' },
      { icon: '🙌', value: 'Facultatif', label: 'installation automatique' },
      { icon: '🔒', value: 'Locale', label: 'source des vérifications' }
    ];
  }
  return [
    { icon: '🔎', value: statusLabel(category.status), label: 'état du domaine' },
    { icon: '🧾', value: String(category.diagnostics?.length || 0), label: 'vérifications disponibles' }
  ];
}

function renderCategorySnapshot(category, report) {
  const snapshot = document.createElement('article');
  snapshot.className = `category-snapshot ${statusClass(category.status)}`;
  const heading = document.createElement('div');
  heading.className = 'snapshot-heading';
  const mascot = document.createElement('span');
  mascot.className = 'snapshot-mascot';
  mascot.setAttribute('aria-hidden', 'true');
  mascot.textContent = category.status === 'problem' ? '😿' : category.status === 'warning' ? '🧐' : category.status === 'unknown' ? '🤔' : '😺';
  const copy = document.createElement('div');
  const title = document.createElement('h3');
  title.textContent = `Le petit bilan ${categoryIcon(category)}`;
  const summary = document.createElement('p');
  summary.textContent = category.summary || 'Les informations essentielles sont regroupées ici avant les détails.';
  copy.append(title, summary);
  const chip = document.createElement('span');
  chip.className = `status-chip ${statusClass(category.status)}`;
  chip.textContent = statusLabel(category.status);
  heading.append(mascot, copy, chip);
  const facts = document.createElement('div');
  facts.className = 'snapshot-facts';
  categorySnapshotFacts(category, report).forEach(item => {
    const fact = document.createElement('section');
    const icon = document.createElement('span');
    icon.className = 'snapshot-icon';
    icon.setAttribute('aria-hidden', 'true');
    icon.textContent = item.icon;
    const value = document.createElement('strong');
    value.textContent = item.value;
    const label = document.createElement('small');
    label.textContent = item.label;
    fact.append(icon, value, label);
    facts.appendChild(fact);
  });
  const scoreExplanation = document.createElement('section');
  scoreExplanation.className = 'score-explanation';
  const scoreTitle = document.createElement('strong');
  scoreTitle.textContent = category.status === 'unknown'
    ? '💡 Pourquoi le score est-il indisponible ?' : `💡 Pourquoi ${category.score} % ?`;
  const scoreCopy = document.createElement('p');
  scoreCopy.textContent = category.score_explanation || 'Le rapport ne fournit pas encore le détail de ce score.';
  scoreExplanation.append(scoreTitle, scoreCopy);
  snapshot.append(heading, facts, scoreExplanation);
  return snapshot;
}

function renderDiagnostics(report) {
  const category = (report.categories || []).find(item => item.id === selectedCategoryId) || report.categories?.[0];
  if (!category) {
    setText(selectedCategoryTitle, 'Diagnostics');
    setText(selectedCategorySummary, 'Aucune catégorie disponible.');
    diagnosticsTarget.textContent = 'Aucune donnée.';
    diagnosticsTarget.removeAttribute('aria-labelledby');
    setText(diagnosticsStatus, 'Aucune catégorie de diagnostic disponible.');
    return;
  }

  setText(selectedCategoryTitle, `${categoryIcon(category)} ${category.name}`);
  setText(selectedCategorySummary, category.summary || '');
  diagnosticsTarget.setAttribute('aria-labelledby', `category-tab-${category.id}`);
  setText(diagnosticsStatus, `Catégorie ${category.name} affichée.`);
  const cards = [renderCategorySnapshot(category, report)];
  if (category.id === 'storage') {
    const groups = new Map();
    (report.storage_inventory?.volumes || []).forEach(volume => {
      const key = physicalDiskKey(volume);
      groups.set(key, [...(groups.get(key) || []), volume]);
    });
    if (groups.size) {
      const overview = document.createElement('section');
      overview.className = 'storage-overview';
      const title = document.createElement('h3');
      title.textContent = 'Vue d’ensemble des disques';
      const hint = document.createElement('p');
      hint.className = 'muted';
      hint.textContent = 'Cliquez un disque pour atteindre ses partitions et son état.';
      const navigator = document.createElement('div');
      navigator.className = 'disk-navigator';
      groups.forEach((volumes, disk) => {
        const mounted = volumes.filter(volume => volume.mounted);
        const used = mounted.length ? Math.round(mounted.reduce((sum, volume) => sum + volume.used_percent, 0) / mounted.length) : 0;
        const id = `disk-${disk.replace(/[^a-z0-9]/gi, '')}`;
        const button = document.createElement('button');
        button.type = 'button';
        button.className = 'disk-shortcut';
        button.addEventListener('click', () => {
          const target = document.querySelector(`#${id}`);
          if (target instanceof HTMLDetailsElement) target.open = true;
          target?.scrollIntoView({ behavior: 'smooth', block: 'start' });
        });
        const ring = document.createElement('span');
        ring.className = `disk-ring ${volumes.some(volume => volume.windows_protected) ? 'protected' : ''}`;
        ring.style.setProperty('--used', `${used}%`);
        ring.textContent = mounted.length ? `${used}%` : '—';
        const label = document.createElement('strong');
        label.textContent = disk;
        const state = document.createElement('small');
        state.textContent = volumes.some(volume => volume.windows_protected) ? 'Windows protégé' : `${mounted.length}/${volumes.length} monté(s)`;
        button.append(ring, label, state);
        navigator.appendChild(button);
      });
      overview.append(title, hint, navigator);
      cards.push(overview);
    }
    groups.forEach((volumes, disk) => {
      const group = document.createElement('details');
      group.className = 'volume-group';
      group.id = `disk-${disk.replace(/[^a-z0-9]/gi, '')}`;
      const heading = document.createElement('summary');
      heading.className = 'volume-group-summary';
      const headingTitle = document.createElement('strong');
      headingTitle.textContent = `💽 Disque physique ${disk || 'inconnu'}`;
      const headingState = document.createElement('small');
      const mountedCount = volumes.filter(volume => volume.mounted).length;
      headingState.textContent = `${volumes.length} partition${volumes.length > 1 ? 's' : ''} · ${mountedCount} montée${mountedCount > 1 ? 's' : ''}`;
      heading.append(headingTitle, headingState);
      group.appendChild(heading);
      if (volumes.some(volume => volume.windows_protected)) {
        const warning = document.createElement('p');
        warning.className = 'dualboot-warning';
        warning.textContent = '⚠ Dual boot Windows probable : ce disque contient des partitions système et données Windows. Ne pas effacer, reformater ni choisir comme destination automatique.';
        group.appendChild(warning);
      }
      const groupCards = document.createElement('div');
      groupCards.className = 'diagnostic-list';
      volumes.forEach((volume, partitionIndex) => {
      const card = document.createElement('article');
      card.className = `diagnostic-card volume-card ${volume.mounted && !volume.read_only ? 'mounted' : 'unmounted'}`;
      const head = document.createElement('div');
      head.className = 'diagnostic-head';
      const title = document.createElement('h3');
      title.textContent = volume.label || volume.path;
      const state = document.createElement('span');
      state.className = 'status-chip status-info';
      state.textContent = volume.mounted ? volume.read_only ? 'Lecture seule' : 'Monté' : 'Non monté';
      head.append(title, state);
      const evidence = document.createElement('div');
      evidence.className = 'evidence';
      [
        { label: 'Disque physique', value: `${disk} · partition ${partitionIndex + 1}/${volumes.length}` },
        { label: 'Périphérique', value: volume.path },
        { label: 'Système de fichiers', value: volume.filesystem || 'Inconnu' },
        { label: 'Montage', value: volume.mountpoint || 'Aucun' },
        { label: 'Espace libre', bytes: volume.available_bytes },
        { label: 'Utilisation', value: volume.mounted ? `${volume.used_percent} %` : 'Non mesurable' },
        { label: 'UUID', value: volume.uuid || 'Indisponible' }
      ].forEach(item => {
        const pill = document.createElement('span');
        pill.textContent = evidenceText(item);
        evidence.appendChild(pill);
      });
      const stateLabel = volume.mounted ? `${volume.used_percent} % utilisés · ${formatBytes(volume.available_bytes)} libres` : 'Non monté : espace libre non mesurable';
      card.append(head, evidence, usageBar(volume.mounted ? volume.used_percent : 0, stateLabel));
      groupCards.appendChild(card);
    });
      group.appendChild(groupCards);
      cards.push(group);
    });
  }
  if (category.id === 'gaming') {
    const inventory = report.steam_inventory || {};
    const controllers = Array.isArray(inventory.controllers) ? [...inventory.controllers] : [];
    if (controllers.length === 0 && inventory.controller_detected) {
      controllers.push({ name: inventory.controller_name || 'Steam Controller', kind: 'steam' });
    }
    const controllerPanel = document.createElement('article');
    controllerPanel.className = `controller-panel ${controllers.length ? 'status-ok' : 'status-info'}`;
    const controllerHead = document.createElement('div');
    controllerHead.className = 'diagnostic-head';
    const controllerTitle = document.createElement('h3');
    controllerTitle.textContent = '🎮 Manettes et Steam Input';
    const controllerChip = document.createElement('span');
    controllerChip.className = `status-chip ${controllers.length ? 'status-ok' : 'status-info'}`;
    controllerChip.textContent = controllers.length
      ? `${controllers.length} détectée${controllers.length > 1 ? 's' : ''}` : 'Aucune connectée';
    controllerHead.append(controllerTitle, controllerChip);
    const controllerIntro = document.createElement('p');
    controllerIntro.className = 'muted';
    controllerIntro.textContent = controllers.length
      ? 'Voici les périphériques reconnus localement par le noyau. Le test Steam Input reste recommandé dans Steam.'
      : 'Connectez une manette puis rechargez le rapport pour afficher sa famille et son identité.';
    controllerPanel.append(controllerHead, controllerIntro);
    if (controllers.length) {
      const controllerGrid = document.createElement('div');
      controllerGrid.className = 'controller-grid';
      controllers.forEach(controller => {
        const item = document.createElement('section');
        item.className = `controller-device kind-${CONTROLLER_BRANDS[controller.kind] ? controller.kind : 'generic'}`;
        const copy = document.createElement('span');
        const name = document.createElement('strong');
        name.textContent = controller.name || 'Manette sans nom';
        const brand = document.createElement('small');
        brand.textContent = controllerBrand(controller.kind).label;
        copy.append(name, brand);
        item.append(createControllerMark(controller), copy);
        controllerGrid.appendChild(item);
      });
      controllerPanel.appendChild(controllerGrid);
    }
    cards.push(controllerPanel);
    (inventory.libraries || []).forEach((library, index) => {
      const card = document.createElement('article');
      card.className = `diagnostic-card ${library.mounted && library.writable ? 'status-ok' : 'status-warning'}`;
      const head = document.createElement('div');
      head.className = 'diagnostic-head';
      const title = document.createElement('h3');
      title.textContent = `Bibliothèque Steam ${index + 1}`;
      const state = document.createElement('span');
      state.className = 'status-chip status-info';
      state.textContent = library.mounted ? library.writable ? 'Disponible' : 'Lecture seule' : 'Indisponible';
      head.append(title, state);
      const evidence = document.createElement('div');
      evidence.className = 'evidence';
      [
        { label: 'Chemin', value: library.path },
        { label: 'Volume', value: library.volume_path || 'Non associé' },
        { label: 'Système de fichiers', value: library.filesystem || 'Inconnu' },
        { label: 'Espace libre', bytes: library.available_bytes },
        { label: 'Jeux installés', value: String(library.game_count) },
        { label: 'Jeux déclarés', bytes: library.game_bytes }
      ].forEach(item => {
        const pill = document.createElement('span');
        pill.textContent = evidenceText(item);
        evidence.appendChild(pill);
      });
      const games = (inventory.games || []).filter(game => game.library_index === index);
      if (games.length) {
        const details = document.createElement('details');
        details.className = 'steam-games-details';
        details.open = true;
        const summary = document.createElement('summary');
        summary.textContent = `🎮 Les ${games.length} jeux et leurs icônes`;
        const list = document.createElement('div');
        list.className = 'steam-game-grid';
        games.forEach(game => {
          const item = document.createElement('article');
          item.className = 'steam-game';
          const visual = document.createElement('span');
          visual.className = 'steam-game-icon';
          visual.textContent = '🕹️';
          const iconUri = safeImageDataUri(game.icon_data_uri);
          if (iconUri) {
            const image = document.createElement('img');
            image.src = iconUri;
            image.alt = '';
            image.addEventListener('error', () => image.remove());
            visual.appendChild(image);
          }
          const copy = document.createElement('span');
          const name = document.createElement('strong');
          name.textContent = game.name || `Jeu ${game.appid}`;
          const size = document.createElement('small');
          size.textContent = `${formatBytes(game.size_bytes)} · AppID ${game.appid}`;
          copy.append(name, size);
          item.append(visual, copy);
          list.appendChild(item);
        });
        details.append(summary, list);
        card.append(head, evidence, details);
      } else card.append(head, evidence);
      cards.push(card);
    });
    const knowledge = report.gaming_knowledge || {};
    const knowledgeCard = document.createElement('article');
    knowledgeCard.className = `gaming-knowledge-card ${knowledge.available ? 'status-info' : 'status-unknown'}`;
    const knowledgeHead = document.createElement('div');
    knowledgeHead.className = 'diagnostic-head';
    const knowledgeTitle = document.createElement('h3');
    knowledgeTitle.textContent = '📚 Guide local Ubuntu Gaming';
    const knowledgeChip = document.createElement('span');
    knowledgeChip.className = `status-chip ${knowledge.available ? 'status-info' : 'status-unknown'}`;
    knowledgeChip.textContent = knowledge.available ? `${knowledge.relevant_entries || 0} fiche(s) utile(s)` : 'Base indisponible';
    knowledgeHead.append(knowledgeTitle, knowledgeChip);
    const knowledgeSummary = document.createElement('p');
    const gameNotices = (knowledge.entries || []).filter(entry => entry.kind === 'game').length;
    knowledgeSummary.textContent = knowledge.available
      ? `${knowledge.total_entries || 0} fiches sont stockées uniquement dans l’application. ${gameNotices} ${gameNotices === 1 ? 'alerte spécifique correspond' : 'alertes spécifiques correspondent'} aux jeux installés. Aucun contrôle Internet n’est lancé.`
      : 'La base locale de conseils gaming n’a pas été chargée ; aucun problème connu ne peut être rapproché des jeux installés.';
    knowledgeCard.append(knowledgeHead, knowledgeSummary);
    if ((knowledge.entries || []).length) {
      const knowledgeDetails = document.createElement('details');
      knowledgeDetails.className = 'knowledge-details';
      const knowledgeToggle = document.createElement('summary');
      knowledgeToggle.textContent = 'Voir les conseils pertinents';
      const knowledgeList = document.createElement('div');
      knowledgeList.className = 'knowledge-list';
      knowledge.entries.forEach(entry => {
        const item = document.createElement('section');
        item.className = `knowledge-item ${statusClass(entry.severity)}`;
        const title = document.createElement('strong');
        title.textContent = `${KNOWLEDGE_ICONS[entry.kind] || '💡'} ${entry.title}`;
        const summary = document.createElement('p');
        summary.textContent = entry.summary;
        const guidance = document.createElement('p');
        guidance.className = 'muted';
        guidance.textContent = `Conseil : ${entry.guidance}`;
        const metadata = document.createElement('small');
        metadata.className = 'muted';
        metadata.textContent = `Fiche mise à jour le ${entry.updated_on}`;
        item.append(title, summary, guidance, metadata);
        const sourceUrl = safeHttpsUrl(entry.source_url);
        if (sourceUrl) {
          const source = document.createElement('a');
          source.href = sourceUrl;
          source.target = '_blank';
          source.rel = 'noreferrer';
          source.textContent = 'Source de la fiche';
          item.appendChild(source);
        }
        knowledgeList.appendChild(item);
      });
      knowledgeDetails.append(knowledgeToggle, knowledgeList);
      knowledgeCard.appendChild(knowledgeDetails);
    }
    cards.push(knowledgeCard);
    const plan = report.steam_migration_plan;
    if (plan?.available) {
      const card = document.createElement('article');
      card.className = `diagnostic-card migration-plan ${plan.selected_bytes < plan.target_free_bytes ? 'status-warning' : 'status-info'}`;
      const title = document.createElement('h3');
      title.textContent = 'Plan de migration simulé';
      const description = document.createElement('p');
      description.className = 'muted';
      description.textContent = `Destination proposée : ${plan.destination_path} (${formatBytes(plan.destination_available_bytes)} libres). Aucun fichier ne sera déplacé.`;
      const selection = document.createElement('div');
      selection.className = 'migration-selection';
      const selectionDetails = document.createElement('details');
      selectionDetails.className = 'migration-details';
      const selectionToggle = document.createElement('summary');
      selectionToggle.textContent = `Afficher et ajuster les ${plan.game_indexes?.length || 0} jeux proposés`;
      const summary = document.createElement('p');
      summary.className = 'migration-summary';
      const refresh = () => {
        const selected = [...selection.querySelectorAll('input:checked')]
          .reduce((total, input) => total + Number(input.dataset.size), 0);
        const shortfall = Math.max(0, Number(plan.target_free_bytes) - selected);
        summary.textContent = shortfall > 0
          ? `Plan partiel : ${formatBytes(selected)} sélectionnés · il manque ${formatBytes(shortfall)} pour atteindre l’objectif.`
          : `Objectif atteint : ${formatBytes(selected)} sélectionnés pour libérer au moins ${formatBytes(plan.target_free_bytes)}.`;
      };
      (plan.game_indexes || []).forEach(index => {
        const game = inventory.games?.[index];
        if (!game) return;
        const label = document.createElement('label');
        const checkbox = document.createElement('input');
        checkbox.type = 'checkbox';
        checkbox.checked = true;
        checkbox.dataset.size = String(game.size_bytes);
        checkbox.addEventListener('change', refresh);
        label.append(checkbox, document.createTextNode(` ${game.name} — ${formatBytes(game.size_bytes)}`));
        selection.appendChild(label);
      });
      refresh();
      const note = document.createElement('small');
      note.className = 'muted';
      note.textContent = 'Simulation V0.4 : ouvrez ensuite le gestionnaire de stockage Steam pour effectuer un déplacement contrôlé.';
      selectionDetails.append(selectionToggle, selection);
      card.append(title, description, summary, selectionDetails, note);
      cards.push(card);
    }
    renderEnergyEstimate(cards);
  }
  if (category.id === 'updates') {
    const inventory = report.updates_inventory || {};
    const packages = Array.isArray(inventory.packages) ? inventory.packages : [];
    const refreshCard = document.createElement('article');
    refreshCard.className = 'apt-refresh-card';
    const refreshHead = document.createElement('div');
    refreshHead.className = 'diagnostic-head';
    const refreshTitle = document.createElement('h3');
    refreshTitle.textContent = 'Actualiser les index APT';
    const terminalChip = document.createElement('span');
    terminalChip.className = 'status-chip status-info';
    terminalChip.textContent = 'Terminal sécurisé';
    refreshHead.append(refreshTitle, terminalChip);
    const refreshText = document.createElement('p');
    refreshText.textContent = 'Depuis la racine du projet LinuxDoctor, lancez cette commande dans un terminal. sudo y demandera votre mot de passe si nécessaire ; Linux Doctor ne le reçoit, ne le transmet et ne le conserve jamais.';
    const command = document.createElement('code');
    command.textContent = APT_REFRESH_COMMAND;
    const refreshActions = document.createElement('div');
    refreshActions.className = 'actions';
    const copy = document.createElement('button');
    copy.type = 'button';
    copy.textContent = 'Copier la commande';
    const copyStatus = document.createElement('small');
    copyStatus.className = 'muted';
    copyStatus.setAttribute('role', 'status');
    copy.addEventListener('click', () => copyText(APT_REFRESH_COMMAND, copyStatus));
    refreshActions.appendChild(copy);
    const refreshNote = document.createElement('small');
    refreshNote.className = 'muted';
    refreshNote.textContent = 'Cette commande actualise les index avec apt-get update, régénère le rapport, mais n’installe aucun paquet. Revenez ensuite ici et choisissez « Recharger le rapport ».';
    refreshCard.append(refreshHead, refreshText, command, refreshActions, copyStatus, refreshNote);
    cards.push(refreshCard);

    const overview = document.createElement('article');
    overview.className = `apt-overview ${inventory.inventory_available ? 'status-info' : 'status-unknown'}`;
    const overviewTitle = document.createElement('h3');
    const counts = inventory.counts || {};
    overviewTitle.textContent = inventory.inventory_available
      ? formatCount(Number(counts.candidates) || 0, 'mise à jour candidate', 'mises à jour candidates')
      : 'Inventaire APT indisponible';
    const overviewText = document.createElement('p');
    overviewText.className = 'muted';
    overviewText.textContent = inventory.inventory_available
      ? !inventory.selection_available && counts.candidates > 0
        ? `${formatCount(counts.candidates, 'candidate trouvée', 'candidats trouvés')}, mais leur état actuel n’a pas pu être confirmé par APT.`
        : `${formatCount(counts.ready, 'prête', 'prêtes')} selon la simulation · ${formatCount(counts.phased, 'en déploiement progressif', 'en déploiement progressif')} · ${formatCount(counts.deferred, 'différée', 'différées')} · ${formatCount(counts.unknown, 'à l’état inconnu', 'à l’état inconnu')} · ${counts.security} via un dépôt de sécurité · ${formatCount(counts.held, 'retenue manuellement', 'retenues manuellement')}.`
      : 'Linux Doctor n’a pas pu exécuter ou interpréter la simulation locale APT. Il ne prétend donc pas que le système est à jour.';
    const freshness = document.createElement('small');
    freshness.className = 'muted';
    freshness.textContent = inventory.cache_available
      ? `Index APT locaux âgés de ${inventory.cache_age_days} jour${inventory.cache_age_days === 1 ? '' : 's'}.`
      : 'Âge des index APT indisponible.';
    overview.append(overviewTitle, overviewText, freshness);
    if (inventory.truncated) {
      const warning = document.createElement('p');
      warning.className = 'apt-warning';
      warning.textContent = 'La liste a été tronquée : tous les candidats ne sont pas affichés.';
      overview.appendChild(warning);
    }
    if (inventory.inventory_available && counts.candidates > 0 && !inventory.metadata_available) {
      const warning = document.createElement('p');
      warning.className = 'apt-warning';
      warning.textContent = `${counts.with_metadata}/${counts.candidates} descriptions locales seulement sont disponibles.`;
      overview.appendChild(warning);
    }
    cards.push(overview);

    const packageList = document.createElement('div');
    packageList.className = 'apt-package-list';
    packages.forEach(update => {
      const state = aptUpdateState(update, inventory.selection_available);
      const card = document.createElement('article');
      card.className = `apt-update-card ${update.security_origin === true ? 'has-security' : ''}`;
      const head = document.createElement('div');
      head.className = 'diagnostic-head';
      const identity = document.createElement('div');
      const title = document.createElement('h3');
      title.textContent = update.name || 'Paquet sans nom';
      const version = document.createElement('p');
      version.className = 'apt-version';
      version.textContent = `${update.installed_version || '?'} → ${update.candidate_version || '?'}`;
      identity.append(title, version);
      const chip = document.createElement('span');
      chip.className = `status-chip ${state.className}`;
      chip.textContent = state.label;
      head.append(identity, chip);

      const evidence = document.createElement('div');
      evidence.className = 'evidence';
      [
        { label: 'Architecture', value: update.architecture || 'inconnue' },
        { label: 'Paquet source', value: update.source_package || update.name || 'inconnu' },
        { label: 'Section', value: update.section || 'non renseignée' },
        { label: 'Origine', value: update.origin || 'non renseignée' },
        { label: 'Dépôt', value: update.repository || 'non renseigné' }
      ].forEach(item => {
        const pill = document.createElement('span');
        pill.textContent = `${item.label} : ${item.value}`;
        evidence.appendChild(pill);
      });
      if (update.security_origin === true) {
        const security = document.createElement('span');
        security.className = 'apt-security-badge';
        security.textContent = 'Fourni via un dépôt de sécurité';
        evidence.appendChild(security);
      }
      if (update.held === true) {
        const held = document.createElement('span');
        held.className = 'apt-held-badge';
        held.textContent = 'Paquet retenu manuellement';
        evidence.appendChild(held);
      }

      const role = document.createElement('section');
      role.className = 'apt-role';
      const roleTitle = document.createElement('h4');
      roleTitle.textContent = 'À quoi sert ce paquet ?';
      const purpose = document.createElement('p');
      purpose.textContent = update.purpose || 'Famille fonctionnelle non déterminée.';
      const technicalDetails = document.createElement('details');
      technicalDetails.className = 'apt-description-details';
      const technicalToggle = document.createElement('summary');
      technicalToggle.textContent = 'Afficher la description technique (souvent en anglais)';
      const description = document.createElement('p');
      description.className = 'muted apt-description';
      description.textContent = update.description || 'Description locale non fournie par les métadonnées APT.';
      const descriptionNote = document.createElement('small');
      descriptionNote.className = 'muted';
      descriptionNote.textContent = 'La description locale explique le rôle du paquet (elle peut être en anglais), pas les changements précis de cette version.';
      technicalDetails.append(technicalToggle, description, descriptionNote);
      role.append(roleTitle, purpose, technicalDetails);
      card.append(head, evidence, role);
      packageList.appendChild(card);
    });
    if (packages.length) {
      const packageDetails = document.createElement('details');
      packageDetails.className = 'apt-packages-details';
      const packageToggle = document.createElement('summary');
      packageToggle.textContent = `Afficher les ${packages.length} paquets et leur rôle`;
      packageDetails.append(packageToggle, packageList);
      cards.push(packageDetails);
    }
  }
  if (category.id === 'apps') {
    const inventory = report.apps_inventory || {};
    (inventory.recommended || []).forEach(app => {
      const card = document.createElement('article');
      card.className = `app-card ${app.installed ? 'status-ok' : 'status-info'}`;
      const head = document.createElement('div');
      head.className = 'diagnostic-head';
      const title = document.createElement('h3');
      title.textContent = app.name || 'Application recommandée';
      const state = document.createElement('span');
      state.className = `status-chip ${app.installed ? 'status-ok' : 'status-info'}`;
      state.textContent = inventory.available ? app.installed ? 'Installée' : 'À installer' : 'État indisponible';
      head.append(title, state);
      const summary = document.createElement('p');
      summary.className = 'muted';
      summary.textContent = app.summary || '';
      card.append(head, summary);
      if (!app.installed && app.install_command) {
        const command = document.createElement('code');
        command.textContent = app.install_command;
        const note = document.createElement('small');
        note.className = 'muted';
        note.textContent = 'Commande affichée à titre informatif : Linux Doctor ne lance aucune installation.';
        card.append(command, note);
      }
      cards.push(card);
    });
  }
  cards.push(...(category.diagnostics || []).map(diagnostic => {
    const card = document.createElement('article');
    card.className = `diagnostic-card ${statusClass(diagnostic.severity)}`;

    const head = document.createElement('div');
    head.className = 'diagnostic-head';
    const left = document.createElement('div');
    const h3 = document.createElement('h3');
    h3.textContent = diagnostic.title;
    const sub = document.createElement('p');
    sub.className = 'muted';
    sub.textContent = diagnostic.summary || '';
    left.append(h3, sub);
    const chip = document.createElement('span');
    chip.className = `status-chip ${statusClass(diagnostic.severity)}`;
    chip.textContent = statusLabel(diagnostic.severity);
    head.append(left, chip);

    const evidence = document.createElement('div');
    evidence.className = 'evidence';
    (diagnostic.evidence || []).forEach(item => {
      const pill = document.createElement('span');
      pill.textContent = evidenceText(item);
      evidence.appendChild(pill);
    });

    const actions = document.createElement('div');
    actions.className = 'actions';
    const details = document.createElement('button');
    details.type = 'button';
    details.textContent = 'Détails et recommandations';
    details.addEventListener('click', () => openInsight(diagnostic, 'Détails et recommandations'));
    actions.append(details);

    card.append(head, evidence, actions);
    return card;
  }));
  diagnosticsTarget.replaceChildren(...cards);
}

function renderGoodNews(report) {
  const items = report.good_news || [];
  if (items.length === 0) {
    goodNewsTarget.innerHTML = '<article class="news-card"><strong>Pas encore de vérification positive détaillée</strong><p class="muted">Le rapport actuel ne contient aucune bonne nouvelle explicite.</p></article>';
    return;
  }
  goodNewsTarget.replaceChildren(...items.map(item => {
    const card = document.createElement('article');
    card.className = 'news-card';
    const title = document.createElement('strong');
    title.textContent = item.title;
    const category = document.createElement('p');
    category.className = 'muted';
    category.textContent = item.category;
    const detail = document.createElement('p');
    detail.textContent = item.detail;
    card.append(title, category, detail);
    return card;
  }));
}

function renderHistory(history) {
  if (!history || !history.enabled) {
    historyPanel.hidden = true;
    return;
  }
  historyPanel.hidden = false;

  if (history.compatible === false) {
    setText(historyMessage, 'Comparaison suspendue : le score courant est incomplet, donc aucune tendance fiable n’est calculée.');
    document.querySelector('.history-bar').hidden = true;
    setText(historyValues, 'Relancez l’analyse depuis une session graphique pour reprendre le suivi.');
    historyChanges.replaceChildren();
    return;
  }

  if (!history.has_previous) {
    setText(historyMessage, 'Première analyse enregistrée. Le suivi commencera à la prochaine analyse.');
    document.querySelector('.history-bar').hidden = true;
    setText(historyValues, '');
    historyChanges.replaceChildren();
    return;
  }

  document.querySelector('.history-bar').hidden = false;
  const direction = history.score_delta > 0 ? 's’améliore' : history.score_delta < 0 ? 'se dégrade' : 'reste stable';
  setText(historyMessage, `Votre score ${direction} de ${Math.abs(history.score_delta)} point${Math.abs(history.score_delta) === 1 ? '' : 's'}. La partition système est passée de ${history.storage_root.previous_used_percent} % à ${history.storage_root.current_used_percent} %.`);
  setText(historyValues, `Analyse précédente : ${history.previous_score}/100 · Aujourd’hui : ${history.previous_score + history.score_delta}/100`);
  if (previousMarker) previousMarker.style.width = `${history.previous_score}%`;
  if (currentMarker) currentMarker.style.width = `${history.previous_score + history.score_delta}%`;

  historyChanges.replaceChildren(...(history.changes || []).map(change => {
    const card = document.createElement('article');
    card.className = 'history-change';
    const title = document.createElement('strong');
    title.textContent = change.id;
    const detail = document.createElement('p');
    detail.className = 'muted';
    detail.textContent = `${change.kind} · ${change.previous} → ${change.current}`;
    card.append(title, detail);
    return card;
  }));
}

function render(report) {
  const normalized = normalizeReport(report);
  const summary = normalized.summary || { counts: { problem: 0, warning: 0, ok: 0, unknown: 0 }, overview: '', last_analysis_date: '' };
  const score = normalized.system_health?.score ?? 0;
  const scoreComplete = normalized.system_health?.complete !== false;
  const application = normalized.application || {};

  setText(greetingTarget, summary.greeting || 'Bonjour');
  setText(lastAnalysisTarget, `Dernière analyse : ${formatAnalysisDate(summary.last_analysis_date)}`);
  setText(overviewTarget, summary.overview || '');
  setText(document.querySelector('#score'), scoreComplete ? score : '—');
  setText(document.querySelector('#app-version'), application.version
    ? `${application.name || 'Linux Doctor Gamer Edition'} · v${application.version}`
    : 'Linux Doctor Gamer Edition');
  const repository = document.querySelector('#repository-link');
  const repositoryUrl = safeHttpsUrl(application.repository);
  if (repository && repositoryUrl) repository.href = repositoryUrl;
  scoreRing.style.setProperty('--score', scoreComplete ? score : 0);
  document.querySelector('#score-ring').title = normalized.system_health?.label || 'health';
  renderCounts(summary, (normalized.good_news || []).length);
  renderCategories(normalized);
  renderDiagnostics(normalized);
  renderGoodNews(normalized);
  renderHistory(normalized.history);
}

async function loadReport(animated = false) {
  if (animated) {
    analyzeButton.disabled = true;
    progressBar.hidden = false;
    analyzeButton.textContent = 'Actualisation en cours…';
    if (analysisTimer) clearInterval(analysisTimer);
    analysisTimer = setInterval(() => {
      progressBar.hidden = false;
    }, 150);
  }
  try {
    const response = await fetch(`report.json?ts=${Date.now()}`, { cache: 'no-store' });
    if (!response.ok) {
      throw new Error('Générez le rapport avec make run.');
    }
    render(validateReport(await response.json()));
  } finally {
    if (analysisTimer) {
      clearInterval(analysisTimer);
      analysisTimer = null;
    }
    progressBar.hidden = true;
    analyzeButton.disabled = false;
    analyzeButton.textContent = 'Recharger le rapport';
  }
}

document.querySelector('#insight-dialog button').addEventListener('click', () => dialog.close());
analyzeButton.addEventListener('click', () => loadReport(true).catch(error => { diagnosticsTarget.textContent = error.message; }));
scrollTopButton?.addEventListener('click', () => window.scrollTo({ top: 0, behavior: 'smooth' }));
window.addEventListener('scroll', () => {
  scrollTopButton?.classList.toggle('visible', window.scrollY > 500);
}, { passive: true });
loadReport().catch(error => {
  diagnosticsTarget.textContent = error.message;
  setText(overviewTarget, error.message);
});
