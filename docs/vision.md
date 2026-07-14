# Vision — Linux Doctor

Linux Doctor est un assistant Ubuntu Gaming local, rapide et agréable à consulter. Sa priorité est d'aider une personne ordinaire à comprendre si son PC possède de bonnes fondations pour Steam, Proton, ses manettes et GeForce NOW. Il ne se contente pas d'exposer des données système : il les interprète, priorise les problèmes et explique leurs conséquences dans un langage clair.

Il devient aussi, si la personne le souhaite, un compagnon de santé local : les analyses successives permettent de voir les progrès, les régressions et les changements importants de la machine.

## Promesse

Répondre à trois questions, pour chaque machine :

1. **Puis-je jouer dans de bonnes conditions ?** — un aperçu des fondations Ubuntu, Steam et graphiques réellement observables.
2. **Que faut-il améliorer ?** — des alertes concrètes, classées par importance.
3. **Pourquoi cela compte ?** — une explication courte, pédagogique et adaptée au contexte.

L'application est donc à la fois un tableau de bord de diagnostic, un assistant de maintenance et un outil d'apprentissage. Une alerte telle que « Partition système à 97 % » doit proposer **Pourquoi ?** et expliquer, par exemple, que l'espace libre aide les SSD, les mises à jour et les gestionnaires de paquets à fonctionner correctement.

## Principes

- **Local-first** : collecte, analyse et affichage fonctionnent sans Internet.
- **Respect de la vie privée** : pas de télémétrie, pas d'envoi de données système.
- **Utile avant tout** : privilégier les conclusions et actions aux inventaires interminables.
- **Accessible sans être simpliste** : une lecture immédiate pour débuter, des détails vérifiables pour les personnes avancées.
- **Transparent** : chaque diagnostic indique les faits observés et les limites éventuelles.
- **Évolutif dans le temps** : l'historique explique ce qui a changé depuis la dernière analyse plutôt que de juxtaposer des scores.
- **Extensible** : les domaines sont fournis par des plugins indépendants.
- **Sobre** : C17 et API POSIX/Linux pour le moteur, HTML/CSS/JavaScript sans framework lourd ni Electron.

## Publics

- Joueurs et joueuses découvrant Ubuntu qui veulent comprendre un avertissement plutôt que rechercher chaque terme.
- Personnes vérifiant Steam, Proton, pilotes graphiques, GeForce NOW et périphériques avant de jouer.
- Développeurs, administrateurs et passionnés souhaitant un état rapide, exportable et justifiable.

## Ce que Linux Doctor n'est pas

- Un terminal déguisé ou un clone de `inxi`/`fastfetch`.
- Un outil d'optimisation automatique opaque.
- Un service cloud ou un collecteur de télémétrie.

## Mesure de réussite

Une personne doit pouvoir ouvrir le tableau de bord, comprendre les priorités de sa machine et savoir quelle action envisager, sans quitter l'application pour déchiffrer une alerte.
