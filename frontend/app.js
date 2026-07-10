const statusLabel = { ok: 'OK', warning: 'À surveiller', problem: 'Problème', unknown: 'Inconnu' };
const dialog = document.querySelector('#why');

function openExplanation(diagnostic) {
  const details = [['Ce que nous avons observé', diagnostic.explanation.observed], ["Pourquoi c'est important", diagnostic.explanation.why], ['Impact possible', diagnostic.explanation.impact], ['Prochaine étape', diagnostic.explanation.next_step]];
  const list = dialog.querySelector('dl');
  list.replaceChildren(...details.map(([title, text]) => { const group = document.createElement('div'); const term = document.createElement('dt'); const description = document.createElement('dd'); term.textContent = title; description.textContent = text; group.append(term, description); return group; }));
  dialog.showModal();
}

function render(report) {
  document.querySelector('#score').textContent = report.system_health.score;
  const diagnostics = report.categories.flatMap(category => category.diagnostics);
  const problems = diagnostics.filter(item => item.severity === 'problem').length;
  const warnings = diagnostics.filter(item => item.severity === 'warning').length;
  document.querySelector('#summary').textContent = problems ? `${problems} problème${problems > 1 ? 's' : ''} mérite${problems > 1 ? 'nt' : ''} votre attention.` : 'Votre système ne présente pas de problème prioritaire.';
  document.querySelector('#counts').textContent = `${problems} problème${problems > 1 ? 's' : ''} · ${warnings} avertissement${warnings > 1 ? 's' : ''}`;
  renderHistory(report.history);
  const target = document.querySelector('#diagnostics');
  const template = document.querySelector('template');
  target.replaceChildren(...diagnostics.map(diagnostic => {
    const fragment = template.content.cloneNode(true);
    const article = fragment.querySelector('article'); article.dataset.severity = diagnostic.severity;
    fragment.querySelector('.severity').textContent = statusLabel[diagnostic.severity];
    fragment.querySelector('h2').textContent = diagnostic.title;
    fragment.querySelector('.evidence').textContent = diagnostic.evidence.map(item => item.value ? `${item.label} : ${item.value}` : item.label).join(' · ');
    fragment.querySelector('.recommendation').textContent = `Conseil : ${diagnostic.recommendations[0].label}`;
    fragment.querySelector('button').addEventListener('click', () => openExplanation(diagnostic));
    return fragment;
  }));
}

function renderHistory(history) {
  const panel = document.querySelector('#history');
  if (!history || !history.enabled) return;
  panel.hidden = false;
  const message = document.querySelector('#history-message');
  if (!history.has_previous) {
    message.textContent = 'Première analyse enregistrée. Le suivi commencera à la prochaine analyse.';
    document.querySelector('.bar').hidden = true;
    return;
  }
  const { previous_score, score_delta, storage_root } = history;
  const direction = score_delta > 0 ? 's’améliore' : score_delta < 0 ? 'se dégrade' : 'reste stable';
  message.textContent = `Votre score ${direction} de ${Math.abs(score_delta)} point${Math.abs(score_delta) === 1 ? '' : 's'}. La partition système est passée de ${storage_root.previous_used_percent} % à ${storage_root.current_used_percent} %.`;
  document.querySelector('#history-values').textContent = `Analyse précédente : ${previous_score}/100 · Aujourd’hui : ${previous_score + score_delta}/100`;
  document.querySelector('#previous-marker').style.width = `${previous_score}%`;
  document.querySelector('#current-marker').style.width = `${previous_score + score_delta}%`;
}

document.querySelector('#why button').addEventListener('click', () => dialog.close());
fetch('report.json', { cache: 'no-store' }).then(response => response.ok ? response.json() : Promise.reject(new Error('Générez le rapport avec make run.'))).then(render).catch(error => { document.querySelector('#diagnostics').textContent = error.message; });
