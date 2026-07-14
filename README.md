# Linux Doctor

Linux Doctor aide une personne ordinaire à transformer Ubuntu en machine de jeu fiable. Il vérifie les fondations locales de Steam, Proton, des manettes, de GeForce NOW, du graphisme et du stockage, puis explique quoi regarder et pourquoi cela compte.

## Version 1.0

- bilans courts et illustrés avant les inventaires détaillés ;
- partitions regroupées par disque, rôles lisibles et détails techniques repliés ;
- jeux Steam avec leurs icônes locales, séparés de Proton et des runtimes ;
- manettes reconnues par famille, avec repère Steam/Valve ;
- graphismes, session Wayland/X11 et fondations Vulkan/OpenGL ;
- simulation APT locale, rôle des paquets et commande d’actualisation explicite ;
- budget GeForce NOW sur 100 h : mensuel, annuel payé d’avance, électricité et économie ;
- base gaming versionnée, mise à jour manuellement puis validée avant installation ;
- premier Future Lab en lecture seule : CPU, charge, mémoire, réseau et activité disque brute ;
- historique local optionnel sur 30 analyses compatibles.

Une analyse normale ne contacte pas Internet. Linux Doctor ne monte aucun volume, ne lance pas `ntfsfix`, ne modifie pas `/etc/fstab`, n’installe pas de paquet et ne déplace aucun jeu. Une donnée indisponible reste inconnue.

## Lancer

Un compilateur C17 et `make` sont nécessaires.

```sh
make run
./scripts/serve.sh
```

Le rapport est écrit dans `frontend/report.json`, puis l’interface est disponible sur `http://127.0.0.1:4545`. `make run` active explicitement l’historique local ; une seconde analyse compatible permet d’afficher une comparaison.

Pour compiler et tester sans lancer l’interface :

```sh
make clean
make all
make test
```

## Actions volontaires

Actualiser les index APT puis régénérer le rapport, sans installer de paquet :

```sh
./scripts/refresh-updates.sh
```

`sudo` demande son secret directement dans le terminal. Linux Doctor ne le lit ni ne le conserve.

Vérifier une base gaming publiée sans l’installer, puis l’installer dans les données XDG de l’utilisateur :

```sh
./scripts/update-knowledge.sh --check
./scripts/update-knowledge.sh
```

Cette seconde action n’utilise pas `sudo`. Le téléchargement est borné et la copie exacte est validée avant un remplacement atomique ; la version 1.0 ne possède toutefois pas encore de signature cryptographique.

## Documentation

L’index [docs/README.md](docs/README.md) indique le document canonique de chaque sujet. Les règles de la base gaming sont dans [data/README.md](data/README.md).
