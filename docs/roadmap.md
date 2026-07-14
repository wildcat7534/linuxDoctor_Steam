# Feuille de route

L’édition personnelle 1.1.1 et son Future Lab vivant sont livrés. Chaque cycle suivant doit compléter le parcours **diagnostiquer → expliquer → agir → mesurer**.

## 1.2 — Gaming Readiness et session expliquée

- identifier la provenance de Steam et ses runtimes ;
- vérifier Vulkan 64/32 bits, GameMode, MangoHud, Gamescope, DXVK et VKD3D-Proton ;
- relier manettes, règles `steam-devices` et test Steam Input ;
- démarrer volontairement une session de mesure liée à un jeu ;
- ajouter fréquences GPU/VRAM, pression mémoire avancée et fournisseurs non NVIDIA selon les API disponibles ;
- corréler lancement, compilation de shaders, réseau et stockage sur une timeline ;
- comparer deux sessions locales, par exemple avant et après une action.

Une extension du contexte IA constitue une fonction distincte : elle devra afficher précisément quelles données supplémentaires sont capturées. Le copilote 1.1.1 reste limité à un seul instantané Future Lab, désormais enrichi des mesures GPU NVIDIA.

## 1.3 — Connaissance gaming synchronisée

- publier une base signée avec manifeste, expiration et retour à la version précédente ;
- synchroniser les fiches Ubuntu, pilotes, Proton et jeux depuis des sources officielles ;
- rapprocher une fiche des versions réellement observées ;
- avertir quand une information pertinente a changé depuis la dernière analyse ;
- proposer une actualisation automatique configurable une fois la chaîne signée opérationnelle.

## 2.0 — Assistant Gaming Linux

- recherche conversationnelle dans le diagnostic et la documentation, directement disponible dans l’édition personnelle ;
- profils d’optimisation expliqués pour écran, pilote et type de jeu ;
- catalogue d’actions testées, simulables et vérifiables ;
- modules versionnés pour les nouveaux fournisseurs GPU, plateformes et périphériques ;
- export d’un dossier d’assistance expurgé pour faciliter un ticket Steam, Proton ou pilote.

## Critères de sortie

Une fonction sort lorsqu’elle répond à une question gaming précise, respecte son budget de ressources, indique le niveau de certitude, compile en C17 strict et passe les tests automatisés ainsi que la validation responsive. Une opération système doit en plus expliquer ses privilèges et vérifier son résultat ; une fonction IA doit borner et afficher son contexte exact.

Les fonctions publiées sont dans [releases.md](releases.md) et la conception livrée de Future Lab dans [future-lab.md](future-lab.md).
