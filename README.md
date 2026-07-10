# Linux Doctor

Outil local de diagnostic Linux qui associe une alerte à son explication : pas seulement *quoi corriger*, mais aussi **pourquoi cela compte**.

## État actuel

Le premier jalon fournit un exécutable C17 qui analyse la capacité de la partition racine et génère un rapport JSON. Le frontend est autonome : il lit ce fichier et ne contient aucune logique de diagnostic. Aucune connexion réseau n'est effectuée par le backend.

## Lancer

Installez un compilateur C compatible C17 et `make`, puis lancez :

```sh
make run
```

Le rapport est écrit dans `frontend/report.json`. Pour le consulter via le frontend, lancez `scripts/serve.sh`, puis ouvrez `http://127.0.0.1:4545`.

Les vérifications sont lancées avec `make test`.

## Documentation

Les décisions de produit et l'architecture sont dans [docs](docs/). La suite prioritaire est d'ajouter les collecteurs et plugins des autres catégories définies dans la feuille de route.
