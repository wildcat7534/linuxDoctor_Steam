# Historique local

## But

L'historique transforme Linux Doctor d'un diagnostic ponctuel en suivi de santé, sans devenir un outil de surveillance intrusif. Il doit montrer une évolution compréhensible et actionnable : problèmes apparus, problèmes résolus et valeurs réellement importantes.

## Principes produit

- **Opt-in** : aucune analyse n'est conservée tant que la personne ne l'a pas activé.
- **Local et effaçable** : les fichiers restent dans le répertoire d'état XDG et peuvent être supprimés depuis l'interface ou le CLI.
- **Explicable** : un changement de score est toujours relié aux diagnostics concernés.
- **Sobre** : pas de collecte continue ; une analyse manuelle ou planifiée crée au plus un snapshot.
- **Honnête** : une comparaison incompatible est signalée, jamais interprétée comme une dégradation.

## Données conservées

Un snapshot contient un horodatage, la version du schéma, le score, les identifiants et sévérités de diagnostics, et les mesures explicitement déclarées suivables (par exemple le pourcentage d'utilisation de `/`). Les chemins personnels, noms de réseaux, adresses, clés et sorties de commandes brutes sont exclus.

Le rapport courant peut exposer un bloc `history` qui contient uniquement les comparaisons utiles au frontend : précédent score, tendance, diagnostics apparus/résolus et séries de mesures compactes.

```json
{
  "history": {
    "enabled": true,
    "previous_score": 93,
    "score_delta": 3,
    "changes": [{
      "id": "storage.root.capacity",
      "kind": "worsened",
      "previous": "85 %",
      "current": "97 %"
    }]
  }
}
```

## Rétention initiale

Conserver les 30 derniers snapshots détaillés. Au-delà, remplacer les données quotidiennes par un résumé mensuel : score min/max, derniers états des diagnostics et quelques mesures agrégées. Cette politique donne une courbe utile sans accumulation infinie de données locales.

## Cas à traiter avant implémentation

- Première analyse : aucune comparaison n'est affichée.
- Nouvelle version du schéma ou d'une règle : comparaison marquée incompatible si nécessaire.
- Changement de machine ou de partition : pas de comparaison automatique.
- Horloge système incohérente : trier de manière sûre et avertir dans les métadonnées, sans fausser la tendance.
