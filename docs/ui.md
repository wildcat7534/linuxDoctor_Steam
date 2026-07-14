# Interface

## Objectif

L'interface doit aider d'abord une personne non spécialiste à préparer Ubuntu pour jouer, sans ressembler à une sortie de terminal. L'inspiration est un tableau de bord moderne, avec une densité d'information maîtrisée, une identité Linux chaleureuse et des bilans illustrés avant tout inventaire long.

## Écran principal

- Un score **System Health** accompagné de son interprétation et de ses limites.
- Un résumé d'accueil : date de dernière analyse, éléments prioritaires et évolution depuis la précédente analyse.
- Des cartes par catégorie : Operating System, Hardware, Graphics, Gaming, AI, Network, Storage, Security, Services et Updates.
- Chaque carte montre un état (`OK`, `Warning`, `Problem` ou `Unknown`), la priorité et un résumé actionnable.
- Les filtres permettent d'afficher les problèmes, avertissements ou toutes les informations.
- Une section compacte « Ce qui est déjà prêt pour jouer » apparaît juste après l'en-tête et met en valeur les vérifications positives avant les listes longues.

## Historique et changement utile

L'historique ne doit pas être une courbe décorative. Il répond à « Qu'est-ce qui a changé ? » :

- score actuel, précédent et tendance sur une période choisie ;
- diagnostics nouveaux, résolus ou dont la sévérité a changé ;
- évolution de mesures pertinentes, par exemple l'occupation de la partition racine ;
- explication courte : « Depuis la dernière analyse, la partition système est passée de 85 % à 97 %. »

Une hausse ou baisse de score n'est jamais affichée sans les diagnostics qui l'expliquent. Les périodes sans historique affichent un état honnête, tel que « Première analyse : le suivi commencera après celle-ci ». L'historique est désactivé par défaut et son état est visible dans l'interface.

## Le bouton « Pourquoi ? »

Toute alerte ou recommandation significative propose un bouton **Pourquoi ?**. Il ouvre un panneau concis contenant :

1. **Ce que nous avons observé** — le fait mesuré, sans jargon inutile.
2. **Pourquoi c'est important** — mécanisme ou conséquence réelle.
3. **Ce qui peut arriver** — impact probable, formulé sans catastrophisme.
4. **Que faire ensuite** — une ou plusieurs actions, avec leur niveau de risque.

Exemple : pour une partition système à 97 %, expliquer que l'espace libre facilite les opérations d'écriture, les mises à jour et les installations ; ne pas prétendre que le SSD est forcément en danger.

## Principes d'interaction

- La couleur ne porte jamais seule le sens : icône, texte et sévérité l'accompagnent.
- Les données techniques sont accessibles par divulgation progressive, près de la conclusion qu'elles justifient.
- Les actions potentiellement risquées précisent leurs effets et demandent confirmation.
- Les états d'absence de données expliquent ce qui manque et, si possible, comment l'obtenir.
- L'interface reste utilisable au clavier, lisible avec un contraste suffisant et adaptable aux petites fenêtres.
- La navigation par catégories évite une page interminable ; l'accueil reste une synthèse, pas un inventaire.
- Chaque catégorie commence par « Le petit bilan » et quatre faits courts accompagnés d'icônes. L'explication du score apparaît immédiatement dans ce bilan ; une valeur inconnue est représentée par un tiret et jamais par un faux 0 %.
- Les jeux Steam et leurs icônes sont ouverts par défaut parce qu'ils constituent la partie la plus visuelle du domaine Gaming ; la personne peut toujours replier la liste.
- Les disques physiques conservent une synthèse visible, tandis que partitions, montages et preuves techniques sont repliés par défaut. Un raccourci vers un disque ouvre automatiquement son détail.
- Le comparatif énergétique affiche un titre lumineux et animé autour de « 100 h de jeu ». L'animation est désactivée lorsque le système demande une réduction des mouvements.
- Une flèche de retour en haut apparaît après défilement. Les liens externes, dont GitHub, s'ouvrent dans un nouvel onglet.
- Les manettes détectées reçoivent un badge de famille et, pour Steam/Valve, le logo Steam local. Un badge générique évite de masquer un modèle non reconnu.
- Une jaquette absente n'est jamais téléchargée automatiquement : l'interface utilise la petite icône locale du cache Steam ou un pictogramme de secours.

## Mises à jour APT et authentification

La page **Mises à jour** présente chaque candidat avec les versions installée et proposée, l'architecture, le dépôt, le paquet source et une réponse visible à « À quoi sert ce paquet ? ». Les états « prêt », « déploiement progressif », « retenu », « différé » et « inconnu » restent distincts. La description APT peut être en anglais : elle est donc repliée derrière « Afficher la description technique » et précise qu'elle décrit le rôle du paquet, pas les changements exacts de la version.

Le frontend statique ne contient jamais de champ de mot de passe. Il peut copier `./scripts/refresh-updates.sh`, mais la personne lance cette commande depuis la racine du projet dans un terminal afin que `sudo` possède seul la saisie du secret. Cette actualisation modifie les index APT, pas les paquets installés ; le rapport précédent reste consultable en cas de refus ou d'échec.

La copie emploie d'abord l'API moderne du presse-papiers puis une sélection locale de secours pour les navigateurs ou contextes qui la refusent. En dernier recours, la commande reste visible et sélectionnable.
