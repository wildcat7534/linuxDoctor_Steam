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
  const target = document.querySelector('#diagnostics');
  const template = document.querySelector('template');
  target.replaceChildren(...report.categories.flatMap(category => category.diagnostics).map(diagnostic => {
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

document.querySelector('#why button').addEventListener('click', () => dialog.close());
fetch('report.json', { cache: 'no-store' }).then(response => response.ok ? response.json() : Promise.reject(new Error('Générez le rapport avec make run.'))).then(render).catch(error => { document.querySelector('#diagnostics').textContent = error.message; });
