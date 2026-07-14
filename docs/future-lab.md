# Future Lab

Future Lab est le cockpit vivant livré avec Linux Doctor 1.1.0. Sa fenêtre autonome affiche les mesures de la machine, leurs variations valides et une reformulation locale facultative d’un instantané choisi par l’utilisateur.

## Capacités livrées

| Capacité | Expérience utilisateur |
| --- | --- |
| Fenêtre autonome | ouverture depuis le tableau de bord, lien de retour et responsive |
| Flux live local | nouvelle mesure horodatée environ chaque seconde |
| Graphiques | timeline en mémoire, 60 points par défaut et 120 maximum |
| Source explicite | état live, flux à actualiser ou photographie statique |
| Assistant local | reformulation qualitative de l’instantané capturé au clic |

## Ouvrir le cockpit

Le lancement normal suffit :

```sh
./scripts/serve.sh
```

Ce script régénère le rapport, démarre automatiquement le collecteur Future Lab et sert l’interface. Il arrête également le collecteur quand le serveur se ferme.

Pour faire fonctionner uniquement le collecteur, par exemple pendant le développement :

```sh
./scripts/refresh-future-lab.sh
```

Le script remplace atomiquement `frontend/future-lab-live.json` environ chaque seconde. Un verrou `flock` refuse une deuxième instance ; `Ctrl+C` arrête la boucle et retire le fichier live qu’elle possède. L’option `--once` écrit un seul snapshot et le conserve.

## Mesures et graphiques

| Carte | Valeur principale | Graphique |
| --- | --- | --- |
| CPU | activité entre deux compteurs `/proc/stat` | pourcentage dans le temps |
| Charge | charge 1 minute rapportée aux CPU logiques | pression relative, distincte de l’activité CPU |
| Mémoire | RAM et swap utilisés | pourcentages et évolution |
| Réseau | réception et émission cumulées | octets par seconde |
| Disques | lectures et écritures des disques physiques | opérations par seconde |

Le réseau additionne toutes les interfaces observées. Un pont, un VPN, une interface physique ou une couche de conteneur peuvent compter le même trafic à plusieurs niveaux ; le graphique représente donc l’activité cumulée de la machine, pas uniquement la connexion Internet.

Canvas dessine les courbes, tandis que les valeurs, unités et états restent présents dans le HTML. La profondeur est réglable à 30, 60 ou 120 points et `prefers-reduced-motion` réduit les animations.

## Quand un delta est valide

Le premier snapshot initialise les compteurs. Le suivant produit un taux uniquement si les deux mesures ont le même schéma, la même version, le même démarrage et des horloges compatibles.

La compatibilité est vérifiée plus finement selon la carte : même nombre de CPU logiques, mêmes noms d’interfaces réseau, mêmes identités `major:minor:nom` des disques physiques et aucune liste tronquée. Une topologie modifiée ou un compteur qui repart en arrière remet le taux en attente.

Un fichier live vieux de plus de quatre secondes est rejeté. Future Lab essaie alors `report.json`, dont la photographie peut afficher charge et mémoire mais ne sert jamais à inventer un débit.

## Assistant local

La 1.1.0 utilise Gemma 3 1B Instruct ONNX en int8 avec Transformers.js 4.2.0. Le modèle et sa révision sont détaillés dans [data-sources.md](data-sources.md).

Son installation facultative demande `hf` et `npm` :

```sh
./scripts/setup-local-ai.sh
```

Le modèle est stocké dans les ressources frontend locales. Dans Future Lab, il ne se charge en mémoire qu’après consentement et clic sur **Charger et analyser**. L’inférence s’exécute dans un Web Worker : WebGPU est retenu pour un adaptateur matériel, avec WASM multithreadé en repli.

Au clic, l’interface :

1. fige l’instantané actuellement affiché ;
2. calcule et montre ses constats qualitatifs déterministes ;
3. envoie uniquement ces constats sans chiffres et la question au modèle ;
4. valide la réponse avant de l’afficher.

Le modèle ne reçoit ni la timeline, ni les diagnostics, ni les preuves, ni les recommandations, ni la base gaming. Il ne compare pas plusieurs moments et ne connaît pas le jeu lancé.

Son rôle est limité à reformuler ce que les cartes permettent déjà de dire : niveau d’activité CPU ou mémoire, présence d’activité réseau ou disque et limites de ces observations. Il ne propose aucune commande et ne déduit aucune cause ou panne.

Toute réponse comportant un nombre, une commande système ou de gestion de paquets, ou un texte sans rapport avec CPU, charge, mémoire, réseau ou stockage est rejetée. Les constats déterministes restent affichés comme source de vérité.

Après l’installation initiale, mesure, question et réponse restent sur la machine.

## Prochaine étape

La 1.2 prévoit des sessions de jeu volontaires avec GPU, VRAM, températures et événements Steam/Proton. Leur éventuelle utilisation par une IA nécessitera un consentement et un contrat de contexte séparés ; elle ne fait pas partie de l’assistant 1.1.0.

La suite est tenue dans la [feuille de route](roadmap.md), l’implémentation dans l’[architecture](architecture.md) et la provenance dans [data-sources.md](data-sources.md).
