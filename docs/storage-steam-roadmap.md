# Stockage et bibliothèques Steam

Ce document fixe le périmètre du domaine stockage. L’état des versions appartient aux [versions livrées](releases.md), les règles générales à l’[architecture](architecture.md), et l’interface à [ui.md](ui.md).

## Objectif

Linux Doctor aide à comprendre où se trouvent Ubuntu, Windows et les bibliothèques Steam, combien d’espace est réellement mesurable et pourquoi un volume est indisponible. Il commence toujours par l’observation ; aucune réparation, modification de `/etc/fstab` ou migration réelle n’est autorisée sans demande explicite et conception dédiée.

## Socle livré en lecture seule

Le rapport inventorie, lorsque la source locale le permet :

- périphérique et disque parent ;
- UUID et libellés ;
- taille et système de fichiers ;
- point de montage, lecture seule et accès en écriture ;
- espace utilisé et disponible pour les volumes montés ;
- transport, caractère amovible et indicateurs Windows confirmés ;
- bibliothèques Steam, jeux, outils/runtime et espace déclaré ;
- simulation d’une sélection de jeux à déplacer, sans copie ni suppression.

L’interface regroupe les partitions par disque, indique leur rôle probable uniquement à partir de faits explicites et replie les identifiants techniques. Une partition non montée a une occupation **inconnue**, jamais 0 %.

## Sources et niveaux de certitude

La collecte privilégie les formats structurés de `lsblk`, les informations de montage locales et les manifestes Steam. Toute conclusion est classée ainsi :

- **confirmée** : signal local direct et non ambigu ;
- **probable** : plusieurs indices cohérents, avec limite affichée ;
- **inconnue** : données absentes ou contradictoires.

Le nom `/dev/sdX` n’est jamais considéré stable. Les UUID servent à reconnaître un volume, sans justifier à eux seuls une action.

## Garde-fous

- Ne rien monter, réparer, reformater ou convertir au lancement.
- Ne jamais lancer toute l’application en administrateur.
- Protéger les composants Windows confirmés des suggestions automatiques.
- Ne pas interpréter l’absence de montage comme un disque vide ou défectueux.
- Ne pas employer `system()` ni concaténer des arguments non validés pour une future action.
- Toute future action devra être expliquée, simulable, confirmée, journalisée, vérifiée et réversible lorsque possible.

`ntfsfix` ne doit jamais être décrit comme l’équivalent de `chkdsk`. Une erreur NTFS peut venir d’un volume marqué dirty, d’une hibernation, d’un arrêt incorrect, d’options de montage ou d’un problème matériel ; Linux Doctor ne doit pas choisir une cause sans preuve.

## Cas de référence à conserver

- Un disque NTFS qui remonte après `ntfsfix` mais échoue au démarrage suivant : rechercher la cause récurrente avant de proposer une commande.
- Un second SSD non monté : produire un diagnostic distinct, sans généraliser le premier cas.
- Un NVMe NTFS à protéger : permettre d’ignorer localement toute recommandation future de réparation, migration ou conversion.
- Une partition EFI, swap ou récupération très petite : la rendre visible sans laisser croire que sa largeur graphique est exacte.

## Évolution prévue

### 1. Diagnostic enrichi

- présence et cohérence d’une entrée `/etc/fstab`, en lecture seule ;
- pilote de système de fichiers et options de montage ;
- erreurs pertinentes du journal, bornées et expliquées ;
- santé SMART et température si les outils sont présents ;
- historique minimal des échecs, relié à un UUID stable.

### 2. Préparation Steam

- détecter la provenance de Steam et ses bibliothèques indisponibles ;
- vérifier `compatdata`, `shadercache`, Workshop et chemins cassés sans lire le contenu personnel ;
- estimer une migration en conservant une marge de sécurité sur la source et la destination ;
- déléguer toute migration réelle au gestionnaire de stockage Steam tant qu’un moteur transactionnel n’existe pas.

### 3. Actions éventuelles, hors périmètre actuel

Une action de montage ou de réparation exigerait un helper minimal, une élévation séparée, des arguments strictement validés et un mode simulation. Une modification de `/etc/fstab` exigerait en plus sauvegarde, validation et retour arrière. Ces fonctions ne doivent pas être implémentées implicitement à partir de cette feuille de route.

## Critères d’acceptation

- fixtures disque simple, multi-partitions, non monté, lecture seule, amovible et dual boot ;
- absence de faux espace libre pour un volume inaccessible ;
- aucune écriture système pendant la collecte ou la simulation ;
- rapport utilisable quand `lsblk`, Steam ou une information optionnelle manque ;
- tests C17 stricts et vérification responsive de l’interface ;
- limites visibles à côté des valeurs concernées.
