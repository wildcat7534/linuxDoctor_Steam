# Feuille de route

Cette feuille de route décrit un ordre de livraison, pas une date contractuelle.

## 0 — Fondations

- Initialiser le binaire C17, la génération de rapport JSON et l'interface statique.
- Définir le schéma de rapport JSON, les sévérités et le contrat des plugins.
- Créer un tableau de bord lisible avec états chargement, erreur et données indisponibles.
- Établir des données de démonstration et des tests de règles.

## 1 — Diagnostic essentiel

- Système, stockage, mises à jour, matériel, réseau et services de base.
- Score global explicable : la contribution de chaque catégorie est visible.
- Alertes accompagnées de preuves, recommandations et bouton **Pourquoi ?**.
- Export JSON et rapport HTML autonome.

## 1.5 — Tableau de bord et suivi local

- Navigation par catégories et page dédiée pour chaque domaine.
- Résumé d'accueil : dernière analyse, score, problèmes, avertissements et vérifications réussies.
- Section « Tout fonctionne correctement » pour les signaux positifs utiles.
- Historique local opt-in : évolution du score, changements importants et diagnostics résolus ou apparus.
- Bouton « Analyser maintenant », avec progression et rafraîchissement du rapport à la fin.

## 2 — Poste Linux moderne

- Graphics/Desktop : pilotes, Vulkan, OpenGL, Wayland/X11, VRR/HDR lorsque détectables.
- Gaming : Steam, Proton, Steam Input, Steam Controller, GameMode, MangoHud et Gamescope.
- Sécurité : état de mises à jour, pare-feu et signaux de configuration, sans promesse d'audit exhaustif.

## 3 — Écosystème et expertise

- Plugins NVIDIA, Docker, Ollama/IA, WireGuard, NFS/SMB et distributions ciblées.
- Rapports PDF, comparaison de rapports et historique local enrichi.
- Traductions et contenu pédagogique enrichi.

## Critères de sortie pour chaque diagnostic

- Une règle testée avec des cas nominal, avertissement et données absentes.
- Un titre compréhensible, des preuves et une action suggérée.
- Une explication « Pourquoi ? » relue pour être exacte, concise et non alarmiste.
- Aucun accès réseau ajouté sans raison explicite et documentée.
