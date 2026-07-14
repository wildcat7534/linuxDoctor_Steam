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
const KNOWLEDGE_UPDATE_COMMAND = './scripts/update-knowledge.sh';
const APT_UPDATE_STATES = new Set(['ready', 'phased', 'deferred', 'unknown']);
const CATEGORY_ICONS = { storage: '💾', gaming: '🎮', graphics: '⚡', future_lab: '🧪', updates: '📦', apps: '🛠️' };
const KNOWLEDGE_ICONS = { game: '🕹️', steam: '♨️', controller: '🎮', gfn: '☁️', ubuntu: '🐧' };
const INTEGER_FORMATTER = new Intl.NumberFormat('fr-FR', { maximumFractionDigits: 0 });
const LOAD_FORMATTER = new Intl.NumberFormat('fr-FR', { minimumFractionDigits: 2, maximumFractionDigits: 2 });
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

function formatInteger(value) {
  return Number.isFinite(value) ? INTEGER_FORMATTER.format(value) : 'Indisponible';
}

function formatKib(value) {
  return Number.isFinite(value) ? formatBytes(value * 1024) : 'Indisponible';
}

function futureLabAvailable(section) {
  return section?.state === 'available';
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

function volumeSize(volume) {
  return Number.isFinite(volume.size_bytes) && volume.size_bytes > 0 ? volume.size_bytes : 0;
}

function partitionRole(volume) {
  const mountpoint = String(volume.mountpoint || '').toLowerCase();
  const filesystem = String(volume.filesystem || '').toLowerCase();
  const description = `${volume.label || ''} ${volume.partition_label || ''}`.toLowerCase();

  if (mountpoint === '/') {
    return { key: 'system', icon: '🐧', label: 'Système Ubuntu', detail: 'Racine du système' };
  }
  if (mountpoint === '/boot/efi' || description.includes('efi system')) {
    return { key: 'boot', icon: '🚀', label: 'Démarrage EFI', detail: 'Démarre le PC' };
  }
  if (filesystem === 'swap') {
    return { key: 'swap', icon: '🧠', label: 'Mémoire d’appoint', detail: 'Espace swap Linux' };
  }
  if (volume.windows_confirmed) {
    return { key: 'windows', icon: '🪟', label: 'Windows détecté', detail: 'Installation confirmée sur cette partition' };
  }
  if (description.includes('recovery') || description.includes('récupération')) {
    return { key: 'recovery', icon: '🩹', label: 'Récupération', detail: 'Rôle indiqué par son libellé' };
  }
  if (description.includes('reserved') || description.includes('réservé')) {
    return { key: 'reserved', icon: '🧩', label: 'Réservée au système', detail: 'Rôle indiqué par son libellé' };
  }
  if (volume.windows_system_component) {
    return { key: 'windows', icon: '🪟', label: 'Composant Windows', detail: 'Type de partition système Windows détecté' };
  }
  if (volume.windows_data_partition) {
    return { key: 'data', icon: '🗂️', label: 'Partition NTFS', detail: 'Format Windows ; contenu non confirmé' };
  }
  if (volume.mounted) {
    return { key: 'data', icon: '📂', label: 'Données accessibles', detail: 'Partition actuellement ouverte' };
  }
  return { key: 'unknown', icon: '🗄️', label: 'Rôle non confirmé', detail: 'Partition détectée, mais non ouverte' };
}

function partitionState(volume) {
  if (!volume.mounted) {
    return { key: 'offline', icon: '○', label: 'Non montée', detail: 'Son occupation ne peut pas être mesurée.' };
  }
  if (volume.read_only) {
    return { key: 'readonly', icon: '🔒', label: 'Lecture seule', detail: 'Les fichiers sont lisibles, mais pas modifiables.' };
  }
  return { key: 'online', icon: '✓', label: 'Accessible', detail: 'Lecture et écriture disponibles.' };
}

function diskMetrics(volumes) {
  const totalBytes = volumes.reduce((total, volume) => total + volumeSize(volume), 0);
  const mounted = volumes.filter(volume => volume.mounted);
  const measurableBytes = mounted.reduce((total, volume) => total + volumeSize(volume), 0);
  const usedBytes = mounted.reduce((total, volume) => {
    const percent = Math.max(0, Math.min(100, Number(volume.used_percent) || 0));
    return total + (volumeSize(volume) * percent / 100);
  }, 0);
  return {
    totalBytes,
    mountedCount: mounted.length,
    usedPercent: measurableBytes > 0 ? Math.round(usedBytes * 100 / measurableBytes) : null
  };
}

function appendPartitionStrip(target, volumes, diskId, interactive = false) {
  const totalBytes = volumes.reduce((total, volume) => total + volumeSize(volume), 0);
  const strip = document.createElement(interactive ? 'div' : 'span');
  strip.className = `partition-strip${interactive ? ' interactive' : ''}`;
  strip.setAttribute('aria-label', 'Répartition visuelle des partitions détectées');

  volumes.forEach((volume, index) => {
    const role = partitionRole(volume);
    const state = partitionState(volume);
    const size = volumeSize(volume);
    const share = totalBytes > 0 ? size * 100 / totalBytes : 100 / volumes.length;
    const segment = document.createElement(interactive ? 'button' : 'span');
    if (interactive) {
      segment.type = 'button';
      segment.addEventListener('click', () => {
        document.querySelector(`#${diskId}-partition-${index}`)?.scrollIntoView({ behavior: 'smooth', block: 'center' });
      });
    }
    segment.className = `partition-segment role-${role.key} state-${state.key}`;
    segment.style.setProperty('--partition-share', `${share}%`);
    segment.title = `${volume.label || volume.path} · ${size > 0 ? formatBytes(size) : 'taille inconnue'} · ${state.label}`;
    segment.setAttribute('aria-label', segment.title);
    if (share >= 11) segment.textContent = String(index + 1);
    strip.appendChild(segment);
  });
  target.appendChild(strip);
}

function createPartitionLegend(volumes) {
  const legend = document.createElement('div');
  legend.className = 'partition-legend';
  volumes.forEach((volume, index) => {
    const role = partitionRole(volume);
    const state = partitionState(volume);
    const item = document.createElement('span');
    const marker = document.createElement('i');
    marker.className = `role-${role.key} state-${state.key}`;
    marker.setAttribute('aria-hidden', 'true');
    const copy = document.createElement('span');
    const name = document.createElement('strong');
    name.textContent = `${index + 1}. ${volume.label || volume.path}`;
    const detail = document.createElement('small');
    detail.textContent = `${volumeSize(volume) > 0 ? formatBytes(volumeSize(volume)) : 'Taille inconnue'} · ${role.label}`;
    copy.append(name, detail);
    item.append(marker, copy);
    legend.appendChild(item);
  });
  return legend;
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
  const gfnCosts = document.createElement('div');
  gfnCosts.className = 'gfn-cost-grid';
  const gfnCostPanel = document.createElement('section');
  gfnCostPanel.className = 'gfn-cost-panel';
  const gfnCostTitle = document.createElement('h4');
  gfnCostTitle.textContent = '☁️ Combien coûtent réellement 100 h avec GeForce NOW ?';
  const gfnCostIntro = document.createElement('p');
  gfnCostIntro.textContent = 'Le coût total additionne l’abonnement mensuel — ou l’équivalent mensuel de l’annuel payé d’avance — et l’électricité du PC qui reçoit le flux vidéo.';
  const gfnPlans = [
    { name: 'Performance', monthlyPrice: 10.99, annualPrice: 109.99 },
    { name: 'Ultimate', monthlyPrice: 21.99, annualPrice: 219.99 }
  ];
  const profiles = [
    { id: 'gfn', icon: '☁️', name: 'GeForce NOW', watts: 110, detail: 'PC en décodage + écran' },
    { id: 'mid', icon: '🖥️', name: 'PC gamer moyen', watts: 420, detail: 'RTX 5070 / Ryzen 5 + 32 Go + écran 27″ 120 Hz' },
    { id: 'uber', icon: '🚀', name: 'Uber PC', watts: 620, detail: 'RTX 4080 Super / CPU haut de gamme + 32 Go DDR5 + écran 27″ 120 Hz' }
  ];
  const money = value => `${value.toFixed(2).replace('.', ',')} €`;
  const refresh = () => {
    const price = Number(rate.value) || 0;
    const duration = Number(hours.value) || 0;
    titleDuration.textContent = `${duration} h de jeu`;
    gfnCostTitle.textContent = `☁️ Combien coûtent réellement ${duration} h avec GeForce NOW ?`;
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
    const gfnElectricity = profiles[0].watts * duration / 1000 * price;
    gfnCosts.replaceChildren(...gfnPlans.map(plan => {
      const card = document.createElement('article');
      card.className = `gfn-plan ${plan.name === 'Ultimate' ? 'ultimate' : 'performance'}`;
      const name = document.createElement('strong');
      name.textContent = `GeForce NOW ${plan.name}`;
      const billing = document.createElement('div');
      billing.className = 'gfn-billing-grid';
      const annualMonthly = plan.annualPrice / 12;
      const annualSaving = plan.monthlyPrice * 12 - plan.annualPrice;
      const annualSavingPercent = annualSaving * 100 / (plan.monthlyPrice * 12);
      [
        {
          kind: 'monthly',
          label: 'Paiement mensuel',
          price: `${money(plan.monthlyPrice)} / mois`,
          formula: `${money(plan.monthlyPrice)} d’abonnement + ${money(gfnElectricity)} d’électricité`,
          total: `${duration > 100 ? 'Minimum ' : ''}${money(plan.monthlyPrice + gfnElectricity)} pour ${duration} h`
        },
        {
          kind: 'annual',
          label: 'Paiement annuel',
          price: `${money(plan.annualPrice)} en une fois`,
          formula: `Équivaut à ${money(annualMonthly)} / mois + ${money(gfnElectricity)} d’électricité`,
          total: `${duration > 100 ? 'Minimum ' : ''}${money(annualMonthly + gfnElectricity)} par mois équivalent pour ${duration} h`
        }
      ].forEach(option => {
        const item = document.createElement('section');
        item.className = `gfn-billing-option ${option.kind}`;
        const optionLabel = document.createElement('span');
        optionLabel.textContent = option.label;
        const optionPrice = document.createElement('strong');
        optionPrice.textContent = option.price;
        const formula = document.createElement('small');
        formula.textContent = option.formula;
        const total = document.createElement('b');
        total.textContent = option.total;
        item.append(optionLabel, optionPrice, formula, total);
        billing.appendChild(item);
      });
      const saving = document.createElement('small');
      saving.className = 'gfn-saving';
      saving.textContent = `Avec l’annuel : ${money(annualSaving)} économisés par an (≈ ${annualSavingPercent.toFixed(1).replace('.', ',')} %), si l’abonnement reste utile toute l’année.`;
      const hourly = document.createElement('small');
      hourly.textContent = duration <= 0
        ? 'Saisissez une durée pour obtenir le coût horaire.'
        : duration <= 100
          ? `Coût horaire abonnement + électricité : mensuel ≈ ${money((plan.monthlyPrice + gfnElectricity) / duration)} · annuel mensualisé ≈ ${money((annualMonthly + gfnElectricity) / duration)}.`
          : 'Coût horaire non calculé : l’achat éventuel d’heures supplémentaires n’est pas inclus.';
      card.append(name, billing, saving, hourly);
      return card;
    }));
    gfnCostIntro.textContent = duration <= 100
      ? `Les formules payantes incluent jusqu’à 100 h chaque mois. L’annuel coûte moins par mois, mais son prix complet est débité en une fois.`
      : `Au-delà de 100 h dans un même mois, du temps supplémentaire peut être facturé : les totaux ci-dessous ne l’incluent pas.`;
  };
  rate.addEventListener('input', refresh);
  hours.addEventListener('input', refresh);
  refresh();
  const source = document.createElement('small');
  source.className = 'energy-sources muted';
  source.append('Électricité : valeur initiale 0,194 €/kWh TTC — Tarif Bleu Base 3 ou 6 kVA, France métropolitaine, au 1er février 2026, modifiable selon votre contrat (');
  const electricitySource = document.createElement('a');
  electricitySource.href = 'https://www.cre.fr/consommateurs/comprendre-les-tarifs-reglementes-de-vente-delectricite-trve.html';
  electricitySource.target = '_blank';
  electricitySource.rel = 'noreferrer';
  electricitySource.textContent = 'source CRE';
  source.append(electricitySource, '). Tarifs GeForce NOW France mensuels et annuels vérifiés le 14 juillet 2026 : ');
  const nvidiaPricing = document.createElement('a');
  nvidiaPricing.href = 'https://www.nvidia.com/fr-fr/geforce-now/#product-matrix';
  nvidiaPricing.target = '_blank';
  nvidiaPricing.rel = 'noreferrer';
  nvidiaPricing.textContent = 'tarifs officiels NVIDIA';
  source.append(nvidiaPricing, '. Le plafond reste mensuel avec l’offre annuelle : ce n’est pas une réserve immédiate de 1 200 h. Jusqu’à 15 h non utilisées peuvent être reportées au mois suivant selon les conditions NVIDIA. ');
  const nvidiaFaq = document.createElement('a');
  nvidiaFaq.href = 'https://www.nvidia.com/fr-fr/geforce-now/faq/';
  nvidiaFaq.target = '_blank';
  nvidiaFaq.rel = 'noreferrer';
  nvidiaFaq.textContent = 'Voir la FAQ officielle';
  source.append(nvidiaFaq, '. Jeux, connexion Internet et achats de temps supplémentaire non inclus.');
  gfnCostPanel.append(gfnCostTitle, gfnCostIntro, gfnCosts);
  panel.append(title, baseline, controls, estimates, gfnCostPanel, source);
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
    const hasReportedScore = Number.isFinite(category.score);
    const scoreAvailable = category.status !== 'unknown' && hasReportedScore;
    const scoreLabel = !hasReportedScore
      ? 'Catégorie informative, sans note'
      : scoreAvailable ? `${category.score} sur 100` : 'Score indisponible';
    score.textContent = !hasReportedScore ? 'Sans note' : scoreAvailable ? `${category.score}%` : '—';
    score.title = category.score_explanation || scoreLabel;
    score.setAttribute('aria-label', `${scoreLabel}. ${category.score_explanation || ''}`.trim());
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
      { icon: '🎮', value: String((steam.games || []).filter(item => item.kind !== 'tool').length), label: 'jeux Steam détectés' },
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
    const protectedDiskCount = new Set(volumes
      .filter(volume => volume.windows_protected)
      .map(physicalDiskKey)).size;
    return [
      { icon: '💽', value: String(volumes.length), label: 'volumes détectés' },
      { icon: '✅', value: String(volumes.filter(volume => volume.mounted).length), label: 'volumes montés' },
      { icon: '🪟', value: String(protectedDiskCount), label: 'disques protégés par prudence' },
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
  if (category.id === 'future_lab') {
    const lab = report.future_lab || {};
    const cpu = lab.cpu || {};
    const load = lab.load || {};
    const memory = lab.memory || {};
    const network = lab.network || {};
    const disks = lab.disks || {};
    const interfaceCount = Number.isFinite(network.reported_interface_count)
      ? network.reported_interface_count : network.interfaces?.length || 0;
    const deviceCount = Number.isFinite(disks.reported_device_count)
      ? disks.reported_device_count : disks.devices?.length || 0;
    return [
      {
        icon: '🧠',
        value: futureLabAvailable(cpu) ? formatInteger(cpu.logical_cpu_count) : 'Indisponible',
        label: 'CPU logiques · compteurs depuis le démarrage'
      },
      {
        icon: '⚖️',
        value: futureLabAvailable(load) && Number.isFinite(load.one_minute) ? LOAD_FORMATTER.format(load.one_minute) : 'Indisponible',
        label: 'charge moyenne à 1 min · ce n’est pas un %'
      },
      {
        icon: '🌱',
        value: futureLabAvailable(memory) ? formatKib(memory.available) : 'Indisponible',
        label: `RAM disponible${futureLabAvailable(memory) ? ` sur ${formatKib(memory.total)}` : ''}`
      },
      {
        icon: '🌐',
        value: futureLabAvailable(network) ? formatInteger(interfaceCount) : 'Indisponible',
        label: 'interfaces · compteurs cumulés, aucun débit'
      },
      {
        icon: '💿',
        value: futureLabAvailable(disks) ? formatInteger(deviceCount) : 'Indisponible',
        label: 'périphériques · compteurs cumulés, aucune vitesse'
      }
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
  if (category.id === 'future_lab') snapshot.classList.add('future-lab-snapshot');
  const heading = document.createElement('div');
  heading.className = 'snapshot-heading';
  const mascot = document.createElement('span');
  mascot.className = 'snapshot-mascot';
  mascot.setAttribute('aria-hidden', 'true');
  mascot.textContent = category.status === 'problem' ? '😿'
    : category.status === 'warning' ? '🧐'
      : category.status === 'unknown' ? '🤔'
        : category.id === 'future_lab' ? '🧑‍🔬' : '😺';
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
  scoreTitle.textContent = !Number.isFinite(category.score)
    ? '💡 Pourquoi cette catégorie n’est-elle pas notée ?'
    : category.status === 'unknown'
      ? '💡 Pourquoi le score est-il indisponible ?' : `💡 Pourquoi ${category.score} % ?`;
  const scoreCopy = document.createElement('p');
  scoreCopy.textContent = category.score_explanation || 'Le rapport ne fournit pas encore le détail de ce score.';
  scoreExplanation.append(scoreTitle, scoreCopy);
  snapshot.append(heading, facts, scoreExplanation);
  return snapshot;
}

function createFutureLabCounterCard(icon, name, items) {
  const card = document.createElement('article');
  card.className = 'future-lab-counter-card';
  const title = document.createElement('h4');
  title.textContent = `${icon} ${name}`;
  const metrics = document.createElement('dl');
  items.forEach(([label, value]) => {
    const term = document.createElement('dt');
    term.textContent = label;
    const detail = document.createElement('dd');
    detail.textContent = value;
    metrics.append(term, detail);
  });
  card.append(title, metrics);
  return card;
}

function renderFutureLab(cards, report) {
  const lab = report.future_lab || {};
  const sections = [lab.cpu, lab.load, lab.memory, lab.network, lab.disks];
  const availableCount = sections.filter(futureLabAvailable).length;
  const guide = document.createElement('article');
  guide.className = `future-lab-guide ${availableCount ? 'status-info' : 'status-unknown'}`;
  const guideHead = document.createElement('div');
  guideHead.className = 'future-lab-guide-head';
  const guideCopy = document.createElement('div');
  const guideTitle = document.createElement('h3');
  guideTitle.textContent = '🔭 Une photo locale, pas un verdict';
  const guideSummary = document.createElement('p');
  guideSummary.textContent = availableCount
    ? `${availableCount}/5 sources locales sont lisibles. Future Lab les présente sans attribuer de note, de débit ou d’anomalie.`
    : 'Aucune des cinq sources locales n’est lisible dans ce rapport. Linux Doctor ne remplace pas cette absence par des zéros.';
  guideCopy.append(guideTitle, guideSummary);
  const guideChip = document.createElement('span');
  guideChip.className = `status-chip ${availableCount ? 'status-info' : 'status-unknown'}`;
  guideChip.textContent = 'Lecture seule';
  guideHead.append(guideCopy, guideChip);
  const principles = document.createElement('div');
  principles.className = 'future-lab-principles';
  [
    {
      icon: '⏱️',
      title: 'Compteurs cumulatifs',
      text: 'Les ticks CPU sont cumulés depuis le démarrage. Réseau et disques cumulent depuis le démarrage ou leur dernière réinitialisation : ce ne sont ni des débits ni une activité instantanée.'
    },
    {
      icon: '⚖️',
      title: 'Charge ≠ pourcentage',
      text: 'La charge à 1, 5 et 15 minutes représente une moyenne de travail en attente ou en cours. Une valeur de 1,00 ne signifie pas 1 %.'
    },
    {
      icon: '🛡️',
      title: 'Aucune accusation',
      text: 'Cet instantané ne déduit aucune lenteur, panne ou activité suspecte. Il fournit seulement des faits locaux à lire dans leur contexte.'
    }
  ].forEach(item => {
    const principle = document.createElement('section');
    const icon = document.createElement('span');
    icon.setAttribute('aria-hidden', 'true');
    icon.textContent = item.icon;
    const copy = document.createElement('div');
    const title = document.createElement('strong');
    title.textContent = item.title;
    const text = document.createElement('p');
    text.textContent = item.text;
    copy.append(title, text);
    principle.append(icon, copy);
    principles.appendChild(principle);
  });
  guide.append(guideHead, principles);
  cards.push(guide);

  const network = lab.network || {};
  const interfaces = Array.isArray(network.interfaces) ? network.interfaces : [];
  const networkDetails = document.createElement('details');
  networkDetails.className = 'future-lab-details network-details';
  const networkSummary = document.createElement('summary');
  const networkCount = Number.isFinite(network.reported_interface_count)
    ? network.reported_interface_count : interfaces.length;
  networkSummary.textContent = futureLabAvailable(network)
    ? `🌐 Voir les ${formatCount(networkCount, 'interface réseau', 'interfaces réseau')}`
    : '🌐 Interfaces réseau indisponibles';
  const networkBody = document.createElement('div');
  networkBody.className = 'future-lab-details-body';
  if (futureLabAvailable(network)) {
    const counters = network.counters || {};
    const note = document.createElement('p');
    note.className = 'future-lab-cumulative-note';
    note.textContent = `Total cumulatif observé : ${formatBytes(counters.received_bytes)} reçus · ${formatBytes(counters.transmitted_bytes)} émis. Ces valeurs ne sont pas des vitesses.`;
    networkBody.appendChild(note);
    if (network.truncated) {
      const warning = document.createElement('p');
      warning.className = 'future-lab-limit';
      warning.textContent = `La liste est partielle : ${formatInteger(network.observed_interface_count)} interfaces ont été observées, ${formatInteger(network.reported_interface_count)} sont affichées.`;
      networkBody.appendChild(warning);
    }
    const grid = document.createElement('div');
    grid.className = 'future-lab-counter-grid';
    interfaces.forEach(item => {
      const itemCounters = item.counters || {};
      grid.appendChild(createFutureLabCounterCard('🌐', item.name || 'Interface sans nom', [
        ['Reçu, cumulatif', formatBytes(itemCounters.received_bytes)],
        ['Émis, cumulatif', formatBytes(itemCounters.transmitted_bytes)],
        ['Paquets reçus / émis', `${formatInteger(itemCounters.received_packets)} / ${formatInteger(itemCounters.transmitted_packets)}`],
        ['Erreurs reçues / émises', `${formatInteger(itemCounters.received_errors)} / ${formatInteger(itemCounters.transmitted_errors)}`],
        ['Paquets abandonnés reçus / émis', `${formatInteger(itemCounters.received_dropped)} / ${formatInteger(itemCounters.transmitted_dropped)}`]
      ]));
    });
    if (interfaces.length) {
      networkBody.appendChild(grid);
    } else {
      const empty = document.createElement('p');
      empty.className = 'muted';
      empty.textContent = 'La source réseau est lisible, mais aucune interface n’est incluse dans cet extrait.';
      networkBody.appendChild(empty);
    }
  } else {
    const unavailable = document.createElement('p');
    unavailable.className = 'muted';
    unavailable.textContent = 'Le fichier local /proc/net/dev n’a pas fourni de données exploitables. Aucun trafic nul n’est donc affirmé.';
    networkBody.appendChild(unavailable);
  }
  networkDetails.append(networkSummary, networkBody);
  cards.push(networkDetails);

  const disks = lab.disks || {};
  const devices = Array.isArray(disks.devices) ? disks.devices : [];
  const diskDetails = document.createElement('details');
  diskDetails.className = 'future-lab-details disk-details';
  const diskSummary = document.createElement('summary');
  const deviceCount = Number.isFinite(disks.reported_device_count)
    ? disks.reported_device_count : devices.length;
  diskSummary.textContent = futureLabAvailable(disks)
    ? `💿 Voir les ${formatCount(deviceCount, 'périphérique disque', 'périphériques disque')}`
    : '💿 Compteurs disque indisponibles';
  const diskBody = document.createElement('div');
  diskBody.className = 'future-lab-details-body';
  if (futureLabAvailable(disks)) {
    const note = document.createElement('p');
    note.className = 'future-lab-cumulative-note';
    note.textContent = 'Lectures, écritures et secteurs sont des compteurs noyau cumulatifs. Les secteurs restent volontairement bruts : Linux Doctor ne les transforme pas ici en octets ou en vitesse.';
    diskBody.appendChild(note);
    if (Number.isFinite(disks.skipped_pseudo_device_count) && disks.skipped_pseudo_device_count > 0) {
      const filtered = document.createElement('small');
      filtered.className = 'muted';
      filtered.textContent = `${formatInteger(disks.skipped_pseudo_device_count)} périphériques virtuels loop, ram ou zram ont été écartés de cette liste.`;
      diskBody.appendChild(filtered);
    }
    if (disks.truncated) {
      const warning = document.createElement('p');
      warning.className = 'future-lab-limit';
      warning.textContent = `La liste est partielle : ${formatInteger(disks.observed_device_count)} périphériques ont été observés, ${formatInteger(disks.reported_device_count)} sont affichés.`;
      diskBody.appendChild(warning);
    }
    const grid = document.createElement('div');
    grid.className = 'future-lab-counter-grid';
    devices.forEach(device => {
      const counters = device.counters || {};
      grid.appendChild(createFutureLabCounterCard('💿', device.name || 'Périphérique sans nom', [
        ['Lectures terminées', formatInteger(counters.reads_completed)],
        ['Écritures terminées', formatInteger(counters.writes_completed)],
        ['Secteurs lus, bruts', formatInteger(counters.sectors_read)],
        ['Secteurs écrits, bruts', formatInteger(counters.sectors_written)]
      ]));
    });
    if (devices.length) {
      diskBody.appendChild(grid);
    } else {
      const empty = document.createElement('p');
      empty.className = 'muted';
      empty.textContent = 'La source disque est lisible, mais aucun périphérique réel n’est inclus dans cet extrait.';
      diskBody.appendChild(empty);
    }
  } else {
    const unavailable = document.createElement('p');
    unavailable.className = 'muted';
    unavailable.textContent = 'Le fichier local /proc/diskstats n’a pas fourni de données exploitables. Aucune activité n’est déduite.';
    diskBody.appendChild(unavailable);
  }
  diskDetails.append(diskSummary, diskBody);
  cards.push(diskDetails);
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
  if (category.id === 'future_lab') renderFutureLab(cards, report);
  if (category.id === 'storage') {
    const groups = new Map();
    (report.storage_inventory?.volumes || []).forEach(volume => {
      const key = physicalDiskKey(volume);
      groups.set(key, [...(groups.get(key) || []), volume]);
    });
    if (groups.size) {
      const overview = document.createElement('section');
      overview.className = 'storage-overview';
      const overviewHead = document.createElement('div');
      overviewHead.className = 'storage-overview-head';
      const overviewIcon = document.createElement('span');
      overviewIcon.setAttribute('aria-hidden', 'true');
      overviewIcon.textContent = '🗂️';
      const overviewCopy = document.createElement('div');
      const title = document.createElement('h3');
      title.textContent = 'Vos disques, en un coup d’œil';
      const hint = document.createElement('p');
      hint.className = 'muted';
      hint.textContent = 'Une carte représente un disque physique. Ouvrez-la pour comprendre chacune de ses partitions.';
      overviewCopy.append(title, hint);
      overviewHead.append(overviewIcon, overviewCopy);
      const navigator = document.createElement('div');
      navigator.className = 'disk-navigator';
      groups.forEach((volumes, disk) => {
        const metrics = diskMetrics(volumes);
        const id = `disk-${disk.replace(/[^a-z0-9]/gi, '')}`;
        const button = document.createElement('button');
        button.type = 'button';
        button.className = 'disk-shortcut';
        button.addEventListener('click', () => {
          const target = document.querySelector(`#${id}`);
          if (target instanceof HTMLDetailsElement) target.open = true;
          target?.scrollIntoView({ behavior: 'smooth', block: 'start' });
        });
        const shortcutHead = document.createElement('span');
        shortcutHead.className = 'disk-shortcut-head';
        const diskIcon = document.createElement('span');
        diskIcon.className = 'disk-shortcut-icon';
        diskIcon.setAttribute('aria-hidden', 'true');
        diskIcon.textContent = '💽';
        const shortcutCopy = document.createElement('span');
        const label = document.createElement('strong');
        label.textContent = disk || 'Disque inconnu';
        const capacity = document.createElement('small');
        capacity.textContent = metrics.totalBytes > 0
          ? `${formatBytes(metrics.totalBytes)} en partitions détectées`
          : 'Capacité non communiquée';
        shortcutCopy.append(label, capacity);
        shortcutHead.append(diskIcon, shortcutCopy);
        appendPartitionStrip(button, volumes, id);
        const state = document.createElement('span');
        state.className = 'disk-shortcut-state';
        const access = document.createElement('small');
        access.textContent = `${metrics.mountedCount}/${volumes.length} accessible${metrics.mountedCount === 1 ? '' : 's'}`;
        const usage = document.createElement('small');
        usage.textContent = metrics.usedPercent === null ? 'Occupation inconnue' : `${metrics.usedPercent} % utilisés sur les partitions accessibles`;
        state.append(access, usage);
        if (volumes.some(volume => volume.windows_protected)) {
          const protectedBadge = document.createElement('span');
          protectedBadge.className = 'disk-protected-badge';
          protectedBadge.textContent = '🪟 Windows protégé';
          state.appendChild(protectedBadge);
        }
        button.prepend(shortcutHead);
        button.appendChild(state);
        navigator.appendChild(button);
      });
      const overviewNote = document.createElement('small');
      overviewNote.className = 'storage-capacity-note';
      overviewNote.textContent = 'ℹ️ Les capacités additionnent les partitions connues. Un éventuel espace non partitionné n’est pas inventé.';
      overview.append(overviewHead, navigator, overviewNote);
      cards.push(overview);
    }
    groups.forEach((volumes, disk) => {
      const metrics = diskMetrics(volumes);
      const group = document.createElement('details');
      group.className = 'volume-group';
      const diskId = `disk-${disk.replace(/[^a-z0-9]/gi, '')}`;
      group.id = diskId;
      const heading = document.createElement('summary');
      heading.className = 'volume-group-summary';
      const headingIcon = document.createElement('span');
      headingIcon.className = 'volume-group-icon';
      headingIcon.setAttribute('aria-hidden', 'true');
      headingIcon.textContent = '💽';
      const headingCopy = document.createElement('span');
      const headingTitle = document.createElement('strong');
      headingTitle.textContent = disk || 'Disque physique inconnu';
      const headingState = document.createElement('small');
      headingState.textContent = `${formatCount(volumes.length, 'partition', 'partitions')} · ${metrics.totalBytes > 0 ? formatBytes(metrics.totalBytes) : 'taille inconnue'} · ${metrics.mountedCount} accessible${metrics.mountedCount === 1 ? '' : 's'}`;
      headingCopy.append(headingTitle, headingState);
      const headingAction = document.createElement('span');
      headingAction.className = 'volume-group-action';
      headingAction.textContent = 'Voir les partitions';
      heading.append(headingIcon, headingCopy, headingAction);
      group.addEventListener('toggle', () => {
        headingAction.textContent = group.open ? 'Replier' : 'Voir les partitions';
      });
      group.appendChild(heading);
      if (volumes.some(volume => volume.windows_protected)) {
        const warning = document.createElement('p');
        warning.className = 'dualboot-warning';
        warning.textContent = '⚠ Contenu Windows confirmé : Linux Doctor protège ce disque des suggestions automatiques. Ne l’effacez et ne le reformatez pas sans avoir identifié son contenu.';
        group.appendChild(warning);
      }
      const map = document.createElement('section');
      map.className = 'partition-map';
      const mapHead = document.createElement('div');
      mapHead.className = 'partition-map-head';
      const mapTitle = document.createElement('h4');
      mapTitle.textContent = 'Carte des partitions';
      const mapHint = document.createElement('p');
      mapHint.className = 'muted';
      mapHint.textContent = 'La longueur indique la taille relative. Les très petites partitions restent volontairement visibles.';
      mapHead.append(mapTitle, mapHint);
      map.appendChild(mapHead);
      appendPartitionStrip(map, volumes, diskId, true);
      map.appendChild(createPartitionLegend(volumes));
      group.appendChild(map);
      const groupCards = document.createElement('div');
      groupCards.className = 'partition-list';
      volumes.forEach((volume, partitionIndex) => {
        const role = partitionRole(volume);
        const state = partitionState(volume);
        const card = document.createElement('article');
        card.id = `${diskId}-partition-${partitionIndex}`;
        card.className = `volume-card role-${role.key} state-${state.key}`;
        const head = document.createElement('div');
        head.className = 'volume-card-head';
        const identity = document.createElement('div');
        identity.className = 'volume-identity';
        const roleIcon = document.createElement('span');
        roleIcon.className = 'volume-role-icon';
        roleIcon.setAttribute('aria-hidden', 'true');
        roleIcon.textContent = role.icon;
        const identityCopy = document.createElement('div');
        const position = document.createElement('small');
        position.textContent = `Partition ${partitionIndex + 1} sur ${volumes.length}`;
        const volumeTitle = document.createElement('h3');
        volumeTitle.textContent = volume.label || volume.path;
        const volumePath = document.createElement('span');
        volumePath.className = 'volume-path';
        volumePath.textContent = volume.path;
        identityCopy.append(position, volumeTitle, volumePath);
        identity.append(roleIcon, identityCopy);
        const stateBadge = document.createElement('span');
        stateBadge.className = `partition-state state-${state.key}`;
        stateBadge.textContent = `${state.icon} ${state.label}`;
        head.append(identity, stateBadge);

        const facts = document.createElement('div');
        facts.className = 'volume-facts';
        [
          { icon: '🎯', label: 'Rôle', value: role.label, detail: role.detail },
          { icon: '📏', label: 'Capacité', value: volumeSize(volume) > 0 ? formatBytes(volumeSize(volume)) : 'Inconnue', detail: volume.mounted ? `${volume.used_percent} % utilisés` : 'Occupation inconnue' },
          { icon: '🧩', label: 'Format', value: String(volume.filesystem || 'Inconnu').toUpperCase(), detail: volume.partition_label || 'Sans libellé technique' },
          { icon: '📍', label: 'Emplacement', value: volume.mountpoint || 'Pas ouverte', detail: state.detail }
        ].forEach(item => {
          const fact = document.createElement('section');
          const icon = document.createElement('span');
          icon.setAttribute('aria-hidden', 'true');
          icon.textContent = item.icon;
          const factCopy = document.createElement('span');
          const label = document.createElement('small');
          label.textContent = item.label;
          const value = document.createElement('strong');
          value.textContent = item.value;
          const detail = document.createElement('small');
          detail.textContent = item.detail;
          factCopy.append(label, value, detail);
          fact.append(icon, factCopy);
          facts.appendChild(fact);
        });

        let capacity;
        if (volume.mounted) {
          capacity = usageBar(volume.used_percent, `${volume.used_percent} % utilisés · ${formatBytes(volume.available_bytes) || 'espace libre inconnu'} libres`);
        } else {
          capacity = document.createElement('div');
          capacity.className = 'capacity capacity-unavailable';
          const unavailableBar = document.createElement('div');
          unavailableBar.className = 'capacity-bar unavailable';
          const unavailableText = document.createElement('small');
          unavailableText.className = 'muted';
          unavailableText.textContent = 'Occupation inconnue : une partition non montée n’est pas forcément vide.';
          capacity.append(unavailableBar, unavailableText);
        }

        const technical = document.createElement('details');
        technical.className = 'volume-technical';
        const technicalSummary = document.createElement('summary');
        technicalSummary.textContent = 'Afficher les détails techniques';
        const technicalList = document.createElement('dl');
        [
          ['Périphérique', volume.path],
          ['Disque parent', disk || 'Inconnu'],
          ['UUID', volume.uuid || 'Indisponible'],
          ['Libellé de partition', volume.partition_label || 'Non renseigné'],
          ['Accès', state.detail],
          ['Transport', volume.transport || 'Non renseigné'],
          ['Amovible', volume.removable ? 'Oui' : 'Non signalé comme amovible']
        ].forEach(([term, value]) => {
          const dt = document.createElement('dt');
          dt.textContent = term;
          const dd = document.createElement('dd');
          dd.textContent = value;
          technicalList.append(dt, dd);
        });
        technical.append(technicalSummary, technicalList);
        card.append(head, facts, capacity, technical);
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
    renderEnergyEstimate(cards);
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
        { label: 'Jeux installés', value: String(library.game_count ?? 0) },
        { label: 'Outils Steam', value: String(library.tool_count ?? 0) },
        { label: 'Jeux déclarés', bytes: library.game_bytes }
      ].forEach(item => {
        const pill = document.createElement('span');
        pill.textContent = evidenceText(item);
        evidence.appendChild(pill);
      });
      const applications = (inventory.games || []).filter(game => game.library_index === index);
      const games = applications.filter(game => game.kind !== 'tool');
      const tools = applications.filter(game => game.kind === 'tool');
      const renderSteamItems = (items, kind) => {
        const details = document.createElement('details');
        details.className = `steam-content-details steam-${kind}-details`;
        details.open = kind === 'games';
        const summary = document.createElement('summary');
        summary.textContent = kind === 'games'
          ? `🎮 Les ${items.length} jeux et leurs icônes`
          : `🧰 Les ${items.length} outils Steam — Proton, runtimes et composants`;
        const list = document.createElement('div');
        list.className = 'steam-game-grid';
        items.forEach(game => {
          const item = document.createElement('article');
          item.className = `steam-game steam-${kind.slice(0, -1)}`;
          const visual = document.createElement('span');
          visual.className = 'steam-game-icon';
          visual.textContent = kind === 'games' ? '🕹️' : '🧰';
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
          name.textContent = game.name || `${kind === 'games' ? 'Jeu' : 'Outil'} ${game.appid}`;
          const size = document.createElement('small');
          size.textContent = `${formatBytes(game.size_bytes)} · AppID ${game.appid}`;
          copy.append(name, size);
          item.append(visual, copy);
          list.appendChild(item);
        });
        details.append(summary, list);
        return details;
      };
      if (games.length) {
        card.append(head, evidence, renderSteamItems(games, 'games'));
      } else card.append(head, evidence);
      if (tools.length) card.append(renderSteamItems(tools, 'tools'));
      cards.push(card);
    });
    const knowledge = report.gaming_knowledge || {};
    const knowledgeCard = document.createElement('article');
    knowledgeCard.className = `gaming-knowledge-card ${knowledge.available ? 'status-info' : 'status-unknown'}`;
    const knowledgeHead = document.createElement('div');
    knowledgeHead.className = 'diagnostic-head';
    const knowledgeTitle = document.createElement('h3');
    knowledgeTitle.textContent = '📚 Base de connaissances Ubuntu Gaming';
    const knowledgeChip = document.createElement('span');
    knowledgeChip.className = `status-chip ${knowledge.available ? 'status-info' : 'status-unknown'}`;
    knowledgeChip.textContent = knowledge.available
      ? knowledge.source === 'user-update' ? 'Mise à jour utilisateur' : 'Incluse dans Linux Doctor'
      : 'Base indisponible';
    knowledgeHead.append(knowledgeTitle, knowledgeChip);
    const knowledgeSummary = document.createElement('p');
    const gameNotices = (knowledge.entries || []).filter(entry => entry.kind === 'game').length;
    knowledgeSummary.textContent = knowledge.available
      ? `${knowledge.relevant_entries || 0} conseil${knowledge.relevant_entries === 1 ? '' : 's'} correspond${knowledge.relevant_entries === 1 ? '' : 'ent'} à votre machine, dont ${gameNotices} ${gameNotices === 1 ? 'fiche de jeu' : 'fiches de jeux'}. Le diagnostic reste local : aucune connexion n’est lancée pendant l’analyse.`
      : 'La base locale de conseils gaming n’a pas été chargée ; aucun problème connu ne peut être rapproché des jeux installés.';
    knowledgeCard.append(knowledgeHead, knowledgeSummary);
    if (knowledge.available) {
      const metadata = document.createElement('div');
      metadata.className = 'knowledge-metadata';
      [
        ['🏷️', 'Version', knowledge.version || 'Non renseignée'],
        ['📅', 'Révisée le', knowledge.reviewed_on || 'Date inconnue'],
        ['🛡️', 'Origine active', knowledge.source === 'user-update' ? 'Copie personnelle validée' : 'Copie livrée avec l’application']
      ].forEach(([icon, label, value]) => {
        const item = document.createElement('span');
        const itemIcon = document.createElement('i');
        itemIcon.setAttribute('aria-hidden', 'true');
        itemIcon.textContent = icon;
        const copy = document.createElement('span');
        const term = document.createElement('small');
        term.textContent = label;
        const detail = document.createElement('strong');
        detail.textContent = value;
        copy.append(term, detail);
        item.append(itemIcon, copy);
        metadata.appendChild(item);
      });
      knowledgeCard.appendChild(metadata);

      const updateDetails = document.createElement('details');
      updateDetails.className = 'knowledge-update';
      const updateToggle = document.createElement('summary');
      updateToggle.textContent = '🌐 Vérifier et installer manuellement une base plus récente';
      const updateExplanation = document.createElement('p');
      updateExplanation.textContent = 'La commande télécharge uniquement la base officielle du projet, contrôle sa taille et son format, puis l’installe dans vos données utilisateur. Elle ne demande pas sudo et ne modifie pas le système.';
      const updateCommand = typeof knowledge.manual_update_command === 'string' && knowledge.manual_update_command === KNOWLEDGE_UPDATE_COMMAND
        ? knowledge.manual_update_command : KNOWLEDGE_UPDATE_COMMAND;
      const command = document.createElement('code');
      command.textContent = updateCommand;
      const actions = document.createElement('div');
      actions.className = 'actions';
      const copy = document.createElement('button');
      copy.type = 'button';
      copy.textContent = 'Copier la commande de mise à jour';
      const copyStatus = document.createElement('small');
      copyStatus.className = 'muted';
      copyStatus.setAttribute('role', 'status');
      copy.addEventListener('click', () => copyText(updateCommand, copyStatus));
      actions.appendChild(copy);
      const security = document.createElement('small');
      security.className = 'muted';
      security.textContent = 'Le téléchargement HTTPS protège le transport, mais la version 1.0 ne vérifie pas encore de signature cryptographique. La mise à jour reste donc volontairement manuelle.';
      updateDetails.append(updateToggle, updateExplanation, command, actions, copyStatus, security);
      knowledgeCard.appendChild(updateDetails);
    }
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
      note.textContent = 'Simulation en lecture seule : ouvrez ensuite le gestionnaire de stockage Steam pour effectuer un déplacement contrôlé.';
      selectionDetails.append(selectionToggle, selection);
      card.append(title, description, summary, selectionDetails, note);
      cards.push(card);
    }
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
const updateScrollTopButton = () => scrollTopButton?.classList.toggle('visible', window.scrollY > 320);
window.addEventListener('scroll', updateScrollTopButton, { passive: true });
updateScrollTopButton();
loadReport().catch(error => {
  diagnosticsTarget.textContent = error.message;
  setText(overviewTarget, error.message);
});
