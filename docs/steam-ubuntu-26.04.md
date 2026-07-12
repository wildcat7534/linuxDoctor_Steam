# Steam sur Ubuntu 26.04

## Objectif du domaine

Le domaine `steam` indique les prérequis locaux mesurables, sans promettre qu'un jeu précis est compatible : présence d'une Steam Controller, règles `steam-devices`, architecture i386, bibliothèque Steam et espace disponible.

Ubuntu 26.04 LTS apporte notamment NTSYNC, qui peut améliorer les performances de Wine et Proton. Cela ne remplace ni les pilotes graphiques compatibles Vulkan, ni les bibliothèques 32 bits nécessaires au client Steam et à certains jeux.

## Problèmes récurrents à distinguer

Les tickets Steam et Proton sont trop nombreux et spécifiques aux jeux pour être réduits honnêtement à une alerte universelle. Linux Doctor les classe plutôt ainsi :

| Famille | Signal local possible | Limite à annoncer |
| --- | --- | --- |
| Steam Controller et Steam Input | Manette vue par le noyau, règles `steam-devices` présentes | Le test d'entrée Steam et le profil par jeu restent nécessaires. |
| Droits `hidraw` / udev | Règles du paquet `steam-devices` | Certaines manettes tierces demandent des règles spécifiques. |
| Bibliothèques 32 bits | Architecture `i386` activée | Les bibliothèques graphiques i386 doivent correspondre au pilote installé. |
| Vulkan, pilotes et Wayland/Xwayland | À ajouter avec les collecteurs GPU/session | Une régression peut dépendre d'une version précise de Mesa, NVIDIA, Proton ou du bureau. |
| Proton par jeu | Aucun verdict global fiable | Audio, vidéo, réseau, anti-triche et périphériques dépendent du jeu et de la version de Proton. |
| Client Steam / runtime | À ajouter : installation et journaux | Fenêtre noire, `steamwebhelper` et mises à jour du runtime sont des symptômes distincts. |
| Flatpak, Snap ou paquet Debian | À ajouter : provenance d'installation | Les permissions et les runtimes diffèrent ; ils ne doivent pas être confondus. |
| Bibliothèques de jeux | Taille de `steamapps` et volumes montés | La taille ne révèle ni le jeu ni le contenu personnel. |
| GeForce NOW sous Wayland | Flatpak officiel, session Wayland et Steam Controller détectés | Une demande de portail bureau peut venir du mode souris Steam Input ; elle n'est pas une preuve de panne. |

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
