# Steam sur Ubuntu 26.04

Le domaine Steam répond à une question pratique : **les fondations nécessaires à mes jeux et périphériques sont-elles cohérentes sur cette machine ?** Il combine installation de Steam, runtimes, architecture i386, Vulkan, bibliothèques, Proton, manettes et session graphique.

Ubuntu 26.04 LTS apporte notamment NTSYNC, qui peut améliorer Wine et Proton. Linux Doctor le replace dans l’ensemble réellement nécessaire : pilotes Vulkan, bibliothèques 32 bits, runtime Steam et version de Proton.

## Matrice Gaming Readiness

| Domaine | Signal local | Livraison visée |
| --- | --- | --- |
| Steam | provenance Debian/Flatpak/Snap, version et runtime | prévu en 1.2 |
| Vulkan | chargeurs et pilotes 64/32 bits cohérents | socle livré, cohérence 32 bits en 1.2 |
| Proton | outils installés et version choisie par jeu | outils livrés, contexte par jeu en 1.2 |
| Steam Input | famille de manette, `steam-devices` et test guidé | famille et règles livrées, test en 1.2 |
| GameMode, MangoHud, Gamescope | présence, version et contexte d’usage | inventaire livré, contexte en 1.2 |
| DXVK/VKD3D-Proton | composants associés au préfixe du jeu | prévu en 1.2 |
| Wayland/Xwayland | session, pilote et contexte du jeu | socle livré |
| Bibliothèques | volume, accès, jeux et espace | livré, actions prévues en 1.3 |
| GeForce NOW | application, session et manette | livré |

## Diagnostic par jeu

Une conclusion utile associe l’AppID, la version de Proton, le pilote, la session et la source du problème connu. Cela permet d’écrire « ce problème est confirmé pour cette combinaison » ou « cette piste est probable » au lieu d’appliquer un verdict universel à tout Steam.

Les symptômes sont regroupés par lancement, image/HDR, performances, audio, réseau/anti-triche, contrôleur et stockage. Cette taxonomie alimente la base gaming, les filtres Future Lab et l’export d’un futur dossier d’assistance.

## Base gaming

`data/gaming-knowledge.tsv` utilise l’AppID Steam comme cible des fiches `game`. Les fiches `steam`, `controller`, `gfn` et `ubuntu` apparaissent selon les observations locales pertinentes.

Chaque fiche porte source HTTPS, date de révision, sévérité et action conseillée. `./scripts/update-knowledge.sh` installe une copie plus récente dans les données XDG ; le moteur conserve la base valide la plus récente. Le détail de la publication appartient à [data-sources.md](data-sources.md).

## Jeux et outils Steam

Linux Doctor distingue les jeux de Proton, Steam Linux Runtime, Steam Runtime, Steamworks Common Redistributables, Steam Input Configs et SteamVR. Cette séparation permet des totaux utiles, une migration correcte et des diagnostics spécifiques aux runtimes.

Valve peut ajouter ou renommer un composant. L’heuristique est donc complétée par les AppID connus et des fixtures ; un outil nouveau devient une règle testée plutôt qu’une exception invisible.

## Sources officielles

- [Documentation Steam pour Ubuntu](https://documentation.ubuntu.com/steam/)
- [Notes de version Ubuntu 26.04 LTS](https://documentation.ubuntu.com/release-notes/26.04/summary-for-lts-users/)
- [Suivi Steam pour Linux](https://github.com/ValveSoftware/steam-for-linux)
- [Suivi Proton](https://github.com/ValveSoftware/Proton/issues)
- [Dépannage Steam Controller](https://help.steampowered.com/en/faqs/view/41EA-7E25-B1F0-67E9)
- [Exigences GeForce NOW](https://www.nvidia.com/en-gb/geforce-now/system-reqs/)
- [Application GeForce NOW pour Linux](https://blogs.nvidia.com/blog/geforce-now-thursday-linux/)

La [veille technologique](technology-watch.md) définit la cadence de révision ; la [feuille de route](roadmap.md) suit la livraison des diagnostics.
