# Instructions Codex — Linux Doctor

Avant toute modification, lire :

- `docs/storage-steam-roadmap.md`
- `README.md`
- les autres documents présents dans `docs/`

Le backend doit rester en C17.
Le frontend doit rester en HTML/CSS/JavaScript pur.
Ne pas implémenter de réparation, de modification de `/etc/fstab`
ou de migration automatique Steam sans demande explicite.

Après chaque tâche :

1. compiler le projet ;
2. exécuter les tests ;
3. expliquer les fichiers modifiés ;
4. signaler les risques et les éléments non testés.

## Toujours

Expliquer les choix.

Faire de petits commits.

Compiler.

Tester.

Documenter.

# Plateforme de référence

Linux Doctor est développé en priorité pour :

- Ubuntu 26.04 LTS
- GNOME 50+
- Wayland

Toutes les nouvelles fonctionnalités doivent être conçues et testées en priorité dans cet environnement.

Les autres distributions restent supportées autant que possible, mais elles ne doivent jamais ralentir l'évolution de la plateforme principale.

# Gaming First

Linux Doctor doit devenir le meilleur outil de diagnostic Linux orienté Gaming.

Chaque nouvelle fonctionnalité doit se poser la question :

Comment cela peut-il améliorer l'expérience de jeu ?

Le logiciel doit connaître notamment :

- Steam
- Proton
- Steam Input
- Steam Controller
- Steam Deck
- Gamescope
- GameMode
- MangoHud
- Vulkan
- DXVK
- VKD3D
- NVIDIA
- AMD
- HDR
- VRR
- Wayland

Les nouvelles fonctionnalités doivent surveiller en priorité :

- Wayland
- Vulkan
- HDR
- VRR
- Gamescope
- Steam Runtime
- Steam Input
- Steam Controller
- Steam Deck
- GameMode
- MangoHud
- DXVK
- VKD3D-Proton
- Proton
- NVIDIA
- AMD
- Intel Xe
- Mesa
- Wine
- Flatpak Steam

# Plateforme principale

La plateforme de référence est :

Ubuntu 26.04 LTS

Toutes les nouvelles fonctionnalités doivent être testées et optimisées en priorité pour Ubuntu 26.04.

Les autres distributions sont supportées autant que possible.

Les diagnostics doivent être adaptés à la distribution détectée.

Avant d'implémenter une fonctionnalité :

- existe-t-il une meilleure solution ?
- existe-t-il déjà une API officielle ?
- existe-t-il une méthode plus robuste ?
- existe-t-il une méthode plus portable ?

Si oui :

l'expliquer avant d'écrire le code.

Ne jamais coder une fonctionnalité uniquement parce qu'elle est demandée.

Toujours vérifier si elle s'intègre dans la vision globale du projet.

Les performances font partie des fonctionnalités.

Éviter :

- les allocations inutiles
- les copies inutiles
- les appels système redondants
- les scans répétés

Préférer :

- cache intelligent
- parallélisme lorsque pertinent
- structures simples
- faible consommation mémoire

Une réparation automatique est toujours plus dangereuse qu'un diagnostic.

Linux Doctor doit être extrêmement conservateur.

Il vaut mieux proposer une solution que modifier le système sans certitude.

Toute réparation doit être :

- expliquée
- simulable
- réversible lorsque possible

Le logiciel ne doit jamais mentir.

Lorsqu'une information n'est pas certaine :

indiquer :

- confirmé
- probable
- hypothèse

Ne jamais présenter une hypothèse comme une vérité.

Linux Doctor ne cherche pas seulement à détecter des problèmes.

Il cherche également à :

- expliquer
- enseigner
- rassurer
- optimiser

Un utilisateur doit ressortir plus compétent après avoir utilisé Linux Doctor.

> **Quand quelqu'un installe Ubuntu 26.04 pour jouer, Linux Doctor devrait devenir le premier outil qu'il installe après Steam.**

Pas parce qu'il est "à la mode", mais parce qu'il lui dira :

* *"Ton pilote NVIDIA est optimal."*
* *"Ton HDR est bien configuré."*
* *"GameMode manque, voici pourquoi tu pourrais l'installer."*
* *"Ton contrôleur Steam utilise le dernier firmware."*
* *"Cette option Wayland est connue pour poser problème avec tel jeu."*

L'idée n'est pas de modifier la machine à la place de l'utilisateur, mais de devenir  **l'assistant de référence pour un PC Linux orienté gaming** , tout en restant excellent pour le diagnostic système général. Je pense que c'est un positionnement qui rendrait le projet vraiment unique. 🎮🐧💚

# Veille technologique

Les agents doivent régulièrement vérifier les évolutions des technologies Linux.

Ils ne doivent pas conserver une ancienne architecture uniquement par habitude.

Si une nouvelle technologie devient mature et apporte un avantage clair :

- expliquer son intérêt ;
- comparer avec l'existant ;
- proposer une migration progressive.
