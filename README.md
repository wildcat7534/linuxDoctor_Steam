# Linux Doctor

Outil local de diagnostic Linux qui associe une alerte à son explication : pas seulement *quoi corriger*, mais aussi **pourquoi cela compte**.

## État actuel

La V0.2 fournit un exécutable C17 qui inventorie les volumes montés ou non montés, puis les bibliothèques Steam et leurs jeux installés. Le rapport JSON V2 reste local ; le frontend le lit sans logique de diagnostic. Aucune connexion réseau n'est effectuée par le backend. Avec `--history`, les 30 dernières analyses sont conservées localement et comparées.

L'inventaire est strictement en lecture seule : il ne monte pas de volume, ne lance pas `ntfsfix`, ne modifie pas `/etc/fstab` et ne déplace aucun jeu. Ces opérations restent prévues pour des versions ultérieures avec simulation et confirmation explicite.

## Lancer

Installez un compilateur C compatible C17 et `make`, puis lancez :

```sh
make run
```

Le rapport est écrit dans `frontend/report.json`. Pour le consulter via le frontend, lancez `scripts/serve.sh`, puis ouvrez `http://127.0.0.1:4545`.

`make run` active l'historique local. Le premier lancement initialise le suivi ; le second affichera une comparaison dans le tableau de bord.

Les vérifications sont lancées avec `make test`.

## Documentation

Les décisions de produit et l'architecture sont dans [docs](docs/). La suite prioritaire est d'ajouter les collecteurs et plugins des autres catégories définies dans la feuille de route.
