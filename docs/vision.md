# Vision — Linux Doctor

Linux Doctor est un outil de diagnostic Linux local, rapide et agréable à consulter. Il ne se contente pas d'exposer des données système : il les interprète, priorise les problèmes et explique leurs conséquences dans un langage clair.

## Promesse

Répondre à trois questions, pour chaque machine :

1. **Quel est son état ?** — un aperçu fiable, avec un score et des catégories.
2. **Que faut-il améliorer ?** — des alertes concrètes, classées par importance.
3. **Pourquoi cela compte ?** — une explication courte, pédagogique et adaptée au contexte.

L'application est donc à la fois un tableau de bord de diagnostic, un assistant de maintenance et un outil d'apprentissage. Une alerte telle que « Partition système à 97 % » doit proposer **Pourquoi ?** et expliquer, par exemple, que l'espace libre aide les SSD, les mises à jour et les gestionnaires de paquets à fonctionner correctement.

## Principes

- **Local-first** : collecte, analyse et affichage fonctionnent sans Internet.
- **Respect de la vie privée** : pas de télémétrie, pas d'envoi de données système.
- **Utile avant tout** : privilégier les conclusions et actions aux inventaires interminables.
- **Accessible sans être simpliste** : une lecture immédiate pour débuter, des détails vérifiables pour les personnes avancées.
- **Transparent** : chaque diagnostic indique les faits observés et les limites éventuelles.
- **Extensible** : les domaines sont fournis par des plugins indépendants.
- **Sobre** : C17 et API POSIX/Linux pour le moteur, HTML/CSS/JavaScript sans framework lourd ni Electron.

## Publics

- Personnes découvrant Linux qui veulent comprendre un avertissement plutôt que rechercher chaque terme.
- Joueurs et joueuses vérifiant Steam, Proton, pilotes graphiques et périphériques.
- Développeurs, administrateurs et passionnés souhaitant un état rapide, exportable et justifiable.

## Ce que Linux Doctor n'est pas

- Un terminal déguisé ou un clone de `inxi`/`fastfetch`.
- Un outil d'optimisation automatique opaque.
- Un service cloud ou un collecteur de télémétrie.

## Mesure de réussite

Une personne doit pouvoir ouvrir le tableau de bord, comprendre les priorités de sa machine et savoir quelle action envisager, sans quitter l'application pour déchiffrer une alerte.
