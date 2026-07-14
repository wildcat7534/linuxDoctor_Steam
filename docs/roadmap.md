# Feuille de route

Cette feuille de route décrit l’ordre des prochains travaux, sans date contractuelle. Les éléments déjà livrés sont synthétisés dans [releases.md](releases.md) et les idées exploratoires dans [future-lab.md](future-lab.md).

## 1.1 — Expliquer si Ubuntu est prêt à jouer

- identifier la provenance de Steam et ses runtimes ;
- vérifier les fondations Vulkan 64/32 bits sans lancer de rendu trompeur ;
- relever GameMode, MangoHud, Gamescope, DXVK/VKD3D-Proton et leur contexte utile ;
- relier manettes, règles `steam-devices` et test Steam Input sans promettre la compatibilité d’un jeu ;
- présenter chaque recommandation avec fait, impact, limite et prochaine étape.

## 1.2 — Mesures Future Lab

- calculer des débits CPU, disque et réseau à partir de deux instantanés horodatés ;
- ajouter températures, pression mémoire et GPU seulement avec une source locale robuste ;
- construire une timeline bornée et volontaire, sans collecte permanente ;
- conserver le mode inconnu quand la source manque ou demande des privilèges.

## 1.3 — Connaissance connectée vérifiable

- signer les versions de la base et vérifier la signature hors ligne ;
- ajouter cache, expiration, retour à la version précédente et journal de provenance ;
- permettre les modes jamais et manuel ; l’automatique reste bloqué tant que ces garanties ne sont pas complètes ;
- rapprocher une fiche d’un jeu, d’une version de Proton et d’un pilote sans transformer une possibilité en panne détectée.

## 2.0 — Diagnostic avancé optionnel

- vue réseau par processus sans scan actif par défaut ;
- journaux Steam, Flatpak, noyau et services avec filtrage local ;
- modèle local facultatif pour résumer, jamais pour décider ou exécuter ;
- architecture de plugins versionnée si plusieurs modules indépendants en justifient le coût.

## Critères de sortie communs

- collecte en lecture seule par défaut et dégradation gracieuse ;
- signal local testable, niveau de certitude et limite documentés ;
- aucune donnée personnelle envoyée ;
- budget de temps, mémoire et volume de données borné ;
- compilation C17 stricte, tests automatisés et validation de l’interface ;
- documentation canonique mise à jour sans recopier l’historique des versions.
