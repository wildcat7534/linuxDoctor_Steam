# Linux Doctor — Stockage, réparation NTFS et migration Steam

## Objectif

Ajouter à Linux Doctor un module de stockage capable de :

- détecter les partitions montées et non montées ;
- expliquer pourquoi un volume ne monte pas ;
- proposer des réparations sûres et réversibles ;
- rendre les montages persistants ;
- suivre les problèmes NTFS récurrents ;
- détecter les bibliothèques Steam ;
- préparer une migration intelligente des jeux Steam.

Le projet reste en **C** pour le backend et en **HTML/CSS/JavaScript** pour le frontend.

Le développement doit commencer en lecture seule. Les actions de réparation et de migration ne seront ajoutées qu’après validation de l’architecture et des diagnostics.

---

## Contraintes essentielles

- Ne jamais modifier un disque automatiquement au lancement.
- Ne jamais formater ou convertir un système de fichiers automatiquement.
- Ne jamais lancer toute l’application en root.
- Ne jamais utiliser `system()` pour des opérations sensibles.
- Afficher les commandes avant exécution.
- Proposer un mode `--dry-run`.
- Demander confirmation avant toute modification.
- Sauvegarder les fichiers modifiés, notamment `/etc/fstab`.
- Vérifier le résultat après chaque action.
- Ne jamais présenter une hypothèse comme une certitude.
- Distinguer clairement les causes :
  - confirmée ;
  - probable ;
  - inconnue.

---

# 1. Inventaire des disques et partitions

Créer un module capable de collecter pour chaque disque et partition :

- périphérique (`/dev/sdb2`) ;
- UUID ;
- label ;
- modèle ;
- taille ;
- système de fichiers ;
- point de montage ;
- état monté ou non monté ;
- lecture seule ou lecture/écriture ;
- options de montage ;
- espace utilisé et disponible ;
- disque local, amovible ou réseau ;
- présence dans `/etc/fstab` ;
- pilote utilisé (`ntfs3`, `ntfs-3g`, etc.).

Sources recommandées :

- `lsblk --json`
- `findmnt --json`
- `blkid`
- `/proc/mounts`
- `/etc/fstab`
- `/sys`
- `journalctl`
- `smartctl` si disponible

Éviter le parsing fragile de sorties humaines lorsqu’un format JSON ou une source système structurée existe.

---

# 2. Cas de test réel

La machine de Clém possède notamment :

## Volume Apple HDD

- périphérique actuel : `/dev/sdb2`
- système de fichiers : NTFS
- point de montage souhaité : `/mnt/applehdd`

Après certains démarrages, les commandes suivantes sont nécessaires :

```bash
sudo ntfsfix /dev/sdb2
sudo mkdir -p /mnt/applehdd
sudo mount -t ntfs /dev/sdb2 /mnt/applehdd
```

Le problème revient après redémarrage alors que Windows n’est plus utilisé.

Linux Doctor doit chercher la cause au lieu d’automatiser aveuglément `ntfsfix`.

## Autre SSD

Un autre SSD ne monte pas non plus. Le module doit détecter automatiquement ce second cas et produire un diagnostic séparé.

## NVMe 4 To

Le NVMe de 4 To est actuellement en NTFS et ne doit pas être modifié pour l’instant.

Prévoir une option locale :

```text
Ignorer les recommandations de migration, réparation ou conversion pour ce disque
```

Cette préférence doit être conservée localement.

---

# 3. Diagnostic NTFS

Pour chaque volume NTFS non monté, déterminer autant que possible :

- absence de point de montage ;
- absence d’entrée `/etc/fstab` ;
- UUID changé ou entrée `fstab` invalide ;
- volume simplement non monté ;
- volume marqué `dirty` ;
- journal NTFS incohérent ;
- volume hiberné ;
- Fast Startup Windows suspecté ;
- démontage incorrect lors du dernier arrêt ;
- arrêt brutal ou perte d’alimentation ;
- problème lié au pilote `ntfs3` ;
- problème lié à `ntfs-3g` ;
- options de montage incompatibles ;
- permissions incorrectes ;
- volume déjà monté ailleurs ;
- erreur matérielle ou SMART ;
- erreur détectée dans le journal du démarrage courant ou précédent.

Examiner notamment :

```bash
journalctl -b
journalctl -b -1
dmesg
findmnt
blkid
mount
smartctl
```

Le diagnostic doit indiquer :

```text
Cause confirmée
Cause probable
Cause inconnue
```

`ntfsfix` doit être présenté comme une réparation limitée, et non comme l’équivalent de `chkdsk`.

---

# 4. Historique du problème

Linux Doctor doit conserver localement un historique minimal :

