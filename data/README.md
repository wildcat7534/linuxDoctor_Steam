# Base locale Ubuntu Gaming

`gaming-knowledge.tsv` appartient à Linux Doctor. Le moteur C lit cette base en lecture seule puis conserve dans le rapport uniquement les fiches pertinentes pour les composants et AppID détectés.

La version 1.0 peut vérifier ou installer manuellement la copie publiée sur le dépôt officiel :

```sh
./scripts/update-knowledge.sh --check
./scripts/update-knowledge.sh
```

Le téléchargement HTTPS provient de la branche de données `knowledge-v1`, réservée au schéma 1. Il est limité à 512 Kio, copié dans un temporaire privé, validé par le parseur C puis installé atomiquement dans `$XDG_DATA_HOME/linux-doctor` (ou `~/.local/share/linux-doctor`). Aucun `sudo` n'est utilisé. Une copie utilisateur absente, invalide ou plus ancienne ne masque jamais la base intégrée.

Une ligne contient exactement huit champs séparés par une tabulation :

```text
kind  target  severity  title  summary  guidance  source_url  updated_on
```

Trois commentaires de métadonnées sont obligatoires pour une mise à jour installable :

```text
# linux-doctor-knowledge-schema<TAB>1
# linux-doctor-knowledge-version<TAB>1.0.0
# reviewed-on<TAB>2026-07-14
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

Une mise à jour assistée par ChatGPT doit toujours relire la source, dater la fiche et éviter toute affirmation générale à partir d'un témoignage isolé. Aucun secret, chemin personnel ou journal utilisateur ne doit être ajouté à cette base. La validation de schéma et HTTPS ne remplacent pas encore une signature cryptographique : seules les données du dépôt officiel doivent être acceptées.
