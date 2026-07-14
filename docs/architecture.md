# Architecture

Linux Doctor 1.1.0 associe un moteur C17 déterministe, deux interfaces web statiques, un flux Future Lab local et un copilote facultatif exécuté dans le navigateur. Le modèle ne participe ni à la collecte, ni aux calculs, ni au diagnostic.

## Processus lancés

```text
scripts/serve.sh
├── make run ───────────────────────────────> frontend/report.json
├── refresh-future-lab.sh ── verrou ───────> frontend/future-lab-live.json
│                                  remplacement atomique environ chaque seconde
└── serveur HTTP Python sur 127.0.0.1:4545
    └── en-têtes COOP/COEP pour WebAssembly multithreadé
```

`scripts/serve.sh` est le point d’entrée normal : il lance le rapport, le collecteur live et le serveur, puis arrête les processus enfants à sa fermeture. `scripts/refresh-future-lab.sh` sert au lancement autonome. Il acquiert un verrou non bloquant avec `flock`, refuse un second collecteur et retire son fichier live lors de l’arrêt normal du mode continu.

Il n’existe pas d’endpoint dynamique ni de serveur d’événements. La page interroge les deux fichiers JSON statiques avec le cache HTTP désactivé.

## Composants

| Composant | Responsabilité |
| --- | --- |
| CLI C17 | collecte, diagnostics, options et sérialisation |
| Modules métier | stockage, Steam, graphismes, APT, applications, connaissance et Future Lab |
| Rapport JSON V2 | contrat complet entre le moteur et le tableau de bord |
| Snapshot Future Lab | mesure autonome horodatée destinée au rafraîchissement rapide |
| Tableau de bord | priorités gaming, preuves, recommandations et actions existantes |
| Fenêtre Future Lab | validation du flux, deltas, graphiques et lecture factuelle |
| Assistant local | reformulation qualitative d’un seul instantané capturé au clic |
| Historique | comparaison de rapports compatibles dans l’état XDG |
| Base gaming | contexte éditorial versionné du tableau de bord |

La base gaming et les diagnostics du rapport ne sont pas des entrées de l’assistant Future Lab 1.1.0.

## Rapport et snapshot live

Le rapport JSON V2 porte les conclusions durables de l’analyse : santé, catégories, stockage, Steam, graphismes, APT, applications, GeForce NOW, base gaming, historique et photographie Future Lab.

`linux-doctor --future-lab-json` collecte exclusivement Future Lab et produit un document `linux-doctor.future-lab.live` version 1. Il contient l’UTC, le temps Unix, une horloge monotone, l’identifiant de démarrage et les compteurs bruts CPU, réseau et disque. Le backend ne calcule aucun débit.

## Acceptation d’un flux

La fenêtre rejette un fichier live si son schéma ou sa version diffère, si l’identifiant de démarrage est absent, si son horodatage n’est pas fiable ou s’il date de plus de quatre secondes. Une avance d’horloge supérieure à dix secondes est également refusée. En l’absence de live accepté, `report.json` fournit uniquement une photographie statique.

Deux snapshots live ne produisent des deltas que s’ils partagent :

- le schéma et sa version ;
- le même identifiant de démarrage ;
- un intervalle monotone compris entre 0,2 et 15 secondes ;
- le même nombre de CPU logiques pour le taux CPU ;
- le même ensemble d’interfaces réseau, identifié par des noms uniques et non tronqué ;
- le même ensemble de disques physiques, identifié par `major:minor:nom` et non tronqué.

Un compteur qui diminue, une identité manquante ou une topologie différente invalide le taux concerné. Les valeurs directes, comme la charge ou l’occupation mémoire, restent affichables sans delta.

Le débit réseau additionne toutes les interfaces rapportées par `/proc/net/dev`. Une interface physique, un pont, un VPN ou une couche de conteneur peuvent représenter le même trafic à plusieurs niveaux : ce cumul décrit l’activité observée, pas le débit de la seule connexion Internet.

Les taux disque utilisent uniquement les périphériques physiques reconnus. Les partitions et volumes logiques restent visibles dans les détails mais ne sont pas ajoutés au graphique de débit.

## Assistant local 1.1.0

`scripts/setup-local-ai.sh` installe la révision épinglée de Gemma 3 1B Instruct int8 et Transformers.js 4.2.0. Le manifeste local doit correspondre exactement au modèle, à la révision, à la quantification et au runtime attendus. L’identifiant, la taille et la licence sont centralisés dans [data-sources.md](data-sources.md).

Le modèle ne se charge qu’après consentement et clic. À cet instant, l’interface fige la mesure courante puis calcule des constats qualitatifs déterministes pour CPU, charge, RAM, swap, réseau et disque. Seuls ces constats sans valeur numérique, accompagnés de la question saisie, entrent dans le prompt.

Le contexte n’inclut pas :

- la timeline ou les snapshots précédents ;
- les diagnostics, scores, preuves ou recommandations du rapport ;
- les fiches de la base gaming ;
- les inventaires détaillés des interfaces ou disques ;
- une commande à exécuter.

Le système demande une reformulation française courte, sans cause inventée ni commande. Après génération, un validateur rejette toute réponse contenant un chiffre, une commande système ou de gestion de paquets, ou aucun terme lié aux mesures Future Lab. Une réponse rejetée est remplacée par la lecture factuelle déterministe.

## Actions séparées

Les actions APT et base gaming du tableau de bord restent indépendantes de Future Lab. Le copilote local ne propose et n’exécute aucune commande. Toute future connexion entre une mesure et une action demandera un contrat distinct, documenté et testé.

## Ressources et fiabilité

- collecteur live protégé par verrou et arrêt coordonné avec le serveur ;
- fichiers JSON remplacés atomiquement ;
- timeline en mémoire de 60 points par défaut, plafonnée à 120 ;
- modèle chargé uniquement à la demande et libéré à la fermeture de la page ;
- inférence isolée dans un Web Worker pour préserver la fluidité de l’interface ;
- WebGPU utilisé avec un adaptateur matériel, WASM multithreadé en repli ;
- valeurs HTML disponibles même si Canvas ou le modèle échoue ;
- fonctionnement du diagnostic et des graphiques statiques sans IA.

La provenance des artefacts et les accès réseau sont détaillés dans [data-sources.md](data-sources.md).
