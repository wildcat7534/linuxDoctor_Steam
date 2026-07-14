# Historique local

L’historique permet de vérifier qu’une action améliore réellement la machine. `make run` active `--history` et conserve les 30 analyses compatibles les plus récentes dans :

```text
$XDG_STATE_HOME/linux-doctor/snapshots-v2.csv
```

Sans `XDG_STATE_HOME`, le chemin devient `~/.local/state/linux-doctor/snapshots-v2.csv`.

## Données actuelles

Le format introduit en 1.0 reste celui de la 1.1.0. Chaque ligne contient l’horodatage, le score global et le remplissage de la partition racine. Le fichier privé utilise les permissions `0600`. Les chemins personnels, réseaux, sorties de commandes et inventaires détaillés ne sont pas enregistrés.

À partir de la deuxième analyse compatible, l’interface affiche le score précédent, son écart et l’évolution de `/`. Une analyse dont le score est incomplet n’ajoute pas de point comparable.

## Extension 1.2–1.3

- bouton d’effacement dans l’interface ;
- identité locale de machine et de partition pour éviter les comparaisons incohérentes ;
- diagnostics apparus, résolus ou inchangés ;
- marqueurs d’action pour comparer avant/après ;
- sessions Future Lab bornées associées volontairement à un jeu ;
- durée et volume conservés visibles avant l’enregistrement.

La timeline live de Future Lab 1.1.0 reste en mémoire et n’entre pas automatiquement dans cet historique. La 1.2 prévoit une session enregistrable afin que l’utilisateur choisisse précisément le jeu et la durée à comparer.

En 1.1.0, l’effacement consiste à supprimer `snapshots-v2.csv`. L’ancien `snapshots.csv`, s’il existe, reste indépendant.
