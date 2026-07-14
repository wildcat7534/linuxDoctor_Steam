# Documentation Linux Doctor

La documentation décrit la **1.1.0 livrée** et identifie séparément les fonctions prévues. Un sujet possède un document canonique ; les autres pages y renvoient au lieu de recopier ses règles.

## Produit

| Document | Rôle |
| --- | --- |
| [Vision](vision.md) | promesse Gaming First et place de l’assistant local |
| [Versions](releases.md) | fonctions réellement livrées par version |
| [Feuille de route](roadmap.md) | ordre des prochaines livraisons concrètes |
| [Future Lab](future-lab.md) | fenêtre autonome, flux live, graphiques et copilote local borné |
| [Interface](ui.md) | parcours du tableau de bord et de la fenêtre Future Lab |

## Architecture et développement

| Document | Rôle |
| --- | --- |
| [Architecture](architecture.md) | moteurs C17, rapports JSON, flux live et actions |
| [Sources de données](data-sources.md) | provenance, réseau, cache et téléchargement de modèle |
| [Historique local](history.md) | snapshots conservés et comparaison |
| [Modules](plugins.md) | frontières des domaines et futur contrat d’extension |
| [Style de code](coding-style.md) | exigences C17, frontend et tests |

## Domaines gaming

| Document | Rôle |
| --- | --- |
| [Steam sur Ubuntu 26.04](steam-ubuntu-26.04.md) | signaux Steam, Proton, manettes et Wayland |
| [Stockage et bibliothèques Steam](storage-steam-roadmap.md) | diagnostic des volumes et futur parcours d’action |
| [Veille technologique](technology-watch.md) | technologies suivies et sources officielles |
| [Format de la base gaming](../data/README.md) | schéma TSV et publication |

Le [README principal](../README.md) reste le point d’entrée pour lancer et tester l’application. [history.md](history.md) conserve le périmètre de l’historique local ; [releases.md](releases.md) évite de répéter cet historique dans les documents de conception.
