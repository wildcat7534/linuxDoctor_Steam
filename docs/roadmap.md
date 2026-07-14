# Feuille de route

Cette feuille de route décrit un ordre de livraison, pas une date contractuelle.

## 0 — Fondations

- Initialiser le binaire C17, la génération de rapport JSON et l'interface statique.
- Définir le schéma de rapport JSON, les sévérités et le contrat des plugins.
- Créer un tableau de bord lisible avec états chargement, erreur et données indisponibles.
- Établir des données de démonstration et des tests de règles.

## 1 — Diagnostic essentiel

- Système, stockage, mises à jour, matériel, réseau et services de base.
- Score global explicable : la contribution de chaque catégorie est visible.
- Alertes accompagnées de preuves, recommandations et bouton **Pourquoi ?**.
- Export JSON et rapport HTML autonome.

## 1.5 — Tableau de bord et suivi local

- Navigation par catégories et page dédiée pour chaque domaine.
- Résumé d'accueil : dernière analyse, score, problèmes, avertissements et vérifications réussies.
- Section « Tout fonctionne correctement » pour les signaux positifs utiles.
- Historique local opt-in : évolution du score, changements importants et diagnostics résolus ou apparus.
- À terme, bouton « Analyser maintenant », avec progression et rafraîchissement du rapport à la fin. Tant que le frontend est servi statiquement, il affiche honnêtement « Recharger le rapport » et ne prétend pas relancer le backend.

## 2 — Poste Linux moderne

- Graphics/Desktop : pilotes, Vulkan, OpenGL, Wayland/X11, VRR/HDR lorsque détectables.
- Gaming : Steam, Proton, Steam Input, Steam Controller, GameMode, MangoHud et Gamescope.
- Sécurité : état de mises à jour, pare-feu et signaux de configuration, sans promesse d'audit exhaustif.

## 3 — Écosystème et expertise

- Plugins NVIDIA, Docker, Ollama/IA, WireGuard, NFS/SMB et distributions ciblées.
- Rapports PDF, comparaison de rapports et historique local enrichi.
- Traductions et contenu pédagogique enrichi.

## Livraison 0.6 — Socle graphique local

- Inventaire en lecture seule des cartes DRM et du pilote noyau associé via `/sys/class/drm`.
- Détection du contexte Wayland/X11 transmis au processus.
- Détection locale des chargeurs OpenGL/Vulkan et des manifestes ICD Vulkan.
- Catégorie **Graphismes**, diagnostics explicables et bonnes nouvelles correspondantes.
- Aucun benchmark, rendu de test, accès réseau automatique, changement de pilote ou action corrective pendant le diagnostic.
- Inventaire APT local et explicable : rôle, versions, dépôt de sécurité éventuel et distinction entre candidat prêt, phasé ou différé.
- Actualisation optionnelle des seuls index APT dans un terminal ; le mot de passe reste sous le contrôle de `sudo` et aucune installation n'est automatique.

Les tests de rendu Vulkan/OpenGL, les versions de pilotes, HDR, VRR et les mesures de performances restent des lots ultérieurs. Une absence de contexte ou d'inventaire produit un état `unknown`, pas un verdict sain.

## Livraison 0.7 — Assistant Ubuntu Gaming

- Recentrer l'accueil et les explications sur la préparation d'un PC Ubuntu pour le jeu.
- Afficher un bilan illustré avant chaque inventaire détaillé.
- Replier les longues descriptions APT et les listes Steam jusqu'à une action explicite.
- Lire les petites icônes du cache Steam et les intégrer localement au rapport.
- Ajouter une base de connaissances locale, datée et sourcée, associable aux AppID installés, à Steam, aux manettes et à GeForce NOW.
- Conserver l'absence de réseau automatique, de réparation et d'installation de paquets.

La base 0.7 constitue un catalogue éditorial initial, pas une promesse de compatibilité universelle. Les fiches par jeu seront ajoutées progressivement après vérification des sources et des versions concernées.

## Livraison 0.8 — Tableau de bord gaming explicable

- Expliquer directement les scores de chaque catégorie, notamment les 95 % de **Gaming** et **Graphismes**.
- Afficher les jeux Steam et leurs icônes par défaut, tout en conservant un bouton pour replier la liste.
- Inventorier les manettes reconnues et distinguer visuellement Steam/Valve, Xbox, PlayStation, Nintendo et 8BitDo.
- Mettre les petites victoires en tête de page avant les inventaires longs.
- Replier les détails des disques, sans masquer leur synthèse ni empêcher les raccourcis de les ouvrir.
- Mettre en évidence que le comparatif énergétique porte sur exactement 100 heures de jeu.
- Ajouter un retour en haut de page et ouvrir le dépôt GitHub dans un nouvel onglet.

La détection d'une manette reste un constat de présence. La validation des touches, du profil Steam Input et du comportement en jeu nécessite toujours un essai réel.

## Livraison 0.9 — Budget de jeu et bibliothèque Steam propre

- Remplacer le clignotement du titre « 100 h » par un courant continu qui parcourt son contour, avec respect de la préférence de réduction des mouvements.
- Remonter le comparatif avant la bibliothèque longue et détailler le coût GeForce NOW : abonnement Performance ou Ultime, électricité, total pour la durée et coût horaire effectif.
- Dater les tarifs, lier la source officielle NVIDIA et annoncer clairement ce qui n'est pas inclus.
- Séparer au niveau du backend les jeux des outils Steam comme Proton, Steam Linux Runtime et Steamworks Common Redistributables.
- Exclure ces outils des suggestions de migration et des rapprochements avec les fiches de jeux.
- Rendre le retour en haut plus visible avec un libellé et le placer contre le bord du contenu plutôt qu'à l'extrémité de l'écran.
- Adopter le nouveau logo carré allégé fourni avec le projet.

Cette classification repose sur des AppID et noms connus. Elle doit être enrichie avec des cas réels sans transformer une heuristique en certitude universelle.

## Critères de sortie pour chaque diagnostic

- Une règle testée avec des cas nominal, avertissement et données absentes.
- Un titre compréhensible, des preuves et une action suggérée.
- Une explication « Pourquoi ? » relue pour être exacte, concise et non alarmiste.
- Aucun accès réseau ajouté sans raison explicite et documentée.
