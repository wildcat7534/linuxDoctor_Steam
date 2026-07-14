# Future Lab

Future Lab est le cockpit vivant de Linux Doctor 1.1.1. Sa fenêtre autonome affiche les mesures de la machine, leurs variations valides et une réponse locale facultative fondée sur un instantané choisi par l’utilisateur.

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
| GPU NVIDIA | activité, VRAM, température et puissance | activité et VRAM dans le temps |
| Réseau | réception et émission cumulées | octets par seconde |
| Disques | lectures et écritures des disques physiques | opérations par seconde |

Le réseau additionne toutes les interfaces observées. Un pont, un VPN, une interface physique ou une couche de conteneur peuvent compter le même trafic à plusieurs niveaux ; le graphique représente donc l’activité cumulée de la machine, pas uniquement la connexion Internet.

Canvas dessine les courbes, tandis que les valeurs, unités et états restent présents dans le HTML. La profondeur est réglable à 30, 60 ou 120 points et `prefers-reduced-motion` réduit les animations.

## Quand un delta est valide

Le premier snapshot initialise les compteurs. Le suivant produit un taux uniquement si les deux mesures ont le même schéma, la même version, le même démarrage et des horloges compatibles.

La compatibilité est vérifiée plus finement selon la carte : même nombre de CPU logiques, mêmes noms d’interfaces réseau, mêmes identités `major:minor:nom` des disques physiques et aucune liste tronquée. Une topologie modifiée ou un compteur qui repart en arrière remet le taux en attente.

Un fichier live vieux de plus de quatre secondes est rejeté. Future Lab essaie alors `report.json`, dont la photographie peut afficher charge et mémoire mais ne sert jamais à inventer un débit.

## Assistant local

La 1.1.1 utilise Gemma 3 270M Instruct ONNX en fp16 avec WebGPU ou le repli WASM, via Transformers.js 4.2.0. Le modèle et sa révision sont détaillés dans [data-sources.md](data-sources.md).

Son installation facultative demande `hf` et `npm` :

```sh
./scripts/setup-local-ai.sh
```

Le modèle est stocké dans les ressources frontend locales. **Charger le modèle** prépare le moteur, puis **Poser la question** l’interroge sans case de consentement intermédiaire dans l’édition personnelle. L’inférence s’exécute dans un Web Worker : WebGPU est retenu pour un adaptateur matériel, avec WASM multithreadé en repli. Le cache navigateur du runtime est désactivé car les fichiers sont déjà servis localement.

Au clic, l’interface :

1. fige l’instantané actuellement affiché ;
2. calcule et montre ses constats qualitatifs déterministes ;
3. envoie uniquement ces constats sans chiffres et la question au modèle ;
4. valide la réponse avant de l’afficher.

Le modèle ne reçoit ni la timeline, ni les diagnostics, ni les preuves, ni les recommandations, ni la base gaming. Il ne compare pas plusieurs moments et ne connaît pas le jeu lancé.

Son contexte reste celui des cartes : activité CPU, mémoire, GPU, réseau et disque. L’édition personnelle autorise une réponse plus naturelle, mais le copilote ne lance aucune commande.

Une réponse vide, démesurée, proposant une commande système ou contredisant directement les niveaux calculés est rejetée. Les constats déterministes restent affichés comme source de vérité.

Après l’installation initiale, mesure, question et réponse restent sur la machine.

## Prochaine étape

La suite ajoutera les fréquences, les sessions de jeu et les événements Steam/Proton. Le GPU, la VRAM, la température et la puissance NVIDIA sont déjà visibles en direct.

La suite est tenue dans la [feuille de route](roadmap.md), l’implémentation dans l’[architecture](architecture.md) et la provenance dans [data-sources.md](data-sources.md).
