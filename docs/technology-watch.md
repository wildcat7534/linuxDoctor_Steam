# Veille technologique

Linux Doctor évolue avec Ubuntu, GNOME et l’écosystème gaming. Cette liste fixe les technologies cibles ; elle ne signifie pas que chaque diagnostic est déjà implémenté.

| Technologie | Priorité | Position du projet |
| --- | --- | --- |
| Wayland | ⭐⭐⭐⭐⭐ Très haute | plateforme graphique principale |
| PipeWire | ⭐⭐⭐⭐⭐ Très haute | audio et capture modernes |
| Vulkan | ⭐⭐⭐⭐⭐ Très haute | fondation gaming prioritaire |
| HDR | ⭐⭐⭐⭐ Haute | à diagnostiquer quand les signaux seront robustes |
| VRR | ⭐⭐⭐⭐ Haute | à relier écran, session et pilote |
| Gamescope | ⭐⭐⭐⭐ Haute | environnement gaming à inventorier |
| Steam Runtime 3 | ⭐⭐⭐⭐ Haute | suivre runtimes et compatibilité |
| WebGPU / Transformers.js | ⭐⭐⭐⭐ Haute | assistant local Future Lab, WebGPU avec repli WASM |
| ONNX local | ⭐⭐⭐ Haute | modèles compacts, quantifiés et versionnés |
| OpenGL | ⭐⭐ Maintenance | compatibilité et applications existantes |
| X11 | ⭐ Compatibilité | repli, pas cible d’innovation |

## Cadence

| Priorité | Revue minimale |
| --- | --- |
| Très haute | chaque mois, avant une version Ubuntu/GNOME majeure et après une régression confirmée |
| Haute | chaque trimestre et avant une livraison qui touche le domaine |
| Maintenance ou compatibilité | tous les six mois, sauf rupture ou sécurité |

Chaque revue note version, date, sources primaires, impact gaming et niveau de certitude. Une nouveauté n’entre dans le diagnostic qu’après définition d’un signal local robuste, d’un coût borné et de ses limites.

## Sources prioritaires

- Plateforme : [notes Ubuntu](https://documentation.ubuntu.com/release-notes/), [versions GNOME](https://release.gnome.org/) et [noyau Linux](https://www.kernel.org/).
- Bureau et audio : [Wayland](https://wayland.freedesktop.org/) et [PipeWire](https://pipewire.org/).
- Graphismes : [Vulkan](https://www.khronos.org/vulkan/), [Mesa](https://docs.mesa3d.org/relnotes.html), [NVIDIA Unix](https://www.nvidia.com/en-us/drivers/unix/) et [AMDGPU](https://docs.kernel.org/gpu/amdgpu/).
- Gaming : [Steam pour Linux](https://github.com/ValveSoftware/steam-for-linux), [Proton](https://github.com/ValveSoftware/Proton), [Steam Runtime](https://github.com/ValveSoftware/steam-runtime) et [Gamescope](https://github.com/ValveSoftware/gamescope).
- IA locale : [Transformers.js](https://huggingface.co/docs/transformers.js/), [modèle Gemma 3 270M ONNX retenu](https://huggingface.co/onnx-community/gemma-3-270m-it-ONNX) et [WebGPU](https://www.w3.org/TR/webgpu/).

Les règles de provenance, cache et réseau appartiennent à [data-sources.md](data-sources.md).
