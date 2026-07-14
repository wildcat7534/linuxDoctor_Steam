# Vision — Linux Doctor

Linux Doctor veut devenir le premier outil installé après Steam sur Ubuntu 26.04. Sa promesse ne s’arrête plus au constat : **diagnostiquer, expliquer, agir et mesurer le résultat** depuis une interface pensée pour une personne qui veut jouer, pas administrer Linux à plein temps.

## Promesse utilisateur

En quelques minutes, une personne doit pouvoir répondre à quatre questions :

1. **Mon PC est-il prêt pour jouer ?**
2. **Qu’est-ce qui limite réellement mon expérience ?**
3. **Pourquoi Linux Doctor arrive-t-il à cette conclusion ?**
4. **Quelle action puis-je effectuer maintenant et comment vérifier son effet ?**

Le bilan illustré répond d’abord. Les preuves, limites et détails techniques restent disponibles à la demande. Future Lab complète ce bilan avec des mesures vivantes, afin de relier une action à un changement visible.

## Des responsabilités visibles

```text
Observations locales ──> Diagnostic Engine ──> rapport et recommandations
Sources officielles ───> Knowledge Engine  ──> contexte gaming du tableau de bord

Snapshot Future Lab ──> constats qualitatifs déterministes
                                      │
                                      └─> assistant local optionnel
                                            reformulation courte
```

- Le **Diagnostic Engine** C17 reste déterministe : mêmes faits, même diagnostic.
- Le **Knowledge Engine** apporte des informations datées sur Ubuntu, Wayland, pilotes, Steam, Proton, contrôleurs et jeux.
- Le **LLM local 1.1.0** reformule uniquement les constats qualitatifs d’un instantané Future Lab capturé au clic. Il ne reçoit pas les deux autres moteurs.

Cette séparation permet une explication naturelle sans rendre le score opaque ni exposer tout le rapport au modèle. L’assistant est une capacité supplémentaire ; le diagnostic complet fonctionne aussi sans lui.

## Gaming First

Chaque fonction doit améliorer au moins un moment du parcours de jeu : installation, lancement, stabilité, fluidité, image, audio, contrôleur, réseau ou stockage. Les priorités sont Ubuntu 26.04 LTS, GNOME 50+, Wayland, Vulkan, HDR, VRR, Gamescope, GameMode, MangoHud, Proton, DXVK, VKD3D-Proton, Steam Runtime et Steam Input.

Linux Doctor doit pouvoir dire, preuves à l’appui :

- « ton pilote et tes chargeurs Vulkan sont cohérents » ;
- « GameMode manque, voici le bénéfice possible et l’action proposée » ;
- « ce jeu est associé à un problème Wayland connu sur cette version » ;
- « ta manette est détectée, voici le test Steam Input à effectuer » ;
- « ce pic de mémoire ou de disque coïncide avec ton lancement de jeu ».

## Plateforme de référence

Ubuntu 26.04 LTS, GNOME 50+ et Wayland forment la plateforme principale. À chaque évolution majeure, la veille vérifie les nouvelles capacités, les régressions, les pilotes et les réglages gaming utiles. Les autres distributions restent supportées lorsqu’elles exposent les mêmes signaux sans ralentir l’évolution de cette cible.

Les technologies et leurs sources officielles sont centralisées dans la [veille technologique](technology-watch.md).

## Actions visibles et maîtrisées

Linux Doctor peut proposer et orchestrer des actions lorsque leur résultat est vérifiable. Une action affiche son objectif, la commande ou l’opération, les privilèges nécessaires, le résultat attendu et la vérification finale. Les modifications privilégiées passent par un composant séparé ou le terminal ; il n’existe pas de réparation implicite déclenchée par une simple analyse.

La réussite se mesure simplement : une personne comprend la priorité, choisit une action adaptée, voit son effet dans Future Lab ou dans le diagnostic suivant et ressort plus compétente.
