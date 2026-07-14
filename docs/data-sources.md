# Sources de données et accès réseau

## Principe

Une analyse Linux Doctor reste locale, déterministe et utilisable hors ligne. Un accès réseau est une action distincte, visible et volontaire ; il ne transmet jamais l'inventaire de la machine. Une indisponibilité distante produit une donnée ancienne ou inconnue, jamais un faux état sain.

## Provenance obligatoire

Toute donnée éditoriale ou téléchargée indique au minimum :

- la source HTTPS directe et son organisme ;
- la portée concernée, par exemple France ou Ubuntu 26.04 ;
- la date de vérification et, si elle existe, la date d'expiration ;
- la version du jeu, pilote ou composant visé ;
- le niveau `confirmé`, `probable` ou `hypothèse`.

Une source officielle prime. Un témoignage peut orienter une recherche, mais ne suffit pas à créer une alerte générale.

## Base de connaissances gaming

Le diagnostic compare la copie utilisateur valide à la base intégrée et garde la plus récente selon sa date de révision. La mise à jour est manuelle : `./scripts/update-knowledge.sh --check` vérifie le canal officiel `knowledge-v1`, réservé au schéma 1, et `./scripts/update-knowledge.sh` l'installe dans le répertoire XDG de l'utilisateur.

Le téléchargement est limité à 512 Kio, borné dans le temps, effectué en HTTPS, validé par le moteur C puis installé atomiquement sans `sudo`. La validation de schéma ne remplace pas une signature cryptographique : aucune mise à jour automatique ne doit être activée avant la vérification d'une signature, la possibilité de revenir en arrière et une politique d'expiration. Le format appartient à [data/README.md](../data/README.md).

## Tarifs GeForce NOW France

Référence vérifiée le **14 juillet 2026**, TTC, sans promotion active :

| Offre | 1 mois | 12 mois payés d'avance | Équivalent mensuel | Économie annuelle |
| --- | ---: | ---: | ---: | ---: |
| Performance | 10,99 € | 109,99 € | 9,17 € | 21,89 € (16,60 %) |
| Ultimate | 21,99 € | 219,99 € | 18,33 € | 43,89 € (16,63 %) |

Sources : [grille NVIDIA France](https://www.nvidia.com/fr-fr/geforce-now/#product-matrix) et [FAQ GeForce NOW](https://www.nvidia.com/fr-fr/geforce-now/faq/). La grille ne proposait pas de nouvel abonnement de six mois. Performance et Ultimate comprennent 100 h par mois, avec au plus 15 h non utilisées reportées au mois suivant ; l'année ne constitue donc pas une réserve immédiate de 1 200 h.

L'interface doit écrire « équivalent mensuel, 12 mois payés d'avance ». Elle conserve le prix standard et le prix courant séparément, n'affiche une promotion que si son prix français et sa fin sont vérifiés, et date toujours le relevé. L'API qui alimente actuellement la page NVIDIA n'est pas documentée : Linux Doctor ne doit pas en dépendre au chargement.

Le calcul électrique part d'une valeur modifiable de **0,194 €/kWh TTC**. Elle correspond au Tarif Bleu réglementé, option Base 3 ou 6 kVA, en France métropolitaine au 1er février 2026 ; les puissances 9 kVA et plus ou un autre contrat ont un tarif différent. Source : [Commission de régulation de l'énergie](https://www.cre.fr/consommateurs/comprendre-les-tarifs-reglementes-de-vente-delectricite-trve.html). Au-delà des 100 h incluses, Linux Doctor n'affiche pas de coût horaire puisqu'il ne connaît pas le prix des heures supplémentaires éventuellement achetées.

## Garde-fous réseau

- domaine et chemin distants explicitement autorisés ;
- HTTPS avec certificats vérifiés, délai, taille et nombre de redirections bornés ;
- téléchargement dans un fichier temporaire, validation avant renommage atomique ;
- aucun secret, chemin personnel, rapport ou identifiant matériel dans la requête ;
- cache local daté, fonctionnement hors ligne et erreur compréhensible ;
- consentement séparé pour les modes `jamais`, `manuel` et, lorsqu'il sera sûr, `automatique`.

Les sources suivies par domaine et leur cadence sont définies dans la [veille technologique](technology-watch.md).
