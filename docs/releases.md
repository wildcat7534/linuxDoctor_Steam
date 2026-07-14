# Versions de Linux Doctor

Cette page résume les versions publiées. La conception technique se trouve dans l’[architecture](architecture.md) et les travaux suivants dans la [feuille de route](roadmap.md).

## 1.1.1 — Édition personnelle accélérée

- chargement du modèle par bouton dédié, sans case de consentement intermédiaire ;
- correction du cache navigateur indisponible en utilisant directement les artefacts locaux ;
- remplacement du modèle 1B par Gemma 3 270M fp16 pour accélérer chargement et réponse ;
- questions plus libres tout en bloquant les commandes et contradictions directes ;
- activité GPU NVIDIA, VRAM, température, puissance et détection de `nvtop` ;
- nettoyage ciblé des Chromium headless de test avec élévation `sudo` si Snap l’exige.

## 1.1.0 — Future Lab vivant · publiée

- fenêtre Future Lab autonome, responsive et accessible depuis le tableau de bord ;
- identité visuelle de cockpit, graphiques Canvas accessibles et valeurs HTML équivalentes ;
- flux JSON local renouvelé environ chaque seconde et lancé automatiquement par `scripts/serve.sh` ;
- collecteur autonome protégé par un verrou contre les exécutions concurrentes ;
- timeline locale de 60 points par défaut, réglable à 30 ou 120 ;
- taux CPU, réseau et disque calculés uniquement entre deux snapshots compatibles ;
- rejet du fichier live ancien et repli explicite sur la photographie du rapport ;
- réseau cumulé sur toutes les interfaces, avec avertissement sur les couches virtuelles ;
- assistant Gemma 3 1B int8 optionnel, local et chargé sur consentement dans un Web Worker ;
- reformulation limitée à l’instantané capturé et à ses constats qualitatifs ;
- rejet des réponses contenant des nombres, des commandes ou un contenu hors des mesures Future Lab.

## 1.0 — Centre Ubuntu Gaming · publiée

- comparaison GeForce NOW mensuelle et annuelle, coût mensuel équivalent et économie ;
- disques et partitions réorganisés autour de leur rôle et de leur état ;
- base gaming versionnée, vérifiée puis installée dans les données utilisateur ;
- premier instantané Future Lab pour CPU, charge, mémoire, réseau et activité disque ;
- fiche Valheim reliée au support officiel.

## 0.6 à 0.9 — Construction du socle

- **0.6** : graphismes, session, Vulkan/OpenGL et simulation APT ;
- **0.7** : accueil Gaming First, icônes Steam et première base gaming ;
- **0.8** : scores expliqués, manettes, détails repliables et comparatif 100 h ;
- **0.9** : budget GeForce NOW, séparation jeux/outils Steam et navigation longue améliorée.

Les détails de ces anciennes versions restent dans l’historique Git ; cette synthèse conserve les jalons utiles au produit actuel.
