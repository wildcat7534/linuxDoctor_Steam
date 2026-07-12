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

Le schéma V2 ajoute deux blocs indépendants des diagnostics :

- `storage_inventory.volumes` : partitions détectées, y compris celles qui ne sont pas montées ;
- `steam_inventory.libraries` et `steam_inventory.games` : bibliothèques Steam et manifests de jeux.

Les collecteurs ne modifient aucun volume. Une partition non montée, une bibliothèque non inscriptible ou un manifeste incomplet sont exposés comme des faits ; le diagnostic NTFS, la lecture de `fstab` et toute réparation sont différés à la V0.3 ou au-delà.

Le plan de migration V0.4 est également lecture seule : le backend choisit une destination déjà montée et inscriptible, puis une sélection de jeux permettant d'atteindre l'objectif d'espace libre. Le rapport expose cette simulation ; l'interface ne déplace aucun fichier et renvoie vers le gestionnaire de stockage Steam pour toute action réelle.

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
- Ne jamais exécuter une action corrective sans demande explicite et confirmation claire.
- Masquer ou exclure des exports les secrets, chemins sensibles et identifiants réseau lorsque nécessaire.
- Versionner le schéma JSON et conserver une compatibilité de lecture raisonnable.
