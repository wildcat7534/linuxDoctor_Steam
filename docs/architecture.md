# Architecture

## Vue d'ensemble

Linux Doctor sépare strictement le diagnostic de l'interface : un exécutable C17 collecte et analyse la machine, puis produit un rapport JSON versionné. Le frontend lit ce fichier sans connaître le système ni les règles de diagnostic.

```text
Système Linux → collecteurs C → règles de diagnostic → rapport JSON
                                                       ↓
                                          interface HTML/CSS/JS
                                                       ↓
                                                export HTML / PDF / JSON
```

Le backend est un outil CLI, par exemple `linux-doctor --output report.json`. Le frontend peut être servi par n'importe quel serveur statique local ; aucune ressource distante n'est requise.

## Schéma V2 — inventaires lecture seule

Le schéma V2 ajoute des blocs indépendants des diagnostics :

- `storage_inventory.volumes` : partitions détectées, y compris celles qui ne sont pas montées ;
- `steam_inventory.libraries`, `steam_inventory.games` et `steam_inventory.controllers` : bibliothèques Steam, manifests de jeux et manettes reconnues par leur nom noyau.
- `graphics_inventory.devices` : cartes DRM, identifiants PCI et pilotes noyau exposés par sysfs ;
- `graphics_inventory.session`, `vulkan` et `opengl` : contexte de session et présence des chargeurs locaux, sans test de rendu.
- `updates_inventory` : fraîcheur des index APT, candidats, versions, origine, rôle local, paquets retenus et état `ready`, `phased`, `deferred` ou `unknown`.
- `gaming_knowledge` : état de la base locale, nombre de fiches et seules fiches pertinentes pour les composants ou AppID détectés ;
- `gfn_inventory` : présence locale de l'application GeForce NOW et contexte utile, sans vérification réseau.

Le champ racine `generated_at` contient l'heure UTC de génération au format ISO 8601. Le tableau `good_news` expose uniquement des conclusions positives explicitement étayées ; le frontend ne les déduit pas lui-même.

Les collecteurs ne modifient aucun volume. Une partition non montée, une bibliothèque non inscriptible ou un manifeste incomplet sont exposés comme des faits ; le diagnostic NTFS, la lecture de `fstab` et toute réparation sont différés à la V0.3 ou au-delà.

Un disque reçoit le marqueur `windows_protected` seulement lorsqu'un volume monté contient réellement `Windows/System32`. Les types de partition Microsoft, une partition de récupération ou une simple partition NTFS restent des indices insuffisants : ils ne déclenchent ni avertissement dual boot ni exclusion automatique. Lorsqu'il est confirmé, le disque est exclu des destinations automatiques de migration et l'interface demande de ne pas l'effacer ou le reformater.

Le plan de migration V0.4 est également lecture seule : le backend choisit une destination déjà montée et inscriptible, puis une sélection de jeux permettant d'atteindre l'objectif d'espace libre. Le rapport expose cette simulation ; l'interface ne déplace aucun fichier et renvoie vers le gestionnaire de stockage Steam pour toute action réelle.

Le socle graphique V0.6 reste lui aussi factuel. La présence de `libvulkan.so.1`, de fichiers manifestes ICD candidats ou de `libGL.so.1` ne prouve ni la validité complète du manifeste, ni qu'un contexte de rendu peut être créé, ni les performances, ni la compatibilité d'un jeu. Ces limites apparaissent dans chaque diagnostic concerné.

L'inventaire APT V0.6 exécute deux simulations `apt-get` locales avec des arguments fixes : une résolution complète en lecture seule inventorie les candidats, y compris les déploiements progressifs et changements de dépendances ; une simulation `upgrade` sans suppression reflète ce qu'APT sélectionnerait actuellement. `apt-mark showhold` empêche de confondre un paquet retenu avec un simple phasage, puis `apt-cache` enrichit chaque candidat avec les métadonnées déjà téléchargées. Un cache n'est déclaré disponible que si un véritable fichier d'index existe. Les processus ont une locale fixe, une sortie bornée, un groupe de processus isolé et un délai d'expiration ; aucun shell, sudo, téléchargement ou verrou d'écriture n'est utilisé pendant la collecte. Une sortie inconnue ou incomplète produit `unknown`, jamais « aucune mise à jour ». Une description APT explique le rôle du paquet, mais pas nécessairement ce que la nouvelle version corrige. Une provenance `*-security` est signalée factuellement sans inventer de criticité ou de CVE.

En V0.7, le collecteur Steam cherche pour chaque AppID une petite icône JPEG déjà mise en cache par le client Steam. Seuls les fichiers réguliers au nom attendu et d'au plus 64 Kio sont retenus. Le rapport encode leur contenu en URI `data:` : il ne publie ni chemin personnel, ni requête vers un CDN. Une icône générique est utilisée lorsque Steam ne possède pas d'image locale.

