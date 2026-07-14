# Linux Doctor — Future Lab

Future Lab est le laboratoire d’idées avancées de Linux Doctor, pas une promesse de tout surveiller. Sa vision reste celle d’un centre de contrôle inspiré de la science-fiction — Black Mesa, Aperture Science, Ghost in the Shell ou JARVIS — où chaque animation révèle un fait utile au lieu de décorer un terminal.

## Principe

L’utilisateur doit comprendre ce que fait sa machine sans être accusé ni inquiété à tort. Future Lab :

- observe d’abord en lecture seule ;
- sépare valeur brute, interprétation et niveau de certitude ;
- affiche `inconnu` plutôt que d’inventer ;
- ne scanne pas le réseau et ne collecte pas en continu par défaut ;
- reste utile hors ligne et n’impose jamais d’intelligence artificielle.

## Première tranche 1.0

| Domaine | Données locales | Limite affichée |
| --- | --- | --- |
| CPU | nombre logique et compteurs de planification de `/proc/stat` | compteurs cumulés depuis le démarrage, pas un pourcentage instantané |
| Charge | moyennes 1, 5 et 15 minutes de `/proc/loadavg` | charge de file d’exécution, pas un pourcentage CPU |
| Mémoire | total, disponible, utilisée, cache, buffers et swap de `/proc/meminfo` | photographie locale, sans attribution à un processus |
| Réseau | interfaces, octets, paquets, erreurs et pertes de `/proc/net/dev` | compteurs cumulés, aucune destination ni géolocalisation |
| Disques | lectures, écritures et secteurs de `/proc/diskstats` | compteurs cumulés ; partitions et volumes peuvent se recouvrir |
| Connaissance | base gaming versionnée, validée et mise à jour manuellement | HTTPS sans signature cryptographique dans la 1.0 |

Cette tranche donne des faits bruts bornés. Elle ne prétend pas encore afficher du « temps réel » : un débit ou un taux exige deux mesures horodatées comparables.

## Expériences suivantes

### Mesures et timeline

- échantillonner CPU, réseau et disque pour produire des deltas explicables ;
- relever fréquence, température, pression mémoire, TRIM et SMART lorsque les sources sont disponibles ;
- afficher une timeline locale de lancements, pics et changements, avec durée et rétention visibles ;
- ajouter GPU, VRAM, encodeur et consommation par fournisseur sans masquer les différences d’API.

### Réseau

- relier une connexion TCP/UDP à un processus avec protocole, volume et interface ;
- reconnaître VPN, WireGuard, DNS, SMB, NFS, conteneurs et machines virtuelles ;
- proposer une carte des appareils uniquement après consentement à un scan local actif ;
- expliquer une variation par rapport à une référence locale avant d’employer « inhabituel » ;
- ne jamais qualifier une connexion de malveillante sans preuve externe vérifiable.

La géolocalisation, les fabricants MAC et les réputations d’adresses nécessitent des bases externes datées. Ils restent hors ligne ou optionnels selon la politique de [sources de données](data-sources.md).

### Journaux et gaming

- filtrer noyau, systemd, Steam, Proton, Flatpak, Gamescope et pilotes ;
- rapprocher lancement d’un jeu, compilation de shaders, pression VRAM et activité disque ;
- conserver un extrait minimal et expurgé, jamais une sortie brute interminable ;
- distinguer corrélation temporelle et cause confirmée.

### Intelligence artificielle facultative

Un modèle local pourra résumer un journal, comparer des pistes ou reformuler une explication. Il ne produit pas le fait source, ne décide pas d’une réparation et n’exécute rien. Le mode règles locales doit toujours rester complet.

## Portes de sécurité avant livraison

Toute expérience doit préciser :

1. la question utilisateur à laquelle elle répond ;
2. la source locale ou distante et sa fraîcheur ;
3. le coût CPU, mémoire, disque et réseau ;
4. les données personnelles possibles et leur rétention ;
5. les faux positifs connus et le niveau de certitude ;
6. les fixtures, tests d’erreur et comportement sans dépendance ;
7. la manière de désactiver et d’effacer la fonction.

La priorité de livraison et les étapes stables appartiennent à la [feuille de route](roadmap.md). Les technologies surveillées et leur cadence appartiennent à la [veille technologique](technology-watch.md).
