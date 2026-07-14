# Linux Doctor

Assistant local pour transformer un PC Ubuntu en machine de jeu fiable et compréhensible. Linux Doctor vérifie les fondations de Steam, Proton, des manettes, de GeForce NOW, du graphisme et du stockage, puis explique simplement *quoi regarder* et **pourquoi cela compte**.

## État actuel

La V0.8 fournit un exécutable C17 qui inventorie les volumes montés ou non montés, les bibliothèques Steam et leurs jeux installés, les manettes reconnues par le noyau, GeForce NOW, une première sélection d'applications utiles, le socle graphique local et les mises à jour candidates APT. Chaque domaine commence par un petit bilan illustré et une explication de son score avant les listes techniques. Les jeux, visibles par défaut, utilisent les icônes 32×32 déjà présentes dans le cache Steam ; elles sont intégrées au rapport sans exposer leur chemin local et sans téléchargement.

Les manettes Steam/Valve, Xbox, PlayStation, Nintendo et 8BitDo reçoivent un repère visuel dédié lorsque leur nom noyau permet de les reconnaître ; les autres restent affichées avec un badge générique. Cette détection confirme la présence du périphérique, pas le fonctionnement de toutes ses touches dans Steam Input. Les scores **Gaming** et **Graphismes** à 95 % explicitent désormais les 5 % réservés aux essais réels qui ne sont pas encore exécutés : lancement d'un jeu, rendu Vulkan/OpenGL et validation du profil de manette.

Une première base de connaissances gaming, [data/gaming-knowledge.tsv](data/gaming-knowledge.tsv), reste stockée avec l'application. Elle rapproche les fiches générales et les AppID documentés des jeux réellement installés. Elle ne contacte pas Internet : chaque fiche possède une date et une source, puis peut être enrichie lors d'une future mise à jour du projet.

La catégorie **Mises à jour** simule APT en lecture seule à partir des index déjà présents, distingue les candidats prêts, différés, retenus manuellement ou en déploiement progressif, puis affiche leur rôle depuis les métadonnées locales. Les longues descriptions techniques, souvent en anglais, restent repliées jusqu'à ce que la personne les demande. Ce rôle n'est pas le journal des changements de la version. Le rapport JSON V2 reste local ; le frontend le lit sans logique de diagnostic. Aucune connexion réseau n'est effectuée par les collecteurs. Avec `--history`, les 30 dernières analyses sont conservées localement et comparées.

L'inventaire est strictement en lecture seule : il ne monte pas de volume, ne lance pas `ntfsfix`, ne modifie pas `/etc/fstab` et ne déplace aucun jeu. Ces opérations restent prévues pour des versions ultérieures avec simulation et confirmation explicite.

## Lancer

Installez un compilateur C compatible C17 et `make`, puis lancez :

```sh
make run
```

Le rapport est écrit dans `frontend/report.json`. Pour le consulter via le frontend, lancez `scripts/serve.sh`, puis ouvrez `http://127.0.0.1:4545`.

`make run` active l'historique local. Le premier lancement initialise le suivi ; le second affichera une comparaison dans le tableau de bord.

Depuis la racine du projet, pour actualiser volontairement les index APT avant de régénérer le rapport :

```sh
./scripts/refresh-updates.sh
```

`sudo` demande alors, si nécessaire, le mot de passe directement dans le terminal. Linux Doctor ne le lit et ne le conserve jamais. Le script exécute uniquement `apt-get update`, puis régénère atomiquement `frontend/report.json` ; il n'installe aucun paquet. L'interface statique affiche et copie cette commande, mais ne peut pas ouvrir elle-même un dialogue sudo sûr.

Les vérifications sont lancées avec `make test`.

## Documentation

Les décisions de produit et l'architecture sont dans [docs](docs/). La suite prioritaire est d'ajouter les collecteurs et plugins des autres catégories définies dans la feuille de route.
