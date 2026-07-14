# Linux Doctor

Linux Doctor transforme Ubuntu 26.04 en poste de jeu compréhensible, mesurable et simple à améliorer. Il rassemble l’état de Steam, Proton, des manettes, du graphisme, des mises à jour et du stockage, puis relie chaque constat à son impact gaming.

## Version personnelle : 1.1.1

Le développement vise d’abord l’usage quotidien de sa propriétaire : les fonctions expérimentales utiles sont activées et testées ici avant toute préparation d’une édition publique plus restrictive.

- bilan illustré avant les inventaires détaillés ;
- graphismes, session Wayland/X11 et fondations Vulkan/OpenGL ;
- jeux Steam avec leurs icônes, séparés de Proton et des runtimes ;
- manettes reconnues par famille, avec repère Steam/Valve ;
- partitions regroupées par disque, rôles lisibles et détails repliables ;
- simulation APT, rôle des paquets et actualisation des index guidée ;
- budget GeForce NOW sur 100 h, avec abonnements mensuels et annuels ;
- base gaming versionnée, datée et reliée aux sources officielles ;
- historique local sur 30 analyses compatibles ;
- fenêtre Future Lab autonome avec graphiques CPU, charge, mémoire, GPU NVIDIA, réseau et disque ;
- flux local actualisé chaque seconde et timeline de 60 points par défaut, 120 au maximum ;
- copilote Gemma 3 270M facultatif, exécuté localement dans un Worker du navigateur ;

Le copilote Future Lab répond à partir des constats calculés sur l’instantané capturé au clic. Il ne reçoit ni la timeline, ni les diagnostics du rapport, ni leurs preuves, ni la base gaming. Les commandes système restent bloquées, mais le mode personnel accepte une réponse explicative plus libre.

## Lancer

Un compilateur C17, `make` et Python 3 sont nécessaires.

```sh
./scripts/serve.sh
```

Ce point d’entrée régénère `frontend/report.json`, active l’historique local, démarre automatiquement le collecteur Future Lab et sert l’interface sur `http://127.0.0.1:4545`. Son arrêt ferme aussi le collecteur live.

Pour produire uniquement le rapport, sans serveur ni flux live :

```sh
make run
```

`scripts/refresh-future-lab.sh` sert au lancement autonome du flux, par exemple pour le développer séparément. Son verrou refuse un second collecteur concurrent :

```sh
./scripts/refresh-future-lab.sh
./scripts/refresh-future-lab.sh --once
```

La fenêtre Future Lab rejette un fichier live ancien ou incompatible. Elle n’affiche un débit que si deux mesures appartiennent au même schéma, au même démarrage et à la même topologie observée.

Pour compiler et tester :

```sh
make clean
make all
make test
```

## Assistant local facultatif

L’installation télécharge la révision épinglée de Gemma 3 270M en fp16, ainsi que Transformers.js 4.2.0, dans les ressources frontend locales :

```sh
./scripts/setup-local-ai.sh
```

Le modèle représente environ 570 Mo avec le tokenizer. Le bouton **Charger le modèle** le place en mémoire ; le bouton **Poser la question** devient alors disponible. Les fichiers sont lus directement depuis le serveur local sans dépendre du cache du navigateur. L’inférence n’envoie ni mesure, ni question, ni réponse sur Internet.

Si un test navigateur interrompu a laissé des Chromium headless actifs :

```sh
./scripts/cleanup-headless.sh
```

Le script cible uniquement les profils Puppeteer de test et peut demander `sudo` pour franchir l’isolation Snap. Le serveur et la collecte GPU (`nvidia-smi`) n’ont pas besoin de root.

## Actions disponibles

Actualiser les index APT puis régénérer le rapport :

```sh
./scripts/refresh-updates.sh
```

`sudo` demande son secret directement dans le terminal. Linux Doctor ne le lit ni ne le conserve.

Vérifier puis installer la dernière base gaming publiée :

```sh
./scripts/update-knowledge.sh --check
./scripts/update-knowledge.sh
```

Le téléchargement est borné et validé avant remplacement de la copie utilisateur. L’analyse reste utilisable hors ligne et aucune donnée de la machine n’est envoyée.

## Documentation

L’index [docs/README.md](docs/README.md) présente l’édition personnelle 1.1.1 et sépare les capacités actuelles des prochains cycles. Le format de la base gaming est documenté dans [data/README.md](data/README.md).
