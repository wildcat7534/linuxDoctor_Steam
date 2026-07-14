# Modules et extensions

Les domaines Linux Doctor sont des modules C17 liés dans un exécutable unique. Ils partagent le rapport normalisé, le contrat de diagnostic et le futur contrat d’action. Cette architecture permet déjà d’ajouter rapidement Steam, GPU, périphériques ou sources de mesure sans coupler leur collecte à l’interface.

## Contrat d’un module

Un module peut déclarer quatre capacités :

1. **collecte** : observations locales ou données distantes autorisées ;
2. **diagnostic** : faits, niveau de certitude, impact et recommandation ;
3. **action** : aperçu, privilèges, exécution et vérification ;
4. **présentation** : métadonnées permettant au frontend de rendre le résultat.

Une panne de module ne bloque pas les autres domaines. Les identifiants restent stables afin que l’historique et la base gaming puissent référencer le même diagnostic.

## API d’extension visée

Une API publique sera utile lorsque des fournisseurs ou domaines devront être développés séparément. Son manifeste décrira :

- version ABI et schéma JSON ;
- capacités, commandes et privilèges ;
- budget temps, mémoire, stockage et réseau ;
- annulation, délais et taille de sortie ;
- provenance et fraîcheur des données ;
- permissions d’action ;
- signature, distribution et compatibilité.

Jusqu’à ce jalon, les interfaces C testables offrent un cycle d’intégration plus rapide et évitent une ABI prématurée.

## Qualité

Chaque domaine possède des fixtures nominales, absentes, malformées et à la limite. Toute commande externe a des arguments contrôlés, une sortie bornée et un délai. Une action ajoute des tests de simulation, interruption, échec et vérification finale. Les conventions générales sont dans [coding-style.md](coding-style.md).