En V0.8, l'inventaire des manettes expose un nom et une famille visuelle (`steam`, `xbox`, `playstation`, `nintendo`, `8bitdo` ou `generic`). La classification repose uniquement sur le nom déclaré au noyau, déduplique les interfaces d'un même Steam Controller et ne réalise aucun test d'entrée. Le frontend choisit les badges et icônes à partir de cette famille, sans inventer de compatibilité Steam Input.

La base `data/gaming-knowledge.tsv` est une ressource locale versionnée. Le module C valide ses champs bornés et rapproche les cibles `game`, `steam`, `controller`, `gfn` ou `ubuntu` des observations locales. Le rapport n'exporte que les fiches pertinentes. Cette base apporte du contexte pédagogique ; elle ne remplace ni un test réel du jeu, ni la lecture d'un ticket récent, ni une vérification humaine de la date et de la source.

L'actualisation des index est une action séparée et explicite : `scripts/refresh-updates.sh` lance la commande fixe `/usr/bin/sudo -- /usr/bin/apt-get update` dans un terminal, puis régénère le rapport avec un fichier temporaire unique et un renommage. Le mot de passe reste lu par `sudo`. Le frontend statique ne reçoit aucun secret et n'exécute aucune commande privilégiée.

En V0.6, `system_health.scope` indique les catégories qui contribuent réellement au score global : `storage` et `graphics`. Le score retient la conclusion la plus faible de ces deux catégories afin qu'un avertissement graphique ne soit pas masqué par un stockage sain. Les autres catégories conservent leur score propre jusqu'à la définition d'une pondération globale explicable.

Depuis la V0.8, chaque catégorie exporte aussi `score_explanation`. Ce texte explique ce qui est mesuré et ce qui manque encore au score. Ainsi, 95 % en graphismes signifie que les pilotes, chargeurs et contexte local ont été détectés, avec 5 % réservés à un véritable rendu et à la validation des versions ; 95 % en gaming réserve de même les essais réels de lancement, Proton et Steam Input. Une catégorie inconnue reste affichée sans valeur numérique.

`system_health.complete` passe à `false` dès qu'une catégorie du périmètre est `unknown`, même si une autre expose parallèlement un problème confirmé. Le problème reste visible, mais l'interface masque alors la valeur numérique partielle et l'historique suspend la comparaison.

## Composants

| Composant | Responsabilité |
| --- | --- |
| Noyau C | Orchestration, score global, production de rapport et exports. |
| Plugins | Collecte d'un domaine, évaluation des règles et production de résultats normalisés. |
| Moteur de règles | Transforme des observations factuelles en diagnostics, sévérités et recommandations. |
| Rapport JSON | Contrat stable entre backend, interface et exports. |
| Interface web | Lit le fichier JSON, présente les filtres, détails techniques et panneaux pédagogiques. Elle ne diagnostique pas. |
| Historique local | Snapshots normalisés et résumés de changements, conservés uniquement sur la machine. |

## Contrat de diagnostic

Chaque diagnostic doit contenir au minimum :

```json
{
  "id": "storage.root.nearly_full",
  "severity": "warning",
  "title": "Partition système presque pleine",
  "summary": "Il reste 6 Go sur la partition racine.",
  "evidence": [{ "label": "Utilisation", "value": "97 %" }],
  "recommendations": [{ "label": "Libérer de l'espace", "priority": "high" }],
  "explanation": {
    "why": "Les SSD et les mises à jour ont besoin d'espace libre pour fonctionner confortablement.",
    "impact": "Les installations peuvent échouer et le système devenir plus difficile à maintenir.",
    "learn_more": "storage.free-space"
  }
}
```

`id` est stable et sert aux liens, tests, traductions et explications. Les valeurs brutes restent distinctes des textes affichés. Un diagnostic inconnu ou incomplet doit l'indiquer au lieu d'inférer un état sain.

## Historique local

L'historique est activé explicitement. Le backend compare l'analyse courante à la dernière analyse compatible et ajoute un résumé de différences au rapport : score, sévérités, valeurs suivies et diagnostics apparus ou résolus. Le frontend affiche ces données, mais ne calcule pas les diagnostics lui-même.

Les snapshots sont conservés sous le répertoire d'état XDG (`$XDG_STATE_HOME/linux-doctor`, ou `~/.local/state/linux-doctor`). Ils ne contiennent ni chemins personnels, ni secrets, ni inventaire détaillé inutile. La première version conserve les 30 analyses récentes. Une agrégation mensuelle pourra compléter cette rétention lorsqu'elle apportera une vraie valeur. La suppression complète de l'historique doit être possible sans privilège.

Un historique est utile seulement si la comparaison est fiable : une modification de schéma, de machine ou de règle doit être signalée comme telle, jamais présentée comme une régression système.

## Sécurité et confidentialité

- Écouter seulement sur la boucle locale par défaut.
- Ne jamais demander, transmettre ou stocker un mot de passe sudo dans le frontend ou le rapport.
- Ne jamais exécuter une action corrective sans demande explicite et confirmation claire.
- Masquer ou exclure des exports les secrets, chemins sensibles et identifiants réseau lorsque nécessaire.
- Versionner le schéma JSON et conserver une compatibilité de lecture raisonnable.
