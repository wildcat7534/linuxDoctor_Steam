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
  diagnosticsTarget.replaceChildren(...(category.diagnostics || []).map(diagnostic => {
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
      pill.textContent = item.value ? `${item.label}: ${item.value}` : item.label;
      evidence.appendChild(pill);
    });

    const actions = document.createElement('div');
    actions.className = 'actions';
    const why = document.createElement('button');
    why.type = 'button';
    why.textContent = 'Pourquoi ?';
    why.addEventListener('click', () => openInsight(diagnostic, 'Pourquoi ?'));
    const fix = document.createElement('button');
    fix.type = 'button';
    fix.textContent = 'Comment réparer';
    fix.addEventListener('click', () => openInsight(diagnostic, 'Comment réparer'));
    actions.append(why, fix);

    card.append(head, evidence, actions);
    return card;
  }));
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
