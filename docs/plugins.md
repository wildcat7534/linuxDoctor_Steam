# Plugins

Les plugins rendent chaque domaine de diagnostic indépendant. Ils ne contrôlent pas l'interface ; ils fournissent des données et conclusions au noyau.

## Domaines initiaux

- `storage`, `network`, `updates`, `services`, `hardware`
- `graphics`, `desktop`, `gaming`, `steam`
- Puis : `nvidia`, `docker`, `ollama`, `wayland` et autres intégrations spécialisées.

## Responsabilités

Un plugin :

1. déclare son identifiant, sa version et ses capacités ;
2. collecte des observations locales de façon défensive ;
3. applique ses règles ou transmet des observations au moteur partagé ;
4. retourne informations, diagnostics, score de catégorie et recommandations ;
5. fournit une explication pédagogique pour chaque diagnostic qu'il émet.

Un plugin ne doit pas modifier le système, appeler Internet, ni faire échouer l'analyse entière s'il est indisponible.

## Qualité des résultats

Les sévérités sont `ok`, `info`, `warning`, `problem` et `unknown`. `unknown` signifie que le plugin ne peut pas conclure : ce n'est ni une réussite ni un problème.

Les résultats doivent être déterministes pour une même machine et inclure la source de chaque fait (fichier système, commande, API). Les commandes externes sont limitées, avec délai d'expiration et erreurs converties en résultats exploitables.

Le collecteur `updates` utilise uniquement une simulation locale APT pour son diagnostic. Il distingue un candidat disponible d'un paquet sélectionné pour l'installation immédiate ; un déploiement progressif ou une décision de dépendances ne doit pas être présenté comme une panne. Les descriptions locales indiquent le rôle du paquet, pas le détail du nouveau changelog.

## Ajouter un plugin

- Choisir un identifiant stable, tel que `steam.controller.detected`.
- Écrire d'abord des échantillons de rapport et tests de règles.
- Définir les privilèges nécessaires ; préférer les informations accessibles sans élévation.
- Ajouter le diagnostic, les preuves, recommandations et le contenu **Pourquoi ?**.
- Vérifier la dégradation gracieuse sur une machine où la technologie n'est pas installée.
