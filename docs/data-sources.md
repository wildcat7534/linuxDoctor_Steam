# Sources de données et accès réseau

Linux Doctor combine des mesures locales rapides et des données gaming téléchargées puis mises en cache. Chaque source indique sa provenance, sa date et son rôle afin que le moteur puisse distinguer un fait observé, un contexte éditorial et une reformulation.

## Carte des sources

| Source | Accès | Destination | Utilisation |
| --- | --- | --- | --- |
| `/proc`, système, Steam | local | rapport JSON | faits et diagnostics |
| snapshots Future Lab | local, environ 1 s | `future-lab-live.json` | graphiques et taux temporaires |
| base gaming | HTTPS à la demande | données XDG | contexte Ubuntu/Steam/jeux |
| modèle Gemma 3 | HTTPS à l’installation | `frontend/models` et cache navigateur | reformulation qualitative d’un snapshot |
| tarifs et veille | sources officielles datées | base intégrée/documentation | comparatifs et explications |

L’analyse normale n’envoie aucun inventaire. Une source distante indisponible conserve la dernière copie valide et expose sa date au lieu de bloquer le diagnostic.

## Provenance

Une donnée éditoriale précise : organisme et URL directe, portée, date de vérification ou d’expiration, version concernée et niveau `confirmé`, `probable` ou `hypothèse`. Une source officielle prime ; un témoignage sert à orienter une recherche, pas à créer seul une alerte générale.

## Flux Future Lab

Le flux live ne contacte pas Internet. `scripts/serve.sh` lance automatiquement le collecteur avec le serveur ; `scripts/refresh-future-lab.sh` sert à son lancement autonome et emploie un verrou pour empêcher deux producteurs concurrents. Le fichier JSON est remplacé atomiquement et un exemplaire trop ancien est rejeté par `future-lab.html`.

La timeline reste en mémoire dans la page, avec 60 points par défaut et 120 au maximum. Les débits réseau cumulent toutes les interfaces ; des couches physiques et virtuelles peuvent représenter plusieurs fois un même trafic.

## Base de connaissances gaming

`./scripts/update-knowledge.sh --check` vérifie le canal `knowledge-v1` ; `./scripts/update-knowledge.sh` installe la copie dans le répertoire XDG de l’utilisateur. Le moteur compare base intégrée et copie utilisateur valides puis sélectionne la plus récente selon sa date de révision.

Le téléchargement HTTPS est limité à 512 Kio, borné dans le temps, validé par le moteur C et installé atomiquement sans `sudo`. Le schéma 1 n’apporte pas encore de signature cryptographique. Le cycle 1.3 prévoit un manifeste signé, une expiration, un retour arrière et un mode de synchronisation automatique configurable. Le format appartient à [data/README.md](../data/README.md).

## Modèle local 1.1.0

| Élément | Valeur |
| --- | --- |
| Modèle | [`onnx-community/gemma-3-1b-it-ONNX`](https://huggingface.co/onnx-community/gemma-3-1b-it-ONNX) |
| Révision | `a58439f40017d3b99c7d378ff525e54e0ba08ebf` |
| Quantification | int8 |
| Taille indicative | environ 1,05 Go avec le tokenizer |
| Licence | Gemma |
| Runtime | [Transformers.js 4.2.0](https://huggingface.co/docs/transformers.js/) |
| Accélération | WebGPU matériel, avec WASM multithreadé en repli |
| Installation | `./scripts/setup-local-ai.sh` |

Le téléchargement est explicite, utilise la révision épinglée et place modèle et runtime dans les ressources frontend ignorées par Git. Le chargement en mémoire demande ensuite un consentement et un clic dans Future Lab.

Au clic, l’interface fige l’instantané courant et produit des constats qualitatifs déterministes. Le modèle reçoit uniquement ces constats sans chiffres et la question saisie : ni timeline, ni diagnostics, ni preuves, ni base gaming. Sa sortie est rejetée si elle contient un nombre, une commande ou s’écarte des mesures. Une fois les fichiers présents, l’inférence s’exécute dans un Worker du navigateur sans envoyer mesure, question ou réponse à un service distant.

## Tarifs GeForce NOW France

Référence vérifiée le **14 juillet 2026**, TTC, sans promotion active :

| Offre | 1 mois | 12 mois payés d’avance | Équivalent mensuel | Économie annuelle |
| --- | ---: | ---: | ---: | ---: |
| Performance | 10,99 € | 109,99 € | 9,17 € | 21,89 € (16,60 %) |
| Ultimate | 21,99 € | 219,99 € | 18,33 € | 43,89 € (16,63 %) |

Sources : [grille NVIDIA France](https://www.nvidia.com/fr-fr/geforce-now/#product-matrix) et [FAQ GeForce NOW](https://www.nvidia.com/fr-fr/geforce-now/faq/). Les offres comprennent 100 h par mois avec au plus 15 h reportées ; un paiement annuel n’est donc pas une réserve immédiate de 1 200 h.

Le calcul électrique utilise une valeur modifiable de **0,194 €/kWh TTC**, correspondant au Tarif Bleu réglementé Base 3 ou 6 kVA en France métropolitaine au 1er février 2026. Source : [Commission de régulation de l’énergie](https://www.cre.fr/consommateurs/comprendre-les-tarifs-reglementes-de-vente-delectricite-trve.html).

## Contrat réseau

- domaine et chemin autorisés par la fonction qui les utilise ;
- HTTPS, délai, taille et redirections bornés ;
- cache daté et résultat exploitable hors ligne ;
- aucun secret, chemin personnel, rapport ou identifiant matériel dans la requête ;
- validation avant activation d’une base ou d’un artefact ;
- erreur compréhensible et possibilité de supprimer les données téléchargées.

Les technologies surveillées et leurs sources sont centralisées dans la [veille technologique](technology-watch.md).
