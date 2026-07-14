# Historique local

## Périmètre livré en 1.0

L’historique est un suivi volontaire et local, pas une collecte continue. Il n’est activé que lorsque Linux Doctor est lancé avec `--history` ; `make run` utilise explicitement cette option.

Chaque analyse compatible ajoute une ligne privée dans :

```text
$XDG_STATE_HOME/linux-doctor/snapshots-v2.csv
```

Si `XDG_STATE_HOME` est absent, le chemin devient `~/.local/state/linux-doctor/snapshots-v2.csv`. Chaque ligne contient uniquement :

- l’horodatage Unix de l’analyse ;
- le score global courant ;
- le pourcentage utilisé de la partition racine.

Les chemins personnels, noms de réseaux, diagnostics détaillés et sorties de commandes ne sont pas enregistrés. Le fichier est créé avec les permissions `0600` et conserve au plus les 30 dernières lignes.

## Comparaison affichée

À partir de la deuxième analyse compatible, le rapport expose le score précédent, son écart et l’évolution du remplissage de `/`. L’interface ne prétend pas encore identifier les problèmes apparus ou résolus : ce rapprochement demanderait de conserver des identifiants et états que la 1.0 n’enregistre pas.

Si le score courant est incomplet, par exemple hors d’une session graphique reconnue, aucune ligne n’est ajoutée et la comparaison est marquée incompatible. Une première analyse signale simplement qu’aucun point précédent n’existe.

## Effacement et limites

La 1.0 ne possède pas encore de bouton ni de commande Linux Doctor pour effacer l’historique. La personne peut supprimer manuellement `snapshots-v2.csv` depuis son gestionnaire de fichiers ou son terminal. L’ancien `snapshots.csv`, s’il existe, n’est ni lu ni modifié.

Linux Doctor ne détecte pas encore un changement de machine, de partition racine ou d’horloge incohérente. Ces garde-fous, l’effacement intégré et une comparaison par diagnostic sont des évolutions futures ; ils ne doivent pas être présentés comme disponibles.
