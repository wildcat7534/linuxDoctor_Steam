# Stockage et bibliothèques Steam

Le domaine stockage aide à comprendre, préparer et améliorer l’emplacement des jeux. Il rassemble les disques Ubuntu/Windows, les bibliothèques Steam, l’espace réellement disponible et les actions adaptées à chaque volume.

## Capacités livrées

Le rapport inventorie, lorsque la source locale le permet :

- périphérique, disque parent, UUID et libellé ;
- taille, système de fichiers, point de montage et état d’accès ;
- espace utilisé et disponible pour les volumes montés ;
- transport, caractère amovible et indicateurs Windows confirmés ;
- bibliothèques Steam, jeux, outils/runtime et espace déclaré ;
- sélection de jeux et estimation de leur déplacement.

L’interface regroupe les partitions par disque et montre d’abord rôle, capacité et accessibilité. Les identifiants techniques restent dans les détails. Une partition non montée affiche une occupation inconnue, car sa capacité ne permet pas de déduire son espace libre.

## Parcours utilisateur visé

```text
Disque ou bibliothèque détecté
        ↓
rôle, état et impact gaming
        ↓
action adaptée avec aperçu
        ↓
confirmation et exécution séparée
        ↓
nouvelle analyse du volume et de Steam
```

Le diagnostic n’est donc pas une fin : il prépare une action précise et les preuves permettant d’en contrôler le résultat.

## Niveaux de preuve

- **confirmé** : signal local direct et non ambigu ;
- **probable** : plusieurs indices cohérents avec limite affichée ;
- **inconnu** : données absentes ou contradictoires.

Les UUID servent à reconnaître un volume ; `/dev/sdX` peut changer d’un démarrage à l’autre. Un contenu Windows confirmé reçoit une protection explicite dans toute proposition de modification.

## Cycle 1.2 — Diagnostic actionnable

- vérifier l’entrée `/etc/fstab` correspondante et expliquer les options de montage ;
- relier les erreurs pertinentes du journal à un UUID ;
- afficher pilote de système de fichiers, SMART et température quand disponibles ;
- détecter la provenance de Steam et les bibliothèques devenues indisponibles ;
- vérifier `compatdata`, `shadercache`, Workshop et chemins cassés ;
- calculer source, destination, marge de sécurité et durée estimée d’une migration ;
- générer un plan d’action vérifiable plutôt qu’une simple alerte.

## Cycle 1.3 — Actions Steam

- ouvrir le gestionnaire de stockage Steam sur la bonne bibliothèque ;
- préparer la création d’un dossier de bibliothèque avec ses prérequis ;
- suivre une migration déclenchée par Steam et vérifier l’espace après l’opération ;
- proposer le remontage d’un volume connu via une action privilégiée dédiée ;
- comparer l’état avant/après dans l’historique local.

## Actions système dédiées

Une réparation de système de fichiers, une modification de `/etc/fstab` ou une migration gérée directement par Linux Doctor aura son propre workflow : aperçu, sauvegarde utile, élévation minimale, confirmation explicite, journal d’exécution et contrôle final. L’analyse seule ne déclenche aucune de ces opérations.

`ntfsfix` n’est pas l’équivalent de `chkdsk`. Le workflow NTFS devra distinguer volume dirty, hibernation Windows, options de montage et problème matériel avant d’afficher l’action appropriée.

## Scénarios de référence

- disque NTFS qui échoue de nouveau après un redémarrage ;
- second SSD présent mais non monté ;
- NVMe Windows à exclure des actions ;
- partition EFI, swap ou récupération minuscule ;
- bibliothèque Steam absente après changement de montage ;
- destination trop petite ou source trop pleine pour déplacer un jeu.

## Critères de sortie

- fixtures disque simple, multi-partitions, non monté, accès restreint, amovible et dual boot ;
- espace libre inconnu plutôt que déduit pour un volume inaccessible ;
- plan, privilèges et résultat attendu visibles avant une action ;
- aucune modification déclenchée pendant une analyse ;
- reprise ou retour arrière définis pour une opération interruptible ;
- compilation C17 stricte et validation responsive.

Les fonctions publiées sont suivies dans [releases.md](releases.md), le contrat général d’action dans [architecture.md](architecture.md) et l’interface dans [ui.md](ui.md).
