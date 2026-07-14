# Architecture

Linux Doctor sépare collecte, diagnostic et présentation. Le moteur C17 produit un rapport JSON versionné ; l’interface statique le rend sans accès direct au système et sans inventer de diagnostic.

```text
sources Linux locales → collecteurs C → règles → rapport JSON
                                                   ↓
                                      HTML / CSS / JavaScript

action réseau volontaire → téléchargement borné → validation C → données XDG
```

## Composants

| Composant | Responsabilité |
| --- | --- |
| CLI C17 | orchestration, options, erreurs et écriture du rapport |
| Collecteurs | observations locales bornées par domaine |
| Règles | sévérités, scores, preuves, explications et recommandations |
| Rapport JSON V2 | contrat stable entre moteur, interface et tests |
| Interface web | bilans, navigation et divulgation progressive |
| Historique | comparaison de snapshots compatibles dans l’état XDG |
| Base gaming | contexte éditorial versionné, rapproché des observations locales |

Les domaines sont actuellement compilés dans un seul exécutable. [plugins.md](plugins.md) décrit le contrat souhaité pour les isoler ; il n’existe pas encore de chargement dynamique public.

## Rapport JSON V2

La 1.0 ajoute des blocs racine optionnels à V2 sans modifier le sens des champs existants. Cette extension additive reste lisible par un consommateur qui ignore les clés inconnues ; toute rupture sémantique future exigera une nouvelle version de schéma.

Les blocs principaux sont indépendants afin qu’une source indisponible ne casse pas toute l’analyse :

- `application`, `generated_at` et `schema_version` identifient le rapport ;
- `system_health`, `summary`, `categories` et `good_news` portent les conclusions ;
- `storage_inventory` décrit les volumes détectés ;
- `steam_inventory` sépare bibliothèques, jeux, outils et manettes ;
- `graphics_inventory` décrit périphériques, pilote, session et chargeurs locaux ;
- `updates_inventory` expose les candidats APT et l’état de leur sélection ;
- `apps_inventory` et `gfn_inventory` décrivent les outils gaming visibles localement ;
- `gaming_knowledge` expose uniquement les fiches pertinentes et leur provenance ;
- `future_lab` contient les instantanés bruts CPU, charge, mémoire, réseau et disque ;
- `history` compare uniquement des analyses compatibles.

Une donnée manquante produit `unknown`, une valeur nulle explicitement documentée ou un bloc indisponible. Elle ne devient jamais une réussite par défaut.

## Contrat de diagnostic

Chaque diagnostic possède un identifiant stable, une sévérité (`ok`, `info`, `warning`, `problem`, `unknown`), un résumé, des preuves, des recommandations et une explication. Le fait brut reste distinct du texte pédagogique.

```json
{
  "id": "storage.root.nearly_full",
  "severity": "warning",
  "title": "Partition système presque pleine",
  "evidence": [{"label": "Utilisation", "value": "97 %"}],
  "explanation": {
    "observed": "La partition racine utilise 97 % de sa capacité.",
    "why": "Les mises à jour et installations ont besoin d’espace libre.",
    "impact": "Une opération peut échouer faute d’espace.",
    "next_step": "Identifier d’abord les données volumineuses."
  }
}
```

Le frontend peut filtrer et reformuler la mise en page, mais il ne change ni la sévérité ni la conclusion.

## Collecte locale

- Le stockage privilégie les sources structurées et ne mesure l’occupation que d’un volume monté.
- Steam lit des manifestes bornés et des icônes locales régulières d’au plus 64 Kio ; aucune jaquette n’est téléchargée.
- La classification `game`/`tool` est prudente. Un outil n’entre pas dans une simulation de migration de jeux.
- Les familles de manettes proviennent du nom noyau ; elles ne prouvent pas le fonctionnement de Steam Input.
- Graphismes vérifie les faits locaux disponibles, pas un véritable rendu ni les performances d’un jeu.
- APT exécute des simulations à arguments fixes, sans shell, `sudo`, verrou d’écriture ou réseau. Sorties, durée et groupe de processus sont bornés.
- Future Lab lit `/proc` avec des tableaux de taille fixe. Les compteurs CPU, réseau et disque sont cumulés depuis le démarrage ; deux instantanés sont nécessaires pour un taux.

Le score global n’inclut que les catégories dont la pondération est définie et explicable. Si l’une d’elles est inconnue, `system_health.complete` devient faux et la valeur partielle est masquée.

## Données connectées

Une analyse normale n’accède jamais au réseau. `scripts/update-knowledge.sh` est une action manuelle distincte : il télécharge un fichier HTTPS borné, puis le moteur valide le schéma avant installation atomique dans `$XDG_DATA_HOME/linux-doctor`. Une copie utilisateur invalide ne doit jamais remplacer la base intégrée.

HTTPS protège le transport mais ne signe pas le contenu. La mise à jour automatique reste interdite jusqu’à l’ajout d’une signature, d’une expiration et d’un retour arrière. La politique complète est dans [data-sources.md](data-sources.md).

L’actualisation APT est également séparée : `scripts/refresh-updates.sh` laisse `/usr/bin/sudo` demander le secret dans le terminal, exécute uniquement `apt-get update`, puis régénère le rapport. Le frontend ne reçoit jamais le mot de passe.

## Historique

`--history` constitue l’activation explicite. Les snapshots compatibles sont conservés sous `$XDG_STATE_HOME/linux-doctor` ou `~/.local/state/linux-doctor`, avec une rétention bornée. Les chemins personnels, secrets et inventaires détaillés inutiles en sont exclus. Voir [history.md](history.md).

## Sécurité et évolution

- aucune réparation, écriture de `/etc/fstab` ou migration réelle implicite ;
- aucune exécution de commande système depuis le navigateur ;
- serveur de développement limité à la boucle locale ;
- chaînes, fichiers, tableaux, sorties et délais bornés ;
- données distantes datées et sourcées, rapport toujours utilisable hors ligne ;
- changement du schéma accompagné de fixtures, tests et stratégie de compatibilité.
