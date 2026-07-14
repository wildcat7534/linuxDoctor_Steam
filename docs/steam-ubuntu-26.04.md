# Steam sur Ubuntu 26.04

## Objectif du domaine

Le domaine `steam` indique les prérequis locaux mesurables, sans promettre qu'un jeu précis est compatible : présence des manettes reconnues par le noyau, règles `steam-devices`, architecture i386, bibliothèque Steam et espace disponible.

Ubuntu 26.04 LTS apporte notamment NTSYNC, qui peut améliorer les performances de Wine et Proton. Cela ne remplace ni les pilotes graphiques compatibles Vulkan, ni les bibliothèques 32 bits nécessaires au client Steam et à certains jeux.

## Problèmes récurrents à distinguer

Les tickets Steam et Proton sont trop nombreux et spécifiques aux jeux pour être réduits honnêtement à une alerte universelle. Linux Doctor les classe plutôt ainsi :

| Famille | Signal local possible | Limite à annoncer |
| --- | --- | --- |
| Manettes et Steam Input | Manette Steam/Valve, Xbox, PlayStation, Nintendo, 8BitDo ou générique vue par le noyau ; règles `steam-devices` présentes | Le nom permet seulement une classification visuelle. Le test d'entrée Steam et le profil par jeu restent nécessaires. |
| Droits `hidraw` / udev | Règles du paquet `steam-devices` | Certaines manettes tierces demandent des règles spécifiques. |
| Bibliothèques 32 bits | Architecture `i386` activée | Les bibliothèques graphiques i386 doivent correspondre au pilote installé. |
| Vulkan, pilotes et Wayland/Xwayland | Linux Doctor relève le pilote noyau, la session et la présence des chargeurs/manifests locaux | Aucun rendu n'est encore lancé ; une régression peut dépendre d'une version précise de Mesa, NVIDIA, Proton ou du bureau. |
| Proton par jeu | Aucun verdict global fiable | Audio, vidéo, réseau, anti-triche et périphériques dépendent du jeu et de la version de Proton. |
| Client Steam / runtime | À ajouter : installation et journaux | Fenêtre noire, `steamwebhelper` et mises à jour du runtime sont des symptômes distincts. |
| Flatpak, Snap ou paquet Debian | À ajouter : provenance d'installation | Les permissions et les runtimes diffèrent ; ils ne doivent pas être confondus. |
| Bibliothèques de jeux | Taille de `steamapps` et volumes montés | La taille ne révèle ni le jeu ni le contenu personnel. |
| GeForce NOW sous Wayland | Flatpak officiel, session Wayland et manette détectée | Une demande de portail bureau peut venir du mode souris Steam Input ; elle n'est pas une preuve de panne. |

## Sources de suivi

- [Documentation Steam pour Ubuntu](https://documentation.ubuntu.com/steam/)
- [Notes de version Ubuntu 26.04 LTS](https://documentation.ubuntu.com/release-notes/26.04/summary-for-lts-users/)
- [Suivi officiel Steam pour Linux](https://github.com/ValveSoftware/steam-for-linux)
- [Suivi officiel Proton](https://github.com/ValveSoftware/Proton/issues)
- [Dépannage Steam Controller](https://help.steampowered.com/en/faqs/view/41EA-7E25-B1F0-67E9)
- [Exigences système GeForce NOW](https://www.nvidia.com/en-gb/geforce-now/system-reqs/)
- [Application GeForce NOW pour Linux](https://blogs.nvidia.com/blog/geforce-now-thursday-linux/)

## Règle de prudence

Une incompatibilité anti-triche, un jeu qui ne démarre pas ou une régression Proton doit rester un diagnostic associé à un jeu et une version de Proton. Linux Doctor peut préparer les informations utiles et pointer vers le suivi concerné, mais ne doit pas affirmer qu'un PC est globalement « compatible avec tous les jeux Steam ».

## Base locale et mise à jour 1.0

Les fiches de compatibilité sont conservées dans `data/gaming-knowledge.tsv`. Une fiche `game` utilise l'AppID Steam comme cible ; elle n'apparaît que si ce jeu est installé. Les fiches générales `steam`, `controller`, `gfn` et `ubuntu` suivent la même règle de pertinence locale.

Chaque ajout doit préciser une source HTTPS et une date de révision. Une copie plus récente peut être vérifiée puis installée volontairement dans les données XDG avec `./scripts/update-knowledge.sh`. L’analyse ne contacte jamais Internet et revient à la copie intégrée si la copie utilisateur est invalide. Une fiche ancienne peut guider une investigation, mais ne prouve pas que le problème existe encore avec la version courante du jeu, de Proton ou du pilote. Voir [data-sources.md](data-sources.md).

## Jeux et outils Steam

Linux Doctor distingue les jeux des composants distribués par Steam. Les noms Proton, Steam Linux Runtime, Steam Runtime, Steamworks Common Redistributables, Steam Input Configs et SteamVR, ainsi que plusieurs AppID de runtime connus, sont classés comme outils. Leur taille reste comptabilisée séparément, mais ils ne sont ni proposés à la migration comme des jeux, ni associés à une fiche de compatibilité de jeu.

Cette séparation est une heuristique locale : Valve peut ajouter ou renommer un composant. Un outil non reconnu doit être documenté et couvert par un test avant d'étendre la règle.
