# Interface

## Objectif

L'interface doit donner envie d'explorer l'état de sa machine, sans ressembler à une sortie de terminal. L'inspiration est un tableau de bord moderne, avec une densité d'information maîtrisée et une identité Linux chaleureuse.

## Écran principal

- Un score **System Health** accompagné de son interprétation et de ses limites.
- Un résumé d'accueil : date de dernière analyse, éléments prioritaires et évolution depuis la précédente analyse.
- Des cartes par catégorie : Operating System, Hardware, Graphics, Gaming, AI, Network, Storage, Security, Services et Updates.
- Chaque carte montre un état (`OK`, `Warning`, `Problem` ou `Unknown`), la priorité et un résumé actionnable.
- Les filtres permettent d'afficher les problèmes, avertissements ou toutes les informations.
- Une section « Tout fonctionne correctement » met en valeur les vérifications positives sans diluer les priorités.

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
