# Interface

L’interface aide une personne non spécialiste à préparer Ubuntu pour jouer. Elle montre d’abord un bilan illustré, puis les détails vérifiables à la demande ; elle ne ressemble ni à une sortie de terminal ni à une liste infinie.

## Hiérarchie

1. **Bilan général** : date, score expliqué, problèmes et avertissements.
2. **Petites victoires** : fondations déjà prêtes pour jouer.
3. **Catégories** : Stockage, Gaming, Graphismes, Mises à jour, Applications et Future Lab.
4. **Petit bilan de la catégorie** : quatre faits courts avec icônes et portée du score.
5. **Détails** : inventaires, preuves et recommandations repliables.

Une valeur inconnue apparaît comme telle, jamais comme 0 %. La couleur est toujours accompagnée d’une icône et d’un texte.

## Interactions communes

- clavier, contraste, focus visible et mise en page mobile ;
- bouton **Pourquoi ?** avec observé, importance, impact et prochaine étape ;
- liens externes dans un nouvel onglet ;
- bouton « ↑ Haut » visible après le début du défilement ;
- animations non clignotantes et désactivées avec `prefers-reduced-motion` ;
- commandes visibles, sélectionnables et copiées via l’API moderne ou un secours local.

Le navigateur ne lance aucune commande. Il affiche la procédure à exécuter volontairement dans un terminal.

## Stockage

Les partitions sont regroupées par disque physique. Une première carte montre capacité connue, proportion relative, rôle et accessibilité. L’ouverture du disque révèle chaque partition ; UUID, transport et périphérique restent repliés.

La longueur d’un segment représente une proportion, avec une largeur minimale pour les partitions minuscules. L’espace non partitionné et l’occupation d’un volume non monté restent inconnus. Un contenu Windows confirmé est protégé et expliqué.

## Gaming

- Le contour animé indique clairement que le comparatif porte sur **100 h de jeu**.
- Les profils GeForce NOW, PC moyen et PC haut de gamme ont une couleur et une icône distinctes.
- GeForce NOW sépare paiement mensuel, paiement annuel d’avance, équivalent mensuel, électricité et économie annuelle.
- Les jeux Steam et leurs icônes locales sont visibles par défaut ; Proton, runtimes et outils sont dans un volet distinct.
- Les manettes reçoivent un repère de famille ; le logo Steam local apparaît pour Valve/Steam.
- La base gaming montre version, date, origine et commande manuelle de mise à jour avant ses fiches détaillées.

Les estimations de puissance restent des hypothèses modifiables, jamais des mesures de la prise. Les tarifs sont datés et reliés à leur [source](data-sources.md).

## Mises à jour APT

La page commence par un bilan des candidats prêts, phasés, différés, retenus ou inconnus. La liste complète reste repliée. Pour chaque paquet, « À quoi sert ce paquet ? » précède sa description technique, souvent anglaise, elle-même repliée.

`./scripts/refresh-updates.sh` est copié depuis l’interface puis exécuté dans un terminal. `sudo` y demande directement le mot de passe ; Linux Doctor ne le lit, ne le transmet et ne le conserve jamais. Cette action actualise les index, sans installer de paquet.

## Future Lab

La première vue doit résumer CPU, charge, mémoire, réseau et disques avec des unités explicites. Les compteurs cumulés portent un libellé « depuis le démarrage » et ne sont pas dessinés comme des débits. Les longues listes d’interfaces et de périphériques restent repliées.

## Historique

L’historique 1.0 compare uniquement le score précédent et le remplissage de la partition racine. Il possède un état dédié pour la première analyse ou un score incomplet ; l’identification de diagnostics apparus ou résolus reste une évolution future. Voir le [périmètre réellement conservé](history.md).
