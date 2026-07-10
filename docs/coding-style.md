# Style de code

## Règles générales

- Préférer du code simple, explicite et testable aux abstractions prématurées.
- Ne pas mélanger collecte système, logique de diagnostic et rendu d'interface.
- Nommer les concepts métier avec des termes stables : `Observation`, `Diagnostic`, `Evidence`, `Recommendation`, `Explanation`.
- Documenter les hypothèses liées aux distributions, versions de pilotes ou outils externes.

## C17

- Compiler avec `-std=c17 -Wall -Wextra -Werror -Wpedantic`.
- Préférer les API POSIX et Linux aux commandes externes ; isoler les accès système dans des modules testables.
- Ne pas utiliser de variables globales mutables ni de macros complexes.
- Initialiser les structures, vérifier les valeurs de retour et contextualiser les erreurs.
- Préférer les buffers fixes aux allocations lorsque la taille est raisonnablement bornée. Toute allocation dynamique doit avoir un propriétaire et une libération documentés.

## Frontend

- HTML sémantique, CSS organisé par composant, JavaScript moderne sans framework imposé.
- Ne pas construire du HTML avec des données système non échappées.
- Garder le rendu déterministe à partir du rapport JSON ; aucune logique de diagnostic côté navigateur.
- Employer les mêmes identifiants de diagnostic que le backend pour les détails et explications.

## Tests et documentation

- Tester les règles avec des instantanés de rapports représentatifs, y compris les données manquantes.
- Toute nouvelle alerte inclut son texte « Pourquoi ? », ses preuves et une recommandation vérifiable.
- Les changements de schéma JSON sont documentés, versionnés et accompagnés de compatibilité ou migration.
