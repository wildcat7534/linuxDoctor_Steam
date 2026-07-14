# Base locale Ubuntu Gaming

`gaming-knowledge.tsv` appartient à Linux Doctor et n'est jamais synchronisé automatiquement. Le moteur C lit cette base en lecture seule puis conserve dans le rapport uniquement les fiches pertinentes pour les composants et AppID détectés.

Une ligne contient huit champs séparés par une tabulation :

```text
kind  target  severity  title  summary  guidance  source_url  updated_on
```

- `kind` : `game`, `steam`, `controller`, `gfn` ou `ubuntu` ;
- `target` : AppID pour un jeu, identifiant stable pour les autres familles ;
- `severity` : `info`, `warning` ou `problem` ;
- `source_url` : source HTTPS directe et vérifiable ;
- `updated_on` : date ISO `AAAA-MM-JJ` de la dernière vérification humaine.

Exemple à adapter après vérification d'une source :

```text
game<TAB>123456<TAB>warning<TAB>Titre factuel<TAB>Symptôme et versions concernées.<TAB>Étapes prudentes et réversibles.<TAB>https://source.example/ticket<TAB>2026-07-14
```

Une mise à jour assistée par ChatGPT doit toujours relire la source, dater la fiche et éviter toute affirmation générale à partir d'un témoignage isolé. Aucun secret, chemin personnel ou journal utilisateur ne doit être ajouté à cette base.
