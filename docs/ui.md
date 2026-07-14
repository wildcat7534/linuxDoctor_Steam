# Interface

Linux Doctor possède deux espaces complémentaires : le **tableau de bord**, qui hiérarchise l’état du PC, et **Future Lab**, une fenêtre autonome pour suivre les mesures et explorer les explications locales.

## Tableau de bord

La lecture suit un chemin court :

1. bilan général, date et état de complétude ;
2. priorités à traiter et petites victoires ;
3. catégories Gaming First ;
4. quatre faits essentiels par catégorie ;
5. preuves, inventaires et actions dans des volets repliables.

Une valeur inconnue apparaît comme telle. Chaque couleur est accompagnée d’une icône et d’un texte ; chaque score indique ce qui l’a fait monter ou baisser.

## Fenêtre Future Lab 1.1.0

`future-lab.html` s’ouvre depuis une carte d’appel visible dans le tableau de bord. Son identité visuelle cyan/violet évoque un cockpit gaming tout en conservant contraste, hiérarchie et sobriété des mouvements.

La fenêtre contient :

- un bandeau de source avec horodatage et état live/statique ;
- cinq cartes pour CPU, charge, mémoire, réseau et disques ;
- des valeurs instantanées et unités accessibles hors Canvas ;
- une timeline de 60 points par défaut, réglable à 30 ou 120 ;
- des détails repliables par interface ou périphérique ;
- une zone Assistant local et les constats déterministes utilisés ;
- un lien clair pour revenir au tableau de bord.

Le premier snapshot initialise les compteurs. Les taux apparaissent au point suivant seulement si schéma, démarrage, topologie et identités correspondent. Un fichier live ancien est rejeté ; une interruption rend la fraîcheur visible au lieu de prolonger artificiellement les courbes.

La carte Réseau porte le libellé **Toutes les interfaces**. Elle avertit qu’une interface physique et ses couches virtuelles peuvent comptabiliser le même trafic.

## Assistant local

La zone IA distingue trois états : **modèle absent**, **chargement** et **prêt localement**. Elle indique le modèle actif, son exécution WebGPU ou WASM et le fait que la réponse est une reformulation. Le consentement puis le clic sur l’analyse précèdent tout chargement en mémoire ; un Web Worker garde le cockpit utilisable pendant l’inférence.

Le clic fige l’instantané courant. La liste **Faits utilisés pour cette analyse** montre les constats déterministes associés ; le modèle reçoit leurs versions qualitatives sans chiffres et la question de l’utilisateur. La timeline, le rapport de diagnostic, les preuves et la base gaming ne lui sont pas transmis.

Une réponse contenant un chiffre, une commande ou un texte extérieur aux mesures est écartée. L’interface réaffiche alors la lecture factuelle ; l’assistant ne propose et ne lance aucune commande.

Le bouton de préparation renvoie vers `scripts/setup-local-ai.sh`. Le modèle ne se télécharge ni ne se charge silencieusement : l’utilisateur voit la taille approximative et choisit d’activer cette capacité.

## Actions

Une action visible emploie toujours le même composant :

- **Pourquoi ?** relie le fait à l’impact gaming ;
- **Préparer** affiche la commande ou l’opération et les prérequis ;
- **Lancer/Copier** utilise le canal réellement disponible ;
- **Vérifier** relance la mesure ou le diagnostic associé ;
- **Résultat** indique succès, échec ou état encore inconnu.

Le mot de passe `sudo` reste saisi directement dans le terminal ou par un helper dédié. Ces actions du tableau de bord sont indépendantes de l’assistant Future Lab.

## Domaines actuels

### Stockage

Les partitions sont regroupées par disque physique. Capacité, rôle, état de montage et occupation mesurable apparaissent d’abord ; UUID, transport et périphérique sont dans les détails. La proportion visuelle distingue la taille des partitions de leur taux de remplissage.

### Gaming

- contour animé pour le comparatif **100 h de jeu** ;
- profils GeForce NOW, PC moyen et PC haut de gamme visuellement distincts ;
- paiement mensuel, annuel d’avance, équivalent mensuel, électricité et économie ;
- jeux et icônes visibles par défaut, outils Steam dans un volet séparé ;
- familles de manettes et repère Valve/Steam ;
- version, date et provenance de la base gaming.

### Mises à jour APT

Un bilan précède les candidats prêts, phasés, différés ou retenus. « À quoi sert ce paquet ? » vient avant la description technique anglaise repliée. La commande `scripts/refresh-updates.sh` est copiable et son résultat apparaît dans l’analyse suivante.

## Qualité d’interaction

- navigation clavier, focus visible et responsive mobile ;
- liens externes dans un nouvel onglet ;
- bouton de retour en haut visible pendant les longues pages ;
- commandes sélectionnables avec copie moderne et solution de repli ;
- animations non clignotantes, réduites avec `prefers-reduced-motion` ;
- chiffres et états accessibles même lorsqu’un graphique ne peut pas être rendu.
