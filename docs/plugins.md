# Modules et futur contrat de plugins

Les domaines sont aujourd’hui des modules C17 liés dans un exécutable unique. Ils partagent un rapport normalisé mais il n’existe pas encore d’API binaire ni de chargeur dynamique public. Le mot « plugin » désigne ici la frontière d’architecture visée, pas une capacité déjà livrée.

## Responsabilité d’un module

Un module :

1. collecte des observations locales de façon défensive ;
2. transforme uniquement les signaux fiables en diagnostics ;
3. retourne disponibilité, preuves, sévérité et limites ;
4. fournit une explication pédagogique et une prochaine étape ;
5. échoue sans empêcher les autres domaines de produire leur résultat.

Il ne modifie pas le système, n’appelle pas Internet pendant l’analyse et ne traite pas `unknown` comme `ok`.

## Contrat avant chargement dynamique

Une vraie API de plugins devra définir :

- version ABI et compatibilité du schéma ;
- capacités et privilèges déclarés ;
- budget temps, mémoire et taille de sortie ;
- annulation et délai d’expiration ;
- provenance des données et identifiants de diagnostics stables ;
- isolation des erreurs et stratégie de signature/distribution.

L’ajout de complexité n’est justifié que si des modules indépendants doivent être développés ou distribués séparément. Jusque-là, des interfaces C testables et des fichiers sources séparés restent plus simples et plus sûrs.

## Qualité

Chaque nouveau domaine commence par des fixtures : état nominal, donnée absente, sortie malformée, limite dépassée et dépendance indisponible. Les commandes externes éventuelles ont des arguments fixes, une sortie bornée et un délai. La procédure générale de développement appartient à [coding-style.md](coding-style.md).