- volume monté ou non lors de chaque scan ;
- nombre de démarrages consécutifs avec échec ;
- retour récurrent du bit `dirty` ;
- changement de nom `/dev/sdX` ;
- stabilité de l’UUID ;
- pilote utilisé ;
- dernier arrêt détecté comme propre ou non ;
- résultat des réparations précédentes.

Exemple d’explication :

```text
Ce volume a échoué au montage lors de 3 démarrages consécutifs.
La réparation temporaire ntfsfix ne semble pas traiter la cause.
```

---

# 5. Réparations proposées

Chaque réparation doit posséder :

- un identifiant stable ;
- une description ;
- un niveau de risque ;
- une liste exacte de commandes ;
- un mode simulation ;
- une confirmation ;
- un journal d’exécution ;
- une vérification après action ;
- une méthode de retour arrière si possible.

## Monter uniquement

```bash
sudo mkdir -p /mnt/applehdd
sudo mount /dev/sdb2 /mnt/applehdd
```

## Réparation NTFS légère

```bash
sudo ntfsfix /dev/sdb2
```

Afficher un avertissement clair :

```text
ntfsfix corrige seulement certains problèmes NTFS.
Il ne remplace pas une vérification complète du système de fichiers.
```

## Montage persistant

Ne jamais utiliser `/dev/sdb2` directement dans `/etc/fstab`, car ce nom peut changer.

Utiliser l’UUID.

Exemple conceptuel :

```fstab
UUID=<uuid> /mnt/applehdd ntfs3 defaults,nofail,uid=1000,gid=1000 0 0
```

Les options exactes doivent dépendre :

- du pilote disponible ;
- de l’utilisateur courant ;
- du besoin d’écriture ;
- du système de fichiers ;
- des permissions souhaitées.

Avant modification :

```bash
sudo cp /etc/fstab /etc/fstab.linux-doctor-backup-<date>
```

Puis validation :

```bash
sudo mount -a
```

En cas d’échec, restaurer automatiquement la sauvegarde.

---

# 6. Sécurité du moteur de réparation

Le frontend ne doit jamais exécuter directement une commande privilégiée.

Préférer :

- `execve()` ;
- `posix_spawn()` ;
- un helper privilégié minimal ;
- Polkit pour l’élévation de privilèges.

Éviter :

- `system()` ;
- les commandes shell concaténées ;
- les chemins non validés ;
- l’exécution de toute l’application en root.

Valider strictement :

- noms de périphériques ;
- UUID ;
- chemins de montage ;
- arguments ;
- permissions.

Prévoir :

```text
--dry-run
```

et un journal local des actions.

---

# 7. Interface Stockage

Ajouter une catégorie :

```text
💾 Stockage
```

Chaque volume possède une carte.

Exemple :

```text
Apple HDD
/dev/sdb2
NTFS
931 Go
Non monté

Cause probable :
volume marqué comme incorrect

Actions :
[ Pourquoi ? ]
[ Monter ]
[ Réparer ]
[ Rendre persistant ]
```

Afficher aussi :

- santé SMART ;
- température si disponible ;
- espace libre ;
- pilote utilisé ;
- options de montage ;
- présence dans `/etc/fstab` ;
- historique des échecs.

---

# 8. Détection Steam

Créer un module :

```text
🎮 Migration Steam
```

Détecter :

- toutes les bibliothèques Steam ;
- leurs points de montage ;
- le système de fichiers ;
- l’espace libre ;
- les permissions en écriture ;
- les jeux installés ;
- la taille de chaque jeu ;
- l’AppID ;
- les fichiers `appmanifest_*.acf` ;
- les dossiers `steamapps/common` ;
- `compatdata` ;
- `shadercache` ;
- Workshop ;
- bibliothèques indisponibles ;
- chemins cassés ;
- montages en lecture seule.

---

# 9. Fonctions de migration Steam

## Migration individuelle

Exemple :

```text
Déplacer Cyberpunk 2077
Source : SSD système
Destination : NVMe 4 To
Taille : 86 Go
Espace libéré : 86 Go
```

## Migration par lots

Permettre :

- la sélection de plusieurs jeux ;
- le tri par taille ;
- la sélection automatique des jeux les plus lourds ;
- un objectif d’espace à libérer.

Exemple :

```text
Objectif : libérer au moins 50 Go sur /
```

Linux Doctor peut proposer un plan contenant les jeux nécessaires pour atteindre cet objectif.

## Suggestions intelligentes

- jeux rarement lancés vers HDD ;
- jeux souvent utilisés vers NVMe ;
- gros jeux vers le disque ayant le plus d’espace ;
- éviter un volume NTFS instable ;
- refuser une destination non montée ;
- refuser une destination en lecture seule ;
- avertir lorsque la destination est presque pleine ;
- respecter les disques marqués comme protégés.

