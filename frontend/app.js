const dialog = document.querySelector('#insight-dialog');
const scoreRing = document.querySelector('#score-ring');
const analyzeButton = document.querySelector('#analyze');
const progressBar = document.querySelector('#analysis-progress');
const categoriesTarget = document.querySelector('#categories');
const diagnosticsTarget = document.querySelector('#diagnostics');
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

let selectedCategoryId = null;
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
  return `${count} ${count > 1 ? plural : singular}`;
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

function normalizeReport(report) {
  if (report.summary && report.categories) return report;

  const diagnostics = report.categories?.flatMap(category => category.diagnostics || []) || [];
  const score = report.system_health?.score ?? 0;
  const problems = diagnostics.filter(item => item.severity === 'problem').length;
  const warnings = diagnostics.filter(item => item.severity === 'warning').length;
  const ok = diagnostics.filter(item => item.severity === 'ok').length;
  const unknown = diagnostics.filter(item => item.severity === 'unknown' || !item.severity).length;
  const hasStorageProblem = diagnostics.some(item => item.severity === 'problem');

  return {
    ...report,
    summary: {
      greeting: 'Bonjour',
      last_analysis_date: '',
      overview: hasStorageProblem
        ? 'Un point prioritaire mérite votre attention.'
        : 'Votre machine est globalement en bon état.',
      counts: { problem: problems, warning: warnings, ok, unknown }
    },
    good_news: report.good_news || [],
    categories: report.categories || []
  };
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
    button.setAttribute('role', 'tab');
    button.setAttribute('aria-selected', String(selectedCategoryId === category.id));
    button.className = `category-card ${selectedCategoryId === category.id ? 'selected' : ''}`;
    button.addEventListener('click', () => {
      selectedCategoryId = category.id;
      render(report);
    });

    const top = document.createElement('div');
    top.className = 'top';
    const title = document.createElement('strong');
    title.textContent = `${category.icon || ''} ${category.name}`;
    const score = document.createElement('span');
    score.className = 'score-pill';
    score.textContent = `${category.score}%`;
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
}

function renderDiagnostics(report) {
  const category = (report.categories || []).find(item => item.id === selectedCategoryId) || report.categories?.[0];
  if (!category) {
    setText(selectedCategoryTitle, 'Diagnostics');
    setText(selectedCategorySummary, 'Aucune catégorie disponible.');
    diagnosticsTarget.textContent = 'Aucune donnée.';
    return;
  }

  setText(selectedCategoryTitle, `${category.icon || ''} ${category.name}`);
  setText(selectedCategorySummary, category.summary || '');
  const cards = [];
  if (category.id === 'storage') {
    const groups = new Map();
    (report.storage_inventory?.volumes || []).forEach(volume => {
      const key = physicalDiskKey(volume);
      groups.set(key, [...(groups.get(key) || []), volume]);
    });
    groups.forEach((volumes, disk) => {
      const group = document.createElement('section');
      group.className = 'volume-group';
      const heading = document.createElement('h3');
      heading.textContent = `Disque physique ${disk || 'inconnu'}`;
      group.appendChild(heading);
      const groupCards = document.createElement('div');
      groupCards.className = 'diagnostic-list';
      volumes.forEach(volume => {
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
  if (category.id === 'steam') {
    const inventory = report.steam_inventory || {};
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
        const list = document.createElement('p');
        list.className = 'muted';
        list.textContent = games.map(game => `${game.name} (${formatBytes(game.size_bytes)})`).join(' · ');
        card.append(head, evidence, list);
      } else card.append(head, evidence);
      cards.push(card);
    });
    const plan = report.steam_migration_plan;
    if (plan?.available) {
      const card = document.createElement('article');
      card.className = 'diagnostic-card migration-plan';
      const title = document.createElement('h3');
      title.textContent = 'Plan de migration simulé';
      const description = document.createElement('p');
      description.className = 'muted';
      description.textContent = `Destination proposée : ${plan.destination_path} (${formatBytes(plan.destination_available_bytes)} libres). Aucun fichier ne sera déplacé.`;
      const selection = document.createElement('div');
      selection.className = 'migration-selection';
      const summary = document.createElement('p');
      summary.className = 'migration-summary';
      const refresh = () => {
        const selected = [...selection.querySelectorAll('input:checked')]
          .reduce((total, input) => total + Number(input.dataset.size), 0);
        summary.textContent = `Sélection : ${formatBytes(selected)} · objectif : libérer ${formatBytes(plan.target_free_bytes)} sur la partition système.`;
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
      card.append(title, description, selection, summary, note);
      cards.push(card);
    }
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
    goodNewsTarget.innerHTML = '<article class="news-card"><strong>Pas encore de bonne nouvelle détaillée</strong><p class="muted">Le moteur actuel n’expose qu’un diagnostic principal, mais l’interface est prête pour plus.</p></article>';
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

  setText(greetingTarget, summary.greeting || 'Bonjour');
  setText(lastAnalysisTarget, summary.last_analysis_date ? `Dernière analyse : ${summary.last_analysis_date}` : 'Dernière analyse : inconnue');
  setText(overviewTarget, summary.overview || '');
  setText(document.querySelector('#score'), score);
  scoreRing.style.setProperty('--score', score);
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
    analyzeButton.textContent = 'Analyse en cours…';
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
    render(await response.json());
  } finally {
    if (analysisTimer) {
      clearInterval(analysisTimer);
      analysisTimer = null;
    }
    progressBar.hidden = true;
    analyzeButton.disabled = false;
    analyzeButton.textContent = 'Analyser maintenant';
  }
}

document.querySelector('#insight-dialog button').addEventListener('click', () => dialog.close());
analyzeButton.addEventListener('click', () => loadReport(true).catch(error => { diagnosticsTarget.textContent = error.message; }));
loadReport().catch(error => {
  diagnosticsTarget.textContent = error.message;
  setText(overviewTarget, error.message);
});
