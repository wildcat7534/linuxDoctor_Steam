# Interface

## Objectif

L'interface doit donner envie d'explorer l'état de sa machine, sans ressembler à une sortie de terminal. L'inspiration est un tableau de bord moderne, avec une densité d'information maîtrisée et une identité Linux chaleureuse.

## Écran principal

- Un score **System Health** accompagné de son interprétation et de ses limites.
- Des cartes par catégorie : Operating System, Hardware, Graphics, Gaming, AI, Network, Storage, Security, Services et Updates.
- Chaque carte montre un état (`OK`, `Warning`, `Problem` ou `Unknown`), la priorité et un résumé actionnable.
- Les filtres permettent d'afficher les problèmes, avertissements ou toutes les informations.

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