---

# 10. Méthode de migration Steam

Steam sait déjà déplacer un jeu via son gestionnaire de stockage. Linux Doctor doit, dans une première version, guider ou ouvrir cette fonction plutôt que déplacer naïvement les fichiers.

Ne pas déplacer seulement le dossier du jeu.

Une installation Steam peut dépendre de :

- `appmanifest_<appid>.acf` ;
- `compatdata/<appid>` ;
- `shadercache/<appid>` ;
- Workshop ;
- sauvegardes locales ;
- fichiers externes ;
- Steam Cloud.

## Première version

Proposer :

```text
Ouvrir le gestionnaire de stockage Steam
```

et afficher le plan recommandé.

## Migration automatisée future

Procédure transactionnelle :

1. vérifier que Steam est fermé ou que le jeu n’est pas utilisé ;
2. vérifier l’espace disponible ;
3. vérifier les permissions ;
4. créer la destination ;
5. copier les données ;
6. conserver les attributs nécessaires ;
7. vérifier la taille et éventuellement les sommes de contrôle ;
8. gérer le manifeste ;
9. faire reconnaître la bibliothèque à Steam ;
10. demander une vérification des fichiers ;
11. supprimer la source uniquement après validation ;
12. permettre la reprise après interruption.

Ne jamais faire un simple `mv` sans gestion des manifestes et validation.

---

# 11. Interface Migration Steam

Exemple :

```text
🎮 Bibliothèque Steam

Disque système
214 / 233 Go utilisés
Danger : presque plein

Jeux installés : 18
Taille totale : 412 Go

Destination recommandée :
WD Black SN850X 4 To

[ Choisir les jeux ]
[ Libérer automatiquement 50 Go ]
[ Ouvrir le stockage Steam ]
```

Avant action :

```text
Plan de migration

3 jeux
128 Go à copier
128 Go libérés sur /
Durée estimée : 18 minutes

[ Simuler ]
[ Commencer ]
```

---

# 12. Diagnostics Steam utiles

Détecter :

- bibliothèque sur un disque non monté ;
- bibliothèque NTFS avec permissions incorrectes ;
- `compatdata` inaccessible ;
- manifeste sans dossier de jeu ;
- dossier de jeu sans manifeste ;
- bibliothèque presque pleine ;
- jeu installé sur `/` presque pleine ;
- ancienne bibliothèque absente ;
- chemins dupliqués ;
- montage réseau déconseillé ;
- destination FAT/exFAT problématique ;
- différence de casse ;
- liens symboliques cassés.

---

# 13. Priorités de développement

## V0.2

- inventaire des disques ;
- volumes montés et non montés ;
- UUID ;
- systèmes de fichiers ;
- bibliothèques Steam ;
- jeux et tailles ;
- lecture seule uniquement.

## V0.3

- diagnostic NTFS ;
- lecture de `/etc/fstab` ;
- historique des échecs ;
- recommandations de montage persistant ;
- bouton ouvrant le gestionnaire de stockage Steam.

## V0.4

- planificateur de migration Steam ;
- sélection par lots ;
- objectif d’espace libre ;
- simulation uniquement.

## V0.5

- helper privilégié ;
- montage contrôlé ;
- réparation NTFS contrôlée ;
- édition sécurisée de `fstab` ;
- migration Steam expérimentale et transactionnelle ;
- reprise après erreur.

---

# 14. Consigne immédiate pour Codex

Ne pas implémenter toutes les réparations immédiatement.

Commencer par :

1. définir les structures C ;
2. créer les modules de collecte en lecture seule ;
3. produire un JSON stable ;
4. afficher les volumes et bibliothèques Steam ;
5. ajouter les diagnostics ;
6. écrire les tests ;
7. seulement ensuite proposer les actions.

Le code doit rester :

- en C17 ;
- modulaire ;
- lisible ;
- documenté ;
- testable ;
- sans variables globales inutiles ;
- sans fonctions géantes ;
- sans dépendances lourdes ;
- sans framework frontend lourd.

Le frontend reste en HTML/CSS/JavaScript pur.

---

# 15. Résultat attendu

Linux Doctor doit pouvoir expliquer clairement :

```text
Pourquoi ce disque ne monte-t-il pas ?
Le problème revient-il après chaque redémarrage ?
La réparation précédente a-t-elle fonctionné ?
Le disque peut-il recevoir une bibliothèque Steam ?
Quels jeux déplacer pour libérer suffisamment d’espace ?
L’action proposée est-elle sûre et réversible ?
```

L’objectif est de créer un assistant système pédagogique et fiable, pas seulement un afficheur d’informations.
