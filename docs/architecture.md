# Architecture

## Vue d'ensemble

Linux Doctor sépare strictement le diagnostic de l'interface : un exécutable C17 collecte et analyse la machine, puis produit un rapport JSON versionné. Le frontend lit ce fichier sans connaître le système ni les règles de diagnostic.

```text
Système Linux → collecteurs C → règles de diagnostic → rapport JSON
                                                       ↓
                                          interface HTML/CSS/JS
                                                       ↓
                                                export HTML / PDF / JSON
```

Le backend est un outil CLI, par exemple `linux-doctor --output report.json`. Le frontend peut être servi par n'importe quel serveur statique local ; aucune ressource distante n'est requise.

## Composants

| Composant | Responsabilité |
| --- | --- |
| Noyau C | Orchestration, score global, production de rapport et exports. |
| Plugins | Collecte d'un domaine, évaluation des règles et production de résultats normalisés. |
| Moteur de règles | Transforme des observations factuelles en diagnostics, sévérités et recommandations. |
| Rapport JSON | Contrat stable entre backend, interface et exports. |
| Interface web | Lit le fichier JSON, présente les filtres, détails techniques et panneaux pédagogiques. Elle ne diagnostique pas. |

## Contrat de diagnostic

Chaque diagnostic doit contenir au minimum :

```json
{
  "id": "storage.root.nearly_full",
  "severity": "warning",
  "title": "Partition système presque pleine",
  "summary": "Il reste 6 Go sur la partition racine.",
  "evidence": [{ "label": "Utilisation", "value": "97 %" }],
  "recommendations": [{ "label": "Libérer de l'espace", "priority": "high" }],
  "explanation": {
    "why": "Les SSD et les mises à jour ont besoin d'espace libre pour fonctionner confortablement.",
    "impact": "Les installations peuvent échouer et le système devenir plus difficile à maintenir.",
    "learn_more": "storage.free-space"
  }
}
```

`id` est stable et sert aux liens, tests, traductions et explications. Les valeurs brutes restent distinctes des textes affichés. Un diagnostic inconnu ou incomplet doit l'indiquer au lieu d'inférer un état sain.

## Sécurité et confidentialité

- Écouter seulement sur la boucle locale par défaut.
- Ne jamais exécuter une action corrective sans demande explicite et confirmation claire.
- Masquer ou exclure des exports les secrets, chemins sensibles et identifiants réseau lorsque nécessaire.
- Versionner le schéma JSON et conserver une compatibilité de lecture raisonnable.
