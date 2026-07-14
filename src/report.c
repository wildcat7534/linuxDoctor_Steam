#include "report.h"

#include "json.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static const char *severity_for(const StorageInfo *storage)
{
    if (!storage->available) return "unknown";
    if (storage->used_percent >= 95U) return "problem";
    if (storage->used_percent >= 85U) return "warning";
    return "ok";
}

static int score_for(const char *severity)
{
    if (severity[0] == 'p') return 45;
    if (severity[0] == 'w') return 75;
    if (severity[0] == 'o') return 96;
    return 0;
}

static int write_generated_at(FILE *stream)
{
    char timestamp[32] = "";
    time_t now = time(NULL);
    struct tm *utc;

    if (now != (time_t)-1) {
        utc = gmtime(&now);
        if (utc != NULL) (void)strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", utc);
    }
    return fputs("\"generated_at\":", stream) == EOF || json_write_string(stream, timestamp) != 0 ? -1 : 0;
}

static int write_storage_diagnostic(FILE *stream, const StorageInfo *storage)
{
    const char *severity = severity_for(storage);
    const char *title = storage->available && storage->used_percent < 85U
        ? "Espace système disponible" : "Partition système à surveiller";

    if (storage->available && storage->used_percent >= 95U) {
        title = "Partition système presque pleine";
    } else if (!storage->available) {
        title = "Espace système non disponible";
    }
    if (fprintf(stream, "{\"id\":\"storage.root.capacity\",\"severity\":") < 0 ||
        json_write_string(stream, severity) != 0 ||
        fputs(",\"title\":", stream) == EOF || json_write_string(stream, title) != 0 ||
        fputs(",\"evidence\":[", stream) == EOF) return -1;
    if (storage->available) {
        if (fprintf(stream, "{\"label\":\"Utilisation\",\"value\":\"%u %%\"},", storage->used_percent) < 0 ||
            fprintf(stream, "{\"label\":\"Espace disponible\",\"bytes\":%" PRIu64 "}", storage->available_bytes) < 0) return -1;
    } else if (fputs("{\"label\":\"Utilisation\",\"value\":\"Indisponible\"}", stream) == EOF) return -1;
    return fputs(
        "],\"recommendations\":[{\"label\":\"Examiner les fichiers volumineux et les paquets inutilisés\",\"priority\":\"high\"}],"
        "\"explanation\":{\"observed\":\"La capacité de la partition racine a été mesurée localement.\","
        "\"why\":\"L'espace libre aide les mises à jour, les installations et les écritures temporaires à se terminer correctement. Les SSD conservent aussi une marge de fonctionnement lorsqu'ils ne sont pas presque saturés.\","
        "\"impact\":\"Une mise à jour ou l'installation d'un jeu peut échouer par manque d'espace ; la maintenance des paquets devient plus compliquée.\","
        "\"next_step\":\"Commencez par identifier les dossiers les plus volumineux, puis supprimez ou déplacez ce dont vous n'avez plus besoin.\"}}",
        stream) == EOF ? -1 : 0;
}

static int write_steamapps_diagnostic(FILE *stream, const StorageInfo *storage)
{
    const char *severity = storage->steamapps_available ? "info" : "unknown";
    const char *title = storage->steamapps_available
        ? "Espace occupé par les jeux Steam" : "Bibliothèque Steam non détectée";
    const char *summary = storage->steamapps_available
        ? "La taille du dossier steamapps a été mesurée localement." : "Aucun dossier steamapps lisible n'a été trouvé dans les emplacements Steam habituels.";

    if (fprintf(stream, "{\"id\":\"storage.steamapps.size\",\"severity\":") < 0 ||
        json_write_string(stream, severity) != 0 || fputs(",\"title\":", stream) == EOF ||
        json_write_string(stream, title) != 0 || fputs(",\"evidence\":[", stream) == EOF) return -1;
    if (storage->steamapps_available) {
        if (fprintf(stream, "{\"label\":\"Dossier steamapps\",\"bytes\":%" PRIu64 "}", storage->steamapps_bytes) < 0) return -1;
    } else if (fputs("{\"label\":\"Emplacements vérifiés\",\"value\":\"Steam natif et Flatpak\"}", stream) == EOF) return -1;
    return fputs(
        "],\"recommendations\":[{\"label\":\"Déplacer ou désinstaller les jeux inutilisés depuis Steam si l'espace manque\",\"priority\":\"medium\"}],"
        "\"explanation\":{\"observed\":", stream) == EOF || json_write_string(stream, summary) != 0 ||
        fputs(",\"why\":\"Les jeux installés et leurs contenus additionnels peuvent représenter une part importante du stockage.\",", stream) == EOF ||
        fputs("\"impact\":\"Cette mesure aide à relier l'espace utilisé aux jeux, sans analyser le contenu personnel.\",", stream) == EOF ||
        fputs("\"next_step\":\"Utilisez le gestionnaire de stockage Steam pour voir les jeux les plus volumineux et choisir ceux à déplacer ou désinstaller.\"},\"summary\":", stream) == EOF ||
        json_write_string(stream, summary) != 0 || fputs("}", stream) == EOF ? -1 : 0;
}

static int write_other_storage_diagnostic(FILE *stream, const StorageInfo *storage)
{
    size_t index;

    if (fputs("{\"id\":\"storage.other_mounts.free_space\",\"severity\":\"info\",\"title\":", stream) == EOF ||
        json_write_string(stream, storage->mount_count == 0 ? "Aucun autre stockage local détecté" : "Espace libre des autres stockages") != 0 ||
        fputs(",\"evidence\":[", stream) == EOF) return -1;
    if (storage->mount_count == 0) {
        if (fputs("{\"label\":\"Montages supplémentaires\",\"value\":\"Aucun détecté\"}", stream) == EOF) return -1;
    }
    for (index = 0; index < storage->mount_count; index++) {
        const StorageMount *mount = &storage->mounts[index];
        if (index > 0 && fputc(',', stream) == EOF) return -1;
        if (fputs("{\"label\":", stream) == EOF || json_write_string(stream, mount->path) != 0 ||
            fprintf(stream, ",\"bytes\":%" PRIu64 ",\"detail\":\"%u %% utilisé\"}",
                mount->available_bytes, mount->used_percent) < 0) return -1;
    }
    return fputs(
        "],\"recommendations\":[{\"label\":\"Choisir le stockage qui convient avant d'installer un jeu ou des données volumineuses\",\"priority\":\"low\"}],"
        "\"explanation\":{\"observed\":\"Les systèmes de fichiers montés localement sont mesurés séparément.\","
        "\"why\":\"Un second disque ou une autre partition peut offrir plus d'espace que la partition système.\","
        "\"impact\":\"Vous pouvez éviter de saturer la partition système en choisissant un emplacement adapté.\","
        "\"next_step\":\"Comparez l'espace libre avant de créer une nouvelle bibliothèque Steam ou de déplacer des fichiers volumineux.\"},\"summary\":", stream) == EOF ||
        json_write_string(stream, storage->mount_count == 0 ? "Aucun autre système de fichiers local mesurable." : "Chaque montage indique son espace libre actuel.") != 0 ||
        fputs("}", stream) == EOF ? -1 : 0;
}

static int write_gaming_diagnostic(FILE *stream, const SteamInfo *steam)
{
    const bool installed = steam->steam_devices_installed;
    const char *severity = installed ? "ok" : "warning";
    const char *title = installed ? "steam-devices installé" : "steam-devices absent";
    const char *summary = installed
        ? "Les règles manettes Steam sont installées."
        : "Le paquet steam-devices n'est pas détecté.";
    const char *observed = installed
        ? "Le paquet steam-devices est installé localement."
        : "Le paquet steam-devices n'a pas été trouvé dans dpkg.";
    const char *impact = installed
        ? "Les nouvelles manettes Steam ont de meilleures chances de fonctionner immédiatement sous Ubuntu 26.04."
        : "Les nouvelles manettes Steam peuvent manquer de règles udev et de permissions adaptées.";
    const char *next_step = installed
        ? "Aucune action requise."
        : "Installez steam-devices puis reconnectez la manette Steam.";

    if (fprintf(stream, "{\"id\":\"steam.controller.rules\",\"severity\":") < 0 ||
        json_write_string(stream, severity) != 0 ||
        fputs(",\"title\":", stream) == EOF || json_write_string(stream, title) != 0 ||
        fputs(",\"evidence\":[", stream) == EOF) return -1;
    if (fprintf(stream, "{\"label\":\"Paquet\",\"value\":\"steam-devices\"},") < 0 ||
        fprintf(stream, "{\"label\":\"État\",\"value\":\"%s\"}", installed ? "installé" : "absent") < 0) return -1;
    return fputs(
        "],\"recommendations\":[{\"label\":\"Installer steam-devices pour les nouvelles manettes Steam\",\"priority\":\"high\"}],"
        "\"explanation\":{\"observed\":", stream) == EOF || json_write_string(stream, observed) != 0 ||
        fputs(",\"why\":\"steam-devices fournit les règles et permissions nécessaires pour les nouvelles manettes Steam sous Ubuntu.\",", stream) == EOF ||
        fputs("\"impact\":", stream) == EOF || json_write_string(stream, impact) != 0 ||
        fputs(",\"next_step\":", stream) == EOF || json_write_string(stream, next_step) != 0 ||
        fputs("},\"summary\":", stream) == EOF || json_write_string(stream, summary) != 0 ||
        fputs("}", stream) == EOF ? -1 : 0;
}

static int write_controller_diagnostic(FILE *stream, const SteamInfo *steam)
{
    const bool any_controller = steam->controller_count > 0U || steam->controller_detected;
    const char *detected_name = steam->controller_count > 0U ? steam->controllers[0].name : steam->controller_name;
    const char *severity = any_controller ? "ok" : "info";
    const char *title = steam->controller_detected ? "Steam Controller détectée" : any_controller
        ? "Manette de jeu détectée" : "Aucune manette de jeu connectée";
    const char *summary = any_controller
        ? "La manette est visible par le noyau Linux." : "Branchez ou connectez une manette pour vérifier sa détection.";

    if (fputs("{\"id\":\"steam.controller.detected\",\"severity\":", stream) == EOF ||
        json_write_string(stream, severity) != 0 || fputs(",\"title\":", stream) == EOF ||
        json_write_string(stream, title) != 0 || fputs(",\"evidence\":[{\"label\":\"Noyau Linux\",\"value\":", stream) == EOF ||
        json_write_string(stream, any_controller ? detected_name : "Aucune manette de jeu détectée") != 0 ||
        fputs("}],\"recommendations\":[{\"label\":\"Utiliser le test d'entrée dans les réglages Steam lorsque la manette est connectée\",\"priority\":\"medium\"}],\"explanation\":{\"observed\":", stream) == EOF ||
        json_write_string(stream, summary) != 0 ||
        fputs(",\"why\":\"La détection par le noyau est la première étape ; Steam Input et le jeu peuvent ensuite utiliser des règles et profils différents.\",\"impact\":\"Une manette absente du noyau ne peut pas être configurée dans Steam.\",\"next_step\":\"Vérifiez la connexion USB ou sans fil, puis lancez le test d'entrée Steam.\"},\"summary\":", stream) == EOF ||
        json_write_string(stream, summary) != 0 || fputs("}", stream) == EOF) return -1;
    return 0;
}

static int write_ubuntu_diagnostic(FILE *stream, const SteamInfo *steam)
{
    const char *severity = !steam->ubuntu ? "unknown" : steam->ubuntu_2604 && !steam->i386_available ? "warning" : "ok";
    const char *title = !steam->ubuntu ? "Distribution Ubuntu non détectée" : steam->ubuntu_2604 && !steam->i386_available
        ? "Prise en charge i386 à activer pour Steam" : "Pré-requis Ubuntu pour Steam détectés";
    const char *summary = steam->ubuntu_2604 && !steam->i386_available
        ? "Steam et certains pilotes graphiques ont besoin des bibliothèques 32 bits." : "La version Ubuntu et l'architecture i386 sont vérifiées localement.";

    if (fputs("{\"id\":\"steam.ubuntu.runtime\",\"severity\":", stream) == EOF || json_write_string(stream, severity) != 0 ||
        fputs(",\"title\":", stream) == EOF || json_write_string(stream, title) != 0 || fputs(",\"evidence\":[{\"label\":\"Ubuntu\",\"value\":", stream) == EOF ||
        json_write_string(stream, steam->ubuntu ? steam->ubuntu_version : "non détectée") != 0 ||
        fputs("},{\"label\":\"Architecture i386\",\"value\":", stream) == EOF || json_write_string(stream, steam->i386_available ? "activée" : "absente ou non détectée") != 0 ||
        fputs("}],\"recommendations\":[{\"label\":\"Installer les bibliothèques graphiques i386 correspondant au pilote si Steam les signale manquantes\",\"priority\":\"high\"}],\"explanation\":{\"observed\":", stream) == EOF ||
        json_write_string(stream, summary) != 0 ||
        fputs(",\"why\":\"Le client Steam et certains composants de rendu ont encore besoin de bibliothèques 32 bits sur Ubuntu.\",\"impact\":\"Des dépendances i386 manquantes peuvent empêcher Steam ou Vulkan de démarrer correctement.\",\"next_step\":\"Sur Ubuntu 26.04, conservez les pilotes graphiques et leurs paquets i386 synchronisés ; n'acceptez pas une résolution de paquets qui supprimerait le pilote.\"},\"summary\":", stream) == EOF ||
        json_write_string(stream, summary) != 0 || fputs("}", stream) == EOF) return -1;
    return 0;
}

static int write_gaming_scope_diagnostic(FILE *stream)
{
    return fputs(
        "{\"id\":\"gaming.scope\",\"severity\":\"info\",\"title\":\"Périmètre gaming actuel\","
        "\"evidence\":[{\"label\":\"Inclus\",\"value\":\"Steam, contrôleurs, bibliothèques et plan de migration\"},"
        "{\"label\":\"Graphismes\",\"value\":\"GPU, pilote noyau et présence des socles Vulkan/OpenGL\"},"
        "{\"label\":\"À venir\",\"value\":\"Rendu réel, versions, HDR, VRR et performances\"}],"
        "\"recommendations\":[{\"label\":\"Consulter la catégorie Graphismes avant une session de jeu importante\",\"priority\":\"low\"}],"
        "\"explanation\":{\"observed\":\"Linux Doctor sépare désormais le socle Steam des signaux graphiques locaux.\","
        "\"why\":\"La présence des fichiers et pilotes ne suffit pas à prédire les performances ou la compatibilité d'un jeu.\","
        "\"impact\":\"Une catégorie Graphismes sans avertissement confirme le socle détectable, pas le bon fonctionnement de tous les jeux.\","
        "\"next_step\":\"Consultez les diagnostics Graphismes ; les tests de rendu et de fonctionnalités avancées seront ajoutés séparément.\"},"
        "\"summary\":\"Steam et le socle graphique local sont couverts sans lancer de benchmark.\"}", stream) == EOF ? -1 : 0;
}

static int write_gfn_diagnostic(FILE *stream, const GeForceNowInfo *gfn)
{
    const char *severity = !gfn->installed ? "info" : !gfn->ubuntu_supported ? "warning" : "ok";
    const char *title = !gfn->installed ? "GeForce NOW non détecté" : "GeForce NOW pour Linux détecté";

    if (fputs("{\"id\":\"gaming.geforce_now.availability\",\"severity\":", stream) == EOF ||
        json_write_string(stream, severity) != 0 || fputs(",\"title\":", stream) == EOF ||
        json_write_string(stream, title) != 0 || fputs(",\"evidence\":[{\"label\":\"Application Flatpak officielle\",\"value\":", stream) == EOF ||
        json_write_string(stream, gfn->official_flatpak ? "détectée" : "non détectée") != 0 ||
        fputs("},{\"label\":\"Ubuntu pris en charge\",\"value\":", stream) == EOF ||
        json_write_string(stream, gfn->ubuntu_supported ? "Ubuntu 24.04 ou ultérieur" : "à vérifier") != 0 ||
        fputs("}],\"recommendations\":[{\"label\":\"Mettre à jour l'application depuis son gestionnaire Flatpak avant une session\",\"priority\":\"low\"}],\"explanation\":{\"observed\":", stream) == EOF ||
        json_write_string(stream, gfn->installed ? "L'identifiant Flatpak officiel com.nvidia.geforcenow est présent localement." : "Aucune application GeForce NOW Flatpak officielle n'a été trouvée.") != 0 ||
        fputs(",\"why\":\"La prise en charge Linux officielle commence avec Ubuntu 24.04 et reste une application à mettre à jour séparément.\","
            "\"impact\":\"Linux Doctor ne contacte pas Internet : il ne peut pas affirmer que la version locale est la toute dernière.\","
            "\"next_step\":\"Utilisez votre gestionnaire Flatpak pour consulter les mises à jour disponibles.\"},\"summary\":", stream) == EOF ||
        json_write_string(stream, title) != 0 || fputs("}", stream) == EOF) return -1;
    return 0;
}

static int write_gfn_wayland_diagnostic(FILE *stream, const GeForceNowInfo *gfn)
{
    if (!gfn->installed || !gfn->wayland_session || !gfn->controller_available) return 0;
    return fputs(
        "{\"id\":\"gaming.geforce_now.wayland_controller_portal\",\"severity\":\"info\","
        "\"title\":\"Manette et autorisation Wayland\","
        "\"evidence\":[{\"label\":\"Session\",\"value\":\"Wayland\"},{\"label\":\"Manette de jeu\",\"value\":\"détectée\"}],"
        "\"recommendations\":[{\"label\":\"N'accepter le partage/contrôle du bureau que lorsque la fenêtre attendue s'affiche et que vous utilisez le mode souris/bureau\",\"priority\":\"medium\"}],"
        "\"explanation\":{\"observed\":\"Wayland limite l'émulation de souris et de clavier par les applications.\","
        "\"why\":\"Steam Input peut demander un portail de partage ou de bureau lorsqu'une commande de la manette active le mode souris/bureau.\","
        "\"impact\":\"Une fenêtre d'autorisation peut apparaître en appuyant sur un bouton ; cela ne signifie pas à lui seul que GeForce NOW est en panne.\","
        "\"next_step\":\"Vérifiez d'abord le test d'entrée Steam. Pour une demande de portail attendue, lisez son origine avant de l'accepter ; refusez toute demande inattendue.\"},"
        "\"summary\":\"Sous Wayland, une demande de portail peut apparaître lorsqu'une manette active un mode souris ou bureau.\"}", stream) == EOF ? -1 : 0;
}

static const char *updates_severity(const UpdatesInfo *updates)
{
    if (!updates->cache_available) return "unknown";
    if (updates->cache_age_days > 30U) return "problem";
    if (updates->cache_age_days > 7U) return "warning";
    if (!updates->inventory_available) return "unknown";
    if (updates->truncated) return "unknown";
    if (updates->package_count > 0U && (!updates->selection_available || !updates->metadata_available)) return "unknown";
    if (updates->security_count > 0U) return "warning";
    if (updates->package_count > 0U) return "info";
    return "ok";
}

static const char *updates_cache_severity(const UpdatesInfo *updates)
{
    if (!updates->cache_available) return "unknown";
    if (updates->cache_age_days > 30U) return "problem";
    if (updates->cache_age_days > 7U) return "warning";
    return "ok";
}

static int updates_score(const UpdatesInfo *updates)
{
    const char *severity = updates_severity(updates);
    if (strcmp(severity, "problem") == 0) return 45;
    if (strcmp(severity, "warning") == 0) return 75;
    if (strcmp(severity, "info") == 0) return 95;
    if (strcmp(severity, "ok") == 0) return 100;
    return 0;
}

static const char *updates_summary(const UpdatesInfo *updates)
{
    if (!updates->cache_available) return "Âge du cache APT indisponible.";
    if (!updates->inventory_available && updates->cache_age_days > 7U)
        return "Les index APT sont anciens et l'inventaire local est indisponible.";
    if (!updates->inventory_available) return "Inventaire local des mises à jour indisponible.";
    if (updates->truncated) return "Inventaire APT partiel : certains candidats peuvent manquer.";
    if (updates->package_count > 0U && !updates->selection_available)
        return "Candidats trouvés, mais leur état actuel n'a pas pu être confirmé.";
    if (updates->package_count > 0U && !updates->metadata_available)
        return "Candidats trouvés, mais leurs descriptions locales sont incomplètes.";
    if (updates->cache_age_days > 7U) return "Les index APT locaux doivent être actualisés.";
    if (updates->security_count > 0U) return "Des candidats proviennent d'un dépôt de sécurité.";
    if (updates->package_count > 0U) return "Des mises à jour candidates sont expliquées ci-dessous.";
    return "Aucune mise à jour candidate dans les index APT locaux.";
}

static int write_updates_diagnostic(FILE *stream, const UpdatesInfo *updates)
{
    const char *severity = updates_cache_severity(updates);
    const char *title;
    const char *summary;
    const char *observed;
    const char *impact;
    const char *next_step;

    if (!updates->cache_available) {
        title = "État des mises à jour indisponible";
        summary = "Le cache APT n'a pas pu être lu localement.";
        observed = "Aucun horodatage du cache APT n'est accessible sur cette machine.";
        impact = "Linux Doctor ne peut pas estimer si la liste des paquets est récente.";
        next_step = "Actualisez les informations de paquets avec votre gestionnaire habituel, puis relancez l'analyse.";
    } else if (updates->cache_age_days > 30U) {
        title = "Informations de mises à jour très anciennes";
        summary = "Le cache APT n'a pas été actualisé depuis plus de 30 jours.";
        observed = "La dernière actualisation locale des paquets date de plus de 30 jours.";
        impact = "Des correctifs de sécurité et de stabilité peuvent ne pas être visibles.";
        next_step = "Actualisez la liste des paquets, examinez les mises à jour proposées, puis installez celles que vous validez.";
    } else if (updates->cache_age_days > 7U) {
        title = "Informations de mises à jour à actualiser";
        summary = "Le cache APT date de plus de 7 jours.";
        observed = "La dernière actualisation locale des paquets date de plus d'une semaine.";
        impact = "Les mises à jour récemment publiées risquent de ne pas encore être proposées.";
        next_step = "Actualisez la liste des paquets et examinez les mises à jour proposées.";
    } else {
        title = "Informations de mises à jour récentes";
        summary = "Le cache APT a été actualisé récemment.";
        observed = "La dernière actualisation locale des paquets est récente.";
        impact = "Linux Doctor peut consulter un état local récent, sans affirmer que tous les correctifs sont installés.";
        next_step = "Continuez à examiner régulièrement les mises à jour proposées.";
    }
    if (fprintf(stream, "{\"id\":\"updates.apt.cache_age\",\"severity\":") < 0 ||
        json_write_string(stream, severity) != 0 || fputs(",\"title\":", stream) == EOF ||
        json_write_string(stream, title) != 0 || fputs(",\"evidence\":[", stream) == EOF) return -1;
    if (updates->cache_available) {
        if (fprintf(stream, "{\"label\":\"Âge du cache APT\",\"value\":\"%u jour%s\"}",
            updates->cache_age_days, updates->cache_age_days > 1U ? "s" : "") < 0) return -1;
    } else if (fputs("{\"label\":\"Source\",\"value\":\"cache APT inaccessible\"}", stream) == EOF) return -1;
    return fputs(
        "],\"recommendations\":[{\"label\":\"Lancer ./scripts/refresh-updates.sh dans un terminal pour actualiser les index\",\"priority\":\"medium\"}],"
        "\"explanation\":{\"observed\":", stream) == EOF || json_write_string(stream, observed) != 0 ||
        fputs(",\"why\":\"Un cache de paquets récent permet de voir les mises à jour disponibles sans que Linux Doctor ne contacte Internet.\",", stream) == EOF ||
        fputs("\"impact\":", stream) == EOF || json_write_string(stream, impact) != 0 ||
        fputs(",\"next_step\":", stream) == EOF || json_write_string(stream, next_step) != 0 ||
        fputs("},\"summary\":", stream) == EOF || json_write_string(stream, summary) != 0 ||
        fputs("}", stream) == EOF ? -1 : 0;
}

static int write_updates_candidates_diagnostic(FILE *stream, const UpdatesInfo *updates)
{
    const char *severity;
    const char *title;
    const char *summary;
    char observed[256];

    if (!updates->inventory_available) {
        severity = "unknown";
        title = "Liste des mises à jour indisponible";
        summary = "La simulation locale APT n'a pas pu être interprétée.";
        (void)snprintf(observed, sizeof(observed), "Aucun inventaire de candidats APT n'est disponible.");
    } else if (updates->truncated) {
        severity = "unknown";
        title = "Liste des mises à jour partielle";
        summary = "La sortie APT a dépassé une limite ou contenait une ligne non interprétable.";
        (void)snprintf(observed, sizeof(observed), "%zu candidat%s lisible%s, mais l'inventaire est incomplet.",
            updates->package_count, updates->package_count == 1U ? "" : "s",
            updates->package_count == 1U ? "" : "s");
    } else if (updates->package_count > 0U && !updates->selection_available) {
        severity = "unknown";
        title = "État actuel des candidats non confirmé";
        summary = "La liste des candidats est lisible, mais la seconde simulation APT a échoué.";
        (void)snprintf(observed, sizeof(observed), "%zu candidat%s trouvé%s ; leur état prêt, phasé ou différé reste inconnu.",
            updates->package_count, updates->package_count == 1U ? "" : "s",
            updates->package_count == 1U ? "" : "s");
    } else if (updates->package_count > 0U && !updates->metadata_available) {
        severity = "unknown";
        title = "Descriptions APT incomplètes";
        summary = "Au moins un candidat ne possède pas de métadonnées locales exploitables.";
        (void)snprintf(observed, sizeof(observed), "%zu description%s disponible%s pour %zu candidat%s.",
            updates->metadata_count, updates->metadata_count == 1U ? "" : "s",
            updates->metadata_count == 1U ? "" : "s", updates->package_count,
            updates->package_count == 1U ? "" : "s");
    } else if (updates->security_count > 0U) {
        severity = "warning";
        title = "Mises à jour issues d'un dépôt de sécurité";
        summary = "APT signale au moins un candidat fourni via une suite *-security.";
        (void)snprintf(observed, sizeof(observed), "%zu candidat%s, dont %zu via un dépôt de sécurité.",
            updates->package_count, updates->package_count > 1U ? "s" : "", updates->security_count);
    } else if (updates->package_count > 0U) {
        severity = "info";
        title = "Mises à jour candidates dans le cache local";
        summary = "Chaque paquet est présenté avec son rôle et son état APT actuel.";
        (void)snprintf(observed, sizeof(observed), "%zu candidat%s : %zu prêt%s, %zu en déploiement progressif.",
            updates->package_count, updates->package_count > 1U ? "s" : "", updates->ready_count,
            updates->ready_count == 1U ? "" : "s", updates->phased_count);
    } else {
        severity = "ok";
        title = "Aucun candidat APT dans le cache local";
        summary = updates->cache_available && updates->cache_age_days <= 7U
            ? "Les index locaux récents ne proposent actuellement aucune mise à jour."
            : "Le cache ne propose aucun candidat, mais sa fraîcheur doit être vérifiée.";
        (void)snprintf(observed, sizeof(observed), "La simulation APT locale ne contient aucune ligne de mise à jour.");
    }
    if (fputs("{\"id\":\"updates.apt.candidates\",\"severity\":", stream) == EOF ||
        json_write_string(stream, severity) != 0 || fputs(",\"title\":", stream) == EOF ||
        json_write_string(stream, title) != 0 || fputs(",\"evidence\":[", stream) == EOF) return -1;
    if (updates->inventory_available) {
        if (fprintf(stream,
            "{\"label\":\"Candidats\",\"value\":\"%zu\"},"
            "{\"label\":\"Prêts selon la simulation\",\"value\":\"%zu\"},"
            "{\"label\":\"Déploiement progressif\",\"value\":\"%zu\"},"
            "{\"label\":\"Différés\",\"value\":\"%zu\"},"
            "{\"label\":\"État inconnu\",\"value\":\"%zu\"},"
            "{\"label\":\"Retenus manuellement\",\"value\":\"%zu\"},"
            "{\"label\":\"Dépôt de sécurité\",\"value\":\"%zu\"}",
            updates->package_count, updates->ready_count, updates->phased_count,
            updates->deferred_count, updates->unknown_count, updates->held_count,
            updates->security_count) < 0) return -1;
    } else if (fputs("{\"label\":\"Source\",\"value\":\"simulation APT indisponible\"}", stream) == EOF) return -1;
    return fputs("],\"recommendations\":[{\"label\":\"Lire le rôle et l'état de chaque paquet avant toute installation\",\"priority\":\"medium\"}],\"explanation\":{\"observed\":", stream) == EOF ||
        json_write_string(stream, observed) != 0 ||
        fputs(",\"why\":\"La description APT explique le rôle du paquet ; elle ne constitue pas le journal des changements de cette version.\","
            "\"impact\":\"Une mise à jour prête, différée ou déployée progressivement ne doit pas être présentée de la même manière.\","
            "\"next_step\":\"Actualisez d'abord les index si nécessaire, puis examinez les paquets. Linux Doctor n'installe rien automatiquement.\"},\"summary\":", stream) == EOF ||
        json_write_string(stream, summary) != 0 || fputs("}", stream) == EOF ? -1 : 0;
}

static int write_updates_inventory(FILE *stream, const UpdatesInfo *updates)
{
    if (fputs("\"updates_inventory\":{\"cache_available\":", stream) == EOF ||
        fputs(updates->cache_available ? "true" : "false", stream) == EOF ||
        fputs(",\"cache_age_days\":", stream) == EOF) return -1;
    if (updates->cache_available) {
        if (fprintf(stream, "%u", updates->cache_age_days) < 0) return -1;
    } else if (fputs("null", stream) == EOF) return -1;
    if (fputs(",\"inventory_available\":", stream) == EOF ||
        fputs(updates->inventory_available ? "true" : "false", stream) == EOF ||
        fputs(",\"selection_available\":", stream) == EOF ||
        fputs(updates->selection_available ? "true" : "false", stream) == EOF ||
        fputs(",\"hold_information_available\":", stream) == EOF ||
        fputs(updates->hold_information_available ? "true" : "false", stream) == EOF ||
        fputs(",\"metadata_available\":", stream) == EOF ||
        fputs(updates->metadata_available ? "true" : "false", stream) == EOF ||
        fputs(",\"truncated\":", stream) == EOF ||
        fputs(updates->truncated ? "true" : "false", stream) == EOF ||
        fprintf(stream,
            ",\"counts\":{\"candidates\":%zu,\"ready\":%zu,\"phased\":%zu,\"deferred\":%zu,\"unknown\":%zu,"
            "\"security\":%zu,\"held\":%zu,\"with_metadata\":%zu},"
            "\"refresh\":{\"method\":\"terminal\",\"command\":\"./scripts/refresh-updates.sh\",\"requires_sudo\":true,"
            "\"installs_packages\":false},\"packages\":[",
            updates->package_count, updates->ready_count, updates->phased_count,
            updates->deferred_count, updates->unknown_count, updates->security_count,
            updates->held_count, updates->metadata_count) < 0) return -1;
    for (size_t index = 0U; index < updates->package_count; index++) {
        const AptUpdatePackage *package = &updates->packages[index];
        if (index > 0U && fputc(',', stream) == EOF) return -1;
        if (fputs("{\"name\":", stream) == EOF || json_write_string(stream, package->name) != 0 ||
            fputs(",\"architecture\":", stream) == EOF || json_write_string(stream, package->architecture) != 0 ||
            fputs(",\"installed_version\":", stream) == EOF || json_write_string(stream, package->installed_version) != 0 ||
            fputs(",\"candidate_version\":", stream) == EOF || json_write_string(stream, package->candidate_version) != 0 ||
            fputs(",\"source_package\":", stream) == EOF || json_write_string(stream, package->source_package) != 0 ||
            fputs(",\"repository\":", stream) == EOF || json_write_string(stream, package->repository) != 0 ||
            fputs(",\"origin\":", stream) == EOF || json_write_string(stream, package->origin) != 0 ||
            fputs(",\"section\":", stream) == EOF || json_write_string(stream, package->section) != 0 ||
            fputs(",\"purpose\":", stream) == EOF || json_write_string(stream, package->purpose) != 0 ||
            fputs(",\"description\":", stream) == EOF || json_write_string(stream, package->description) != 0 ||
            fputs(",\"security_origin\":", stream) == EOF ||
            fputs(package->security_origin ? "true" : "false", stream) == EOF ||
            fputs(",\"held\":", stream) == EOF || fputs(package->held ? "true" : "false", stream) == EOF ||
            fputs(",\"metadata_available\":", stream) == EOF ||
            fputs(package->metadata_available ? "true" : "false", stream) == EOF ||
            fputs(",\"state\":", stream) == EOF || json_write_string(stream, updates_state_name(package->state)) != 0 ||
            fputs(",\"phased_percentage\":", stream) == EOF) return -1;
        if (package->phased_percentage_available) {
            if (fprintf(stream, "%u", package->phased_percentage) < 0) return -1;
        } else if (fputs("null", stream) == EOF) return -1;
        if (fputc('}', stream) == EOF) return -1;
    }
    return fputs("]}", stream) == EOF ? -1 : 0;
}

static const char *apps_severity(const AppsInfo *apps)
{
    if (!apps->package_database_available) return "unknown";
    return apps->gnome_tweaks_installed ? "ok" : "info";
}

static int write_apps_inventory(FILE *stream, const AppsInfo *apps)
{
    if (fputs("\"apps_inventory\":{\"available\":", stream) == EOF ||
        fputs(apps->package_database_available ? "true" : "false", stream) == EOF ||
        fputs(",\"recommended\":[{\"id\":\"desktop.gnome_tweaks\",\"name\":\"Gnome Tweaks\","
            "\"summary\":\"Réglages avancés de GNOME : apparence, polices, extensions et comportements du bureau.\","
            "\"install_command\":\"sudo apt install gnome-tweaks\",\"installed\":", stream) == EOF ||
        fputs(apps->gnome_tweaks_installed ? "true" : "false", stream) == EOF) return -1;
    return fputs("}]}", stream) == EOF ? -1 : 0;
}

static int write_apps_diagnostic(FILE *stream, const AppsInfo *apps)
{
    const bool available = apps->package_database_available;
    const bool installed = apps->gnome_tweaks_installed;
    const char *title = !available ? "État de Gnome Tweaks indisponible" : installed
        ? "Gnome Tweaks détecté" : "Gnome Tweaks recommandé";
    const char *summary = !available ? "La base locale des paquets DPKG n'est pas accessible." : installed
        ? "Les réglages avancés de GNOME sont disponibles." : "Ajoutez les réglages avancés de GNOME si vous souhaitez personnaliser votre bureau.";
    const char *observed = !available ? "Linux Doctor ne peut pas lire la base locale des paquets." : installed
        ? "Le paquet gnome-tweaks est installé localement." : "Le paquet gnome-tweaks n'est pas installé localement.";
    const char *next_step = !available ? "Vérifiez que le gestionnaire de paquets est disponible, puis relancez l'analyse." : installed
        ? "Ouvrez Ajustements depuis le lanceur d'applications lorsque vous en avez besoin." : "Commande proposée, à lancer seulement si vous la validez : sudo apt install gnome-tweaks";

    if (fputs("{\"id\":\"desktop.gnome_tweaks\",\"severity\":", stream) == EOF ||
        json_write_string(stream, apps_severity(apps)) != 0 || fputs(",\"title\":", stream) == EOF ||
        json_write_string(stream, title) != 0 || fputs(",\"evidence\":[{\"label\":\"État\",\"value\":", stream) == EOF ||
        json_write_string(stream, !available ? "inconnu" : installed ? "installé" : "à installer") != 0 ||
        fputs("},{\"label\":\"Paquet\",\"value\":\"gnome-tweaks\"}],\"recommendations\":[{\"label\":", stream) == EOF ||
        json_write_string(stream, next_step) != 0 || fputs(",\"priority\":\"low\"}],\"explanation\":{\"observed\":", stream) == EOF ||
        json_write_string(stream, observed) != 0 ||
        fputs(",\"why\":\"Gnome Tweaks centralise des réglages avancés que les paramètres standards de GNOME ne présentent pas toujours.\","
            "\"impact\":\"Cette application est facultative : son absence n'indique pas une panne du système.\",\"next_step\":", stream) == EOF ||
        json_write_string(stream, next_step) != 0 || fputs("},\"summary\":", stream) == EOF ||
        json_write_string(stream, summary) != 0 || fputs("}", stream) == EOF) return -1;
    return 0;
}

static bool graphics_drivers_detected(const GraphicsInfo *graphics)
{
    size_t index;

    if (!graphics->device_inventory_available || graphics->device_count == 0U) return false;
    for (index = 0U; index < graphics->device_count; index++) {
        if (graphics->devices[index].driver[0] == '\0') return false;
    }
    return true;
}

static const char *graphics_severity(const GraphicsInfo *graphics)
{
    if (!graphics->device_inventory_available || graphics->device_count == 0U) return "unknown";
    if (!graphics_drivers_detected(graphics) || !graphics->vulkan_loader_available ||
        graphics->vulkan_icd_count == 0U || !graphics->opengl_loader_available) return "warning";
    if (!graphics->session_available) return "unknown";
    return "info";
}

static int graphics_score(const GraphicsInfo *graphics)
{
    const char *severity = graphics_severity(graphics);

    if (severity[0] == 'o') return 100;
    if (severity[0] == 'i') return 95;
    if (severity[0] == 'w') return 65;
    return 0;
}

int report_health_score(const StorageInfo *storage, const GraphicsInfo *graphics)
{
    const char *storage_status;
    const char *graphics_status;
    int storage_score;
    int current_graphics_score;

    if (storage == NULL || graphics == NULL) return 0;
    storage_status = severity_for(storage);
    graphics_status = graphics_severity(graphics);
    storage_score = storage_status[0] == 'u' ? 101 : score_for(storage_status);
    current_graphics_score = graphics_score(graphics);
    if (graphics_status[0] == 'u') current_graphics_score = 101;
    if (storage_score > 100) return current_graphics_score <= 100 ? current_graphics_score : 0;
    if (current_graphics_score > 100) return storage_score;
    return storage_score < current_graphics_score ? storage_score : current_graphics_score;
}

const char *report_health_severity(const StorageInfo *storage, const GraphicsInfo *graphics)
{
    const char *storage_status;
    const char *graphics_status;

    if (storage == NULL || graphics == NULL) return "unknown";
    storage_status = severity_for(storage);
    graphics_status = graphics_severity(graphics);
    if (storage_status[0] == 'p' || graphics_status[0] == 'p') return "problem";
    if (storage_status[0] == 'w' || graphics_status[0] == 'w') return "warning";
    if (storage_status[0] == 'u' || graphics_status[0] == 'u') return "unknown";
    if (storage_status[0] == 'i' || graphics_status[0] == 'i') return "info";
    return "ok";
}

bool report_health_complete(const StorageInfo *storage, const GraphicsInfo *graphics)
{
    if (storage == NULL || graphics == NULL) return false;
    return storage->available && graphics->device_inventory_available &&
        graphics->device_count > 0U && graphics->session_available;
}

static int write_graphics_inventory(FILE *stream, const GraphicsInfo *graphics)
{
    size_t index;

    if (fputs("\"graphics_inventory\":{\"available\":", stream) == EOF ||
        fputs(graphics->device_inventory_available ? "true" : "false", stream) == EOF ||
        fputs(",\"truncated\":", stream) == EOF ||
        fputs(graphics->inventory_truncated ? "true" : "false", stream) == EOF ||
        fputs(",\"session\":{\"available\":", stream) == EOF ||
        fputs(graphics->session_available ? "true" : "false", stream) == EOF ||
        fputs(",\"type\":", stream) == EOF || json_write_string(stream, graphics->session_type) != 0 ||
        fputs(",\"wayland\":", stream) == EOF || fputs(graphics->wayland_session ? "true" : "false", stream) == EOF ||
        fputs(",\"x11\":", stream) == EOF || fputs(graphics->x11_session ? "true" : "false", stream) == EOF ||
        fputs("},\"vulkan\":{\"loader_available\":", stream) == EOF ||
        fputs(graphics->vulkan_loader_available ? "true" : "false", stream) == EOF ||
        fprintf(stream, ",\"icd_manifest_candidate_count\":%zu}", graphics->vulkan_icd_count) < 0 ||
        fputs(",\"opengl\":{\"loader_available\":", stream) == EOF ||
        fputs(graphics->opengl_loader_available ? "true" : "false", stream) == EOF ||
        fputs("},\"devices\":[", stream) == EOF) return -1;
    for (index = 0U; index < graphics->device_count; index++) {
        const GraphicsDevice *device = &graphics->devices[index];

        if (index > 0U && fputc(',', stream) == EOF) return -1;
        if (fputs("{\"card\":", stream) == EOF || json_write_string(stream, device->card) != 0 ||
            fputs(",\"vendor\":", stream) == EOF || json_write_string(stream, device->vendor) != 0 ||
            fputs(",\"vendor_id\":", stream) == EOF || json_write_string(stream, device->vendor_id) != 0 ||
            fputs(",\"device_id\":", stream) == EOF || json_write_string(stream, device->device_id) != 0 ||
            fputs(",\"driver\":", stream) == EOF || json_write_string(stream, device->driver) != 0 ||
            fputs(",\"boot_vga\":", stream) == EOF ||
            fputs(device->boot_vga ? "true}" : "false}", stream) == EOF) return -1;
    }
    return fputs("]}", stream) == EOF ? -1 : 0;
}

static int write_future_lab_cpu(FILE *stream, const FutureLabCpuSnapshot *cpu)
{
    if (fputs("\"cpu\":{\"state\":", stream) == EOF ||
        json_write_string(stream, future_lab_state_name(cpu->state)) != 0 ||
        fputs(",\"source\":\"/proc/stat\"", stream) == EOF) return -1;
    if (cpu->state == FUTURE_LAB_STATE_AVAILABLE &&
        (fprintf(stream, ",\"logical_cpu_count\":%zu,\"counters\":{"
            "\"cumulative\":true,\"scope\":\"since_boot\",\"unit\":\"scheduler_ticks\","
            "\"user\":%" PRIu64 ",\"nice\":%" PRIu64 ",\"system\":%" PRIu64
            ",\"idle\":%" PRIu64 ",\"iowait\":%" PRIu64 ",\"irq\":%" PRIu64
            ",\"softirq\":%" PRIu64 ",\"steal\":%" PRIu64 ",\"busy\":%" PRIu64
            ",\"total\":%" PRIu64 "}",
            cpu->logical_cpu_count, cpu->user_ticks, cpu->nice_ticks, cpu->system_ticks,
            cpu->idle_ticks, cpu->iowait_ticks, cpu->irq_ticks, cpu->softirq_ticks,
            cpu->steal_ticks, cpu->busy_ticks, cpu->total_ticks) < 0)) return -1;
    return fputc('}', stream) == EOF ? -1 : 0;
}

static int write_future_lab_load(FILE *stream, const FutureLabLoadSnapshot *load)
{
    if (fputs("\"load\":{\"state\":", stream) == EOF ||
        json_write_string(stream, future_lab_state_name(load->state)) != 0 ||
        fputs(",\"source\":\"/proc/loadavg\"", stream) == EOF) return -1;
    if (load->state == FUTURE_LAB_STATE_AVAILABLE &&
        fprintf(stream, ",\"kind\":\"kernel_run_queue_average\","
            "\"one_minute\":%.2f,\"five_minutes\":%.2f,\"fifteen_minutes\":%.2f,"
            "\"running_tasks\":%" PRIu64 ",\"total_tasks\":%" PRIu64,
            load->one_minute, load->five_minutes, load->fifteen_minutes,
            load->running_tasks, load->total_tasks) < 0) return -1;
    return fputc('}', stream) == EOF ? -1 : 0;
}

static int write_future_lab_memory(FILE *stream, const FutureLabMemorySnapshot *memory)
{
    if (fputs("\"memory\":{\"state\":", stream) == EOF ||
        json_write_string(stream, future_lab_state_name(memory->state)) != 0 ||
        fputs(",\"source\":\"/proc/meminfo\"", stream) == EOF) return -1;
    if (memory->state == FUTURE_LAB_STATE_AVAILABLE &&
        fprintf(stream, ",\"unit\":\"KiB\",\"total\":%" PRIu64
            ",\"available\":%" PRIu64 ",\"used\":%" PRIu64
            ",\"buffers\":%" PRIu64 ",\"cached\":%" PRIu64
            ",\"swap_total\":%" PRIu64 ",\"swap_free\":%" PRIu64,
            memory->total_kib, memory->available_kib, memory->used_kib,
            memory->buffers_kib, memory->cached_kib, memory->swap_total_kib,
            memory->swap_free_kib) < 0) return -1;
    return fputc('}', stream) == EOF ? -1 : 0;
}

static int write_future_lab_network(FILE *stream, const FutureLabNetworkSnapshot *network)
{
    size_t index;

    if (fputs("\"network\":{\"state\":", stream) == EOF ||
        json_write_string(stream, future_lab_state_name(network->state)) != 0 ||
        fputs(",\"source\":\"/proc/net/dev\"", stream) == EOF) return -1;
    if (network->state != FUTURE_LAB_STATE_AVAILABLE) return fputc('}', stream) == EOF ? -1 : 0;
    if (fputs(",\"truncated\":", stream) == EOF ||
        fputs(network->truncated ? "true" : "false", stream) == EOF ||
        fprintf(stream, ",\"observed_interface_count\":%zu,\"reported_interface_count\":%zu,"
            "\"counters\":{\"cumulative\":true,\"rates_calculated\":false,"
            "\"received_bytes\":%" PRIu64 ",\"transmitted_bytes\":%" PRIu64 "},"
            "\"interfaces\":[", network->observed_interface_count, network->interface_count,
            network->received_bytes, network->transmitted_bytes) < 0) return -1;
    for (index = 0U; index < network->interface_count; index++) {
        const FutureLabNetworkInterface *interface = &network->interfaces[index];

        if (index > 0U && fputc(',', stream) == EOF) return -1;
        if (fputs("{\"name\":", stream) == EOF || json_write_string(stream, interface->name) != 0 ||
            fprintf(stream, ",\"counters\":{\"cumulative\":true,\"received_bytes\":%" PRIu64
                ",\"received_packets\":%" PRIu64 ",\"received_errors\":%" PRIu64
                ",\"received_dropped\":%" PRIu64 ",\"transmitted_bytes\":%" PRIu64
                ",\"transmitted_packets\":%" PRIu64 ",\"transmitted_errors\":%" PRIu64
                ",\"transmitted_dropped\":%" PRIu64 "}}",
                interface->received_bytes, interface->received_packets,
                interface->received_errors, interface->received_dropped,
                interface->transmitted_bytes, interface->transmitted_packets,
                interface->transmitted_errors, interface->transmitted_dropped) < 0) return -1;
    }
    return fputs("]}", stream) == EOF ? -1 : 0;
}

static int write_future_lab_disks(FILE *stream, const FutureLabDiskSnapshot *disks)
{
    size_t index;

    if (fputs("\"disks\":{\"state\":", stream) == EOF ||
        json_write_string(stream, future_lab_state_name(disks->state)) != 0 ||
        fputs(",\"source\":\"/proc/diskstats\"", stream) == EOF) return -1;
    if (disks->state != FUTURE_LAB_STATE_AVAILABLE) return fputc('}', stream) == EOF ? -1 : 0;
    if (fputs(",\"truncated\":", stream) == EOF ||
        fputs(disks->truncated ? "true" : "false", stream) == EOF ||
        fprintf(stream, ",\"observed_device_count\":%zu,\"reported_device_count\":%zu,"
            "\"skipped_pseudo_device_count\":%zu,\"counters_are_cumulative\":true,"
            "\"rates_calculated\":false,\"sector_size_not_interpreted\":true,\"devices\":[",
            disks->observed_device_count, disks->device_count,
            disks->skipped_pseudo_device_count) < 0) return -1;
    for (index = 0U; index < disks->device_count; index++) {
        const FutureLabDiskDevice *device = &disks->devices[index];

        if (index > 0U && fputc(',', stream) == EOF) return -1;
        if (fputs("{\"name\":", stream) == EOF || json_write_string(stream, device->name) != 0 ||
            fprintf(stream, ",\"counters\":{\"cumulative\":true,\"reads_completed\":%" PRIu64
                ",\"sectors_read\":%" PRIu64 ",\"writes_completed\":%" PRIu64
                ",\"sectors_written\":%" PRIu64 "}}",
                device->reads_completed, device->sectors_read, device->writes_completed,
                device->sectors_written) < 0) return -1;
    }
    return fputs("]}", stream) == EOF ? -1 : 0;
}

static int write_future_lab(FILE *stream, const FutureLabSnapshot *snapshot)
{
    const bool complete = snapshot->cpu.state == FUTURE_LAB_STATE_AVAILABLE &&
        snapshot->load.state == FUTURE_LAB_STATE_AVAILABLE &&
        snapshot->memory.state == FUTURE_LAB_STATE_AVAILABLE &&
        snapshot->network.state == FUTURE_LAB_STATE_AVAILABLE &&
        snapshot->disks.state == FUTURE_LAB_STATE_AVAILABLE;

    if (fputs("\"future_lab\":{\"read_only\":true,\"snapshot_kind\":\"single_local_snapshot\","
        "\"rates_calculated\":false,\"complete\":", stream) == EOF ||
        fputs(complete ? "true," : "false,", stream) == EOF ||
        write_future_lab_cpu(stream, &snapshot->cpu) != 0 || fputc(',', stream) == EOF ||
        write_future_lab_load(stream, &snapshot->load) != 0 || fputc(',', stream) == EOF ||
        write_future_lab_memory(stream, &snapshot->memory) != 0 || fputc(',', stream) == EOF ||
        write_future_lab_network(stream, &snapshot->network) != 0 || fputc(',', stream) == EOF ||
        write_future_lab_disks(stream, &snapshot->disks) != 0 || fputc('}', stream) == EOF) return -1;
    return 0;
}

static bool future_lab_has_available_source(const FutureLabSnapshot *snapshot)
{
    return snapshot->cpu.state == FUTURE_LAB_STATE_AVAILABLE ||
        snapshot->load.state == FUTURE_LAB_STATE_AVAILABLE ||
        snapshot->memory.state == FUTURE_LAB_STATE_AVAILABLE ||
        snapshot->network.state == FUTURE_LAB_STATE_AVAILABLE ||
        snapshot->disks.state == FUTURE_LAB_STATE_AVAILABLE;
}

static int write_future_lab_category(FILE *stream, const FutureLabSnapshot *snapshot)
{
    const bool available = future_lab_has_available_source(snapshot);
    const char *summary = available
        ? "Instantané local du CPU, de la charge, de la mémoire, du réseau et des disques, sans débit ni détection d'anomalie."
        : "Les sources locales /proc du Future Lab ne sont pas disponibles dans cette analyse.";

    if (fputs("{\"id\":\"future_lab\",\"name\":\"Future Lab\",\"icon\":\"🧪\",\"status\":", stream) == EOF ||
        json_write_string(stream, available ? "info" : "unknown") != 0 ||
        fputs(",\"score\":null,\"score_explanation\":", stream) == EOF ||
        json_write_string(stream, "Cette vue n'entre pas dans le score global : un instantané et des compteurs cumulatifs ne suffisent pas pour juger les performances ou détecter une anomalie.") != 0 ||
        fputs(",\"diagnostics\":[],\"summary\":", stream) == EOF ||
        json_write_string(stream, summary) != 0 || fputc('}', stream) == EOF) return -1;
    return 0;
}

static int write_graphics_driver_diagnostic(FILE *stream, const GraphicsInfo *graphics)
{
    const bool ready = graphics_drivers_detected(graphics);
    const bool missing = !graphics->device_inventory_available || graphics->device_count == 0U;
    const char *severity = missing ? "unknown" : ready ? "ok" : "warning";
    const char *title = !graphics->device_inventory_available ? "Inventaire GPU indisponible" :
        graphics->device_count == 0U ? "Aucun GPU DRM exposé" : ready ?
        "Pilote graphique associé" : "Pilote graphique non identifié";
    const char *summary = !graphics->device_inventory_available ?
        "Linux Doctor ne peut pas lire l'inventaire DRM local." : graphics->device_count == 0U ?
        "Aucune carte graphique principale n'est visible dans /sys/class/drm." : ready ?
        "Chaque carte graphique inventoriée expose un pilote noyau." :
        "Au moins une carte graphique n'expose pas de pilote noyau identifiable.";
    bool first_evidence = true;
    size_t index;

    if (fputs("{\"id\":\"graphics.gpu.driver\",\"severity\":", stream) == EOF ||
        json_write_string(stream, severity) != 0 || fputs(",\"title\":", stream) == EOF ||
        json_write_string(stream, title) != 0 || fputs(",\"evidence\":[", stream) == EOF) return -1;
    if (missing) {
        if (fputs("{\"label\":\"Source\",\"value\":\"/sys/class/drm\"}", stream) == EOF) return -1;
        first_evidence = false;
    }
    for (index = 0U; index < graphics->device_count; index++) {
        const GraphicsDevice *device = &graphics->devices[index];
        char label[64];
        char driver_label[64];
        char pci_identifier[40];

        if (!first_evidence && fputc(',', stream) == EOF) return -1;
        first_evidence = false;
        (void)snprintf(label, sizeof(label), "GPU %s", device->card);
        (void)snprintf(driver_label, sizeof(driver_label), "Pilote %s", device->card);
        if (device->vendor_id[0] != '\0' && device->device_id[0] != '\0') {
            (void)snprintf(pci_identifier, sizeof(pci_identifier), "%s:%s", device->vendor_id, device->device_id);
        } else {
            (void)snprintf(pci_identifier, sizeof(pci_identifier), "%s", "indisponible");
        }
        if (fputs("{\"label\":", stream) == EOF || json_write_string(stream, label) != 0 ||
            fputs(",\"value\":", stream) == EOF || json_write_string(stream, device->vendor) != 0 ||
            fputs(",\"detail\":", stream) == EOF || json_write_string(stream, pci_identifier) != 0 ||
            fputs("},{\"label\":", stream) == EOF || json_write_string(stream, driver_label) != 0 ||
            fputs(",\"value\":", stream) == EOF ||
            json_write_string(stream, device->driver[0] != '\0' ? device->driver : "non détecté") != 0 ||
            fputs("}", stream) == EOF) return -1;
    }
    return fputs("],\"recommendations\":[{\"label\":\"Vérifier le pilote proposé par la distribution si une carte reste sans pilote\",\"priority\":\"high\"}],"
        "\"explanation\":{\"observed\":", stream) == EOF || json_write_string(stream, summary) != 0 ||
        fputs(",\"why\":\"Le noyau doit associer un pilote à la carte graphique avant que les piles OpenGL ou Vulkan puissent l'utiliser.\","
            "\"impact\":\"Un pilote absent peut limiter l'affichage ou empêcher l'accélération graphique native.\","
            "\"next_step\":\"Cette vérification ne juge pas la version du pilote. Consultez les outils de votre distribution avant toute installation ou mise à jour.\"},"
            "\"summary\":", stream) == EOF || json_write_string(stream, summary) != 0 ||
        fputs("}", stream) == EOF ? -1 : 0;
}

static int write_vulkan_diagnostic(FILE *stream, const GraphicsInfo *graphics)
{
    const bool gpu_known = graphics->device_inventory_available && graphics->device_count > 0U;
    const bool ready = graphics->vulkan_loader_available && graphics->vulkan_icd_count > 0U;
    const char *severity = !gpu_known ? "unknown" : ready ? "info" : "warning";
    const char *title = !gpu_known ? "Fichiers Vulkan non vérifiables" : ready ?
        "Chargeur et fichiers Vulkan détectés" : "Socle Vulkan natif incomplet";
    const char *summary = !gpu_known ?
        "Aucun GPU local ne permet de relier les fichiers Vulkan à un périphérique." : ready ?
        "Le chargeur Vulkan et au moins un fichier manifeste ICD candidat sont présents." :
        "Le chargeur Vulkan ou son manifeste de pilote n'a pas été détecté.";

    if (fputs("{\"id\":\"graphics.vulkan.loader\",\"severity\":", stream) == EOF ||
        json_write_string(stream, severity) != 0 || fputs(",\"title\":", stream) == EOF ||
        json_write_string(stream, title) != 0 ||
        fputs(",\"evidence\":[{\"label\":\"Chargeur libvulkan.so.1\",\"value\":", stream) == EOF ||
        json_write_string(stream, graphics->vulkan_loader_available ? "chargeable" : "non détecté") != 0 ||
        fprintf(stream, "},{\"label\":\"Manifestes ICD candidats\",\"value\":\"%zu\"}]", graphics->vulkan_icd_count) < 0 ||
        fputs(",\"recommendations\":[{\"label\":\"Vérifier les paquets Vulkan du pilote et de la distribution\",\"priority\":\"high\"}],"
            "\"explanation\":{\"observed\":", stream) == EOF || json_write_string(stream, summary) != 0 ||
        fputs(",\"why\":\"Le chargeur Vulkan s'appuie sur un manifeste ICD pour trouver le pilote graphique installé.\","
            "\"impact\":\"Un socle natif incomplet peut empêcher les jeux Vulkan ou Proton d'initialiser leur rendu.\","
            "\"next_step\":\"Linux Doctor n'a lancé aucun rendu Vulkan : utilisez ensuite un outil de test adapté pour confirmer le fonctionnement réel.\"},"
            "\"summary\":", stream) == EOF || json_write_string(stream, summary) != 0 ||
        fputs("}", stream) == EOF) return -1;
    return 0;
}

static int write_opengl_diagnostic(FILE *stream, const GraphicsInfo *graphics)
{
    const bool gpu_known = graphics->device_inventory_available && graphics->device_count > 0U;
    const char *severity = !gpu_known ? "unknown" : graphics->opengl_loader_available ? "info" : "warning";
    const char *title = !gpu_known ? "Socle OpenGL non vérifiable" : graphics->opengl_loader_available ?
        "Chargeur OpenGL local détecté" : "Chargeur OpenGL natif non détecté";
    const char *summary = !gpu_known ? "Aucun GPU local n'est disponible pour cette conclusion." :
        graphics->opengl_loader_available ? "La bibliothèque native libGL.so.1 peut être chargée." :
        "La bibliothèque native libGL.so.1 ne peut pas être chargée par Linux Doctor.";

    if (fputs("{\"id\":\"graphics.opengl.loader\",\"severity\":", stream) == EOF ||
        json_write_string(stream, severity) != 0 || fputs(",\"title\":", stream) == EOF ||
        json_write_string(stream, title) != 0 ||
        fputs(",\"evidence\":[{\"label\":\"Chargeur libGL.so.1\",\"value\":", stream) == EOF ||
        json_write_string(stream, graphics->opengl_loader_available ? "chargeable" : "non détecté") != 0 ||
        fputs("}],\"recommendations\":[{\"label\":\"Vérifier les bibliothèques OpenGL fournies par le pilote\",\"priority\":\"medium\"}],"
            "\"explanation\":{\"observed\":", stream) == EOF || json_write_string(stream, summary) != 0 ||
        fputs(",\"why\":\"De nombreuses applications graphiques utilisent encore OpenGL directement ou comme solution de repli.\","
            "\"impact\":\"Un chargeur absent peut empêcher une application native OpenGL de démarrer.\","
            "\"next_step\":\"La présence du chargeur ne valide ni le moteur de rendu ni l'accélération : un test OpenGL dédié reste nécessaire.\"},"
            "\"summary\":", stream) == EOF || json_write_string(stream, summary) != 0 ||
        fputs("}", stream) == EOF) return -1;
    return 0;
}

static int write_graphics_session_diagnostic(FILE *stream, const GraphicsInfo *graphics)
{
    const char *severity = graphics->session_available ? "info" : "unknown";
    const char *title = !graphics->session_available ? "Session graphique non détectée" :
        graphics->wayland_session ? "Session Wayland détectée" : graphics->x11_session ?
        "Session X11 détectée" : "Type de session détecté";
    const char *summary = graphics->session_available ?
        "Le type de session est lu depuis l'environnement local du processus." :
        graphics->session_type[0] != '\0' ? "Le contexte détecté n'est ni une session Wayland ni une session X11." :
        "L'analyse semble avoir été lancée hors d'une session graphique ou sans variables de session.";

    if (fputs("{\"id\":\"graphics.session\",\"severity\":", stream) == EOF ||
        json_write_string(stream, severity) != 0 || fputs(",\"title\":", stream) == EOF ||
        json_write_string(stream, title) != 0 ||
        fputs(",\"evidence\":[{\"label\":\"Type de session\",\"value\":", stream) == EOF ||
        json_write_string(stream, graphics->session_type[0] != '\0' ? graphics->session_type : "indisponible") != 0 ||
        fputs("}],\"recommendations\":[{\"label\":\"Relancer l'analyse depuis la session de bureau pour connaître Wayland ou X11\",\"priority\":\"low\"}],"
            "\"explanation\":{\"observed\":", stream) == EOF || json_write_string(stream, summary) != 0 ||
        fputs(",\"why\":\"Le protocole de session influence la capture d'écran, les portails de bureau, le HDR, le VRR et certains outils de jeu.\","
            "\"impact\":\"Un type absent signifie seulement que le contexte de lancement ne permet pas de conclure.\","
            "\"next_step\":\"Aucune session n'est considérée meilleure par défaut ; ce fait servira aux diagnostics plus ciblés.\"},"
            "\"summary\":", stream) == EOF || json_write_string(stream, summary) != 0 ||
        fputs("}", stream) == EOF) return -1;
    return 0;
}

static int write_good_news_item(FILE *stream, bool *first, const char *title,
    const char *category, const char *detail)
{
    if (!*first && fputc(',', stream) == EOF) return -1;
    *first = false;
    return fputs("{\"title\":", stream) == EOF || json_write_string(stream, title) != 0 ||
        fputs(",\"category\":", stream) == EOF || json_write_string(stream, category) != 0 ||
        fputs(",\"detail\":", stream) == EOF || json_write_string(stream, detail) != 0 ||
        fputs("}", stream) == EOF ? -1 : 0;
}

static int write_good_news(FILE *stream, const StorageInfo *storage, const GraphicsInfo *graphics)
{
    bool first = true;
    char vulkan_detail[160];

    if (fputs("\"good_news\":[", stream) == EOF) return -1;
    if (storage->available && storage->used_percent < 85U &&
        write_good_news_item(stream, &first, "Espace système disponible", "Stockage",
            "La partition système conserve une marge supérieure au seuil d'avertissement de Linux Doctor.") != 0) return -1;
    if (graphics_drivers_detected(graphics) &&
        write_good_news_item(stream, &first, "Pilote graphique associé", "Graphismes",
            "Chaque carte DRM inventoriée expose un pilote noyau ; sa version n'est pas encore évaluée.") != 0) return -1;
    if (graphics->device_count > 0U && graphics->vulkan_loader_available && graphics->vulkan_icd_count > 0U) {
        (void)snprintf(vulkan_detail, sizeof(vulkan_detail),
            "Le chargeur Vulkan et %zu manifeste%s ICD candidat%s sont présents ; aucun rendu n'a été lancé.",
            graphics->vulkan_icd_count, graphics->vulkan_icd_count > 1U ? "s" : "",
            graphics->vulkan_icd_count > 1U ? "s" : "");
        if (write_good_news_item(stream, &first, "Fichiers Vulkan détectés", "Graphismes", vulkan_detail) != 0) return -1;
    }
    if (graphics->device_count > 0U && graphics->opengl_loader_available &&
        write_good_news_item(stream, &first, "Chargeur OpenGL présent", "Graphismes",
            "La bibliothèque native OpenGL est chargeable ; le rendu réel reste à tester.") != 0) return -1;
    return fputs("]", stream) == EOF ? -1 : 0;
}

static int write_history(FILE *stream, const StorageInfo *storage, const GraphicsInfo *graphics,
    const HistoryComparison *history)
{
    if (history == NULL || !history->enabled) return fputs("\"history\":{\"enabled\":false}", stream) == EOF ? -1 : 0;
    if (!history->current_score_complete) return fputs("\"history\":{\"enabled\":true,\"compatible\":false,\"reason\":\"score_incomplete\"}", stream) == EOF ? -1 : 0;
    if (!history->has_previous) return fputs("\"history\":{\"enabled\":true,\"compatible\":true,\"has_previous\":false}", stream) == EOF ? -1 : 0;
    return fprintf(stream,
        "\"history\":{\"enabled\":true,\"compatible\":true,\"has_previous\":true,\"previous_score\":%d,\"score_delta\":%d,"
        "\"storage_root\":{\"previous_used_percent\":%u,\"current_used_percent\":%u}}",
        history->previous_score, report_health_score(storage, graphics) - history->previous_score,
        history->previous_used_percent, storage->used_percent) < 0 ? -1 : 0;
}

static int write_volumes(FILE *stream, const VolumeInventory *volumes)
{
    size_t index;

    if (fputs("\"storage_inventory\":{\"available\":", stream) == EOF ||
        fputs(volumes->available ? "true" : "false", stream) == EOF ||
        fputs(",\"truncated\":", stream) == EOF || fputs(volumes->truncated ? "true" : "false", stream) == EOF ||
        fputs(",\"volumes\":[", stream) == EOF) return -1;
    for (index = 0; index < volumes->count; index++) {
        const Volume *volume = &volumes->items[index];

        if (index > 0 && fputc(',', stream) == EOF) return -1;
        if (fputs("{\"path\":", stream) == EOF || json_write_string(stream, volume->path) != 0 ||
            fputs(",\"parent_path\":", stream) == EOF || json_write_string(stream, volume->parent_path) != 0 ||
            fputs(",\"uuid\":", stream) == EOF || json_write_string(stream, volume->uuid) != 0 ||
            fputs(",\"label\":", stream) == EOF || json_write_string(stream, volume->label) != 0 ||
            fputs(",\"filesystem\":", stream) == EOF || json_write_string(stream, volume->filesystem) != 0 ||
            fputs(",\"partition_label\":", stream) == EOF || json_write_string(stream, volume->partition_label) != 0 ||
            fputs(",\"mountpoint\":", stream) == EOF || json_write_string(stream, volume->mountpoint) != 0 ||
            fputs(",\"transport\":", stream) == EOF || json_write_string(stream, volume->transport) != 0 ||
            fprintf(stream, ",\"size_bytes\":%" PRIu64 ",\"available_bytes\":%" PRIu64
                ",\"used_percent\":%u,\"mounted\":%s,\"read_only\":%s,\"removable\":%s",
                volume->size_bytes, volume->available_bytes, volume->used_percent,
                volume->mounted ? "true" : "false", volume->read_only ? "true" : "false",
                volume->removable ? "true" : "false") < 0 ||
            fprintf(stream, ",\"windows_system_component\":%s,\"windows_data_partition\":%s,"
                "\"windows_confirmed\":%s,\"windows_protected\":%s}",
                volume->windows_system_component ? "true" : "false",
                volume->windows_data_partition ? "true" : "false",
                volume->windows_confirmed ? "true" : "false",
                volume->windows_protected ? "true" : "false") < 0) return -1;
    }
    return fputs("]}", stream) == EOF ? -1 : 0;
}

static int write_game_icon_data(FILE *stream, const char *path)
{
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    unsigned char data[STEAM_ICON_MAX_BYTES + 1U];
    const char *mime;
    FILE *source;
    size_t length;
    size_t index;

    if (path == NULL || path[0] == '\0') return fputs("null", stream) == EOF ? -1 : 0;
    mime = strstr(path, ".png") != NULL ? "image/png" :
        (strstr(path, ".jpg") != NULL || strstr(path, ".jpeg") != NULL) ? "image/jpeg" : NULL;
    if (mime == NULL || (source = fopen(path, "rb")) == NULL) return fputs("null", stream) == EOF ? -1 : 0;
    length = fread(data, 1U, sizeof(data), source);
    if (ferror(source) || length == 0U || length > STEAM_ICON_MAX_BYTES) {
        (void)fclose(source);
        return fputs("null", stream) == EOF ? -1 : 0;
    }
    if (fclose(source) != 0) return fputs("null", stream) == EOF ? -1 : 0;
    if (fprintf(stream, "\"data:%s;base64,", mime) < 0) return -1;
    for (index = 0U; index < length; index += 3U) {
        unsigned int value = (unsigned int)data[index] << 16U;
        size_t remaining = length - index;

        if (remaining > 1U) value |= (unsigned int)data[index + 1U] << 8U;
        if (remaining > 2U) value |= data[index + 2U];
        if (fputc(alphabet[(value >> 18U) & 63U], stream) == EOF ||
            fputc(alphabet[(value >> 12U) & 63U], stream) == EOF ||
            fputc(remaining > 1U ? alphabet[(value >> 6U) & 63U] : '=', stream) == EOF ||
            fputc(remaining > 2U ? alphabet[value & 63U] : '=', stream) == EOF) return -1;
    }
    return fputc('"', stream) == EOF ? -1 : 0;
}

static int write_steam_inventory(FILE *stream, const SteamInfo *steam)
{
    size_t index;

    if (fputs("\"steam_inventory\":{\"truncated\":", stream) == EOF ||
        fputs(steam->inventory_truncated ? "true" : "false", stream) == EOF ||
        fputs(",\"ubuntu\":", stream) == EOF || fputs(steam->ubuntu ? "true" : "false", stream) == EOF ||
        fputs(",\"ubuntu_version\":", stream) == EOF || json_write_string(stream, steam->ubuntu_version) != 0 ||
        fputs(",\"i386_available\":", stream) == EOF || fputs(steam->i386_available ? "true" : "false", stream) == EOF ||
        fputs(",\"steam_devices_installed\":", stream) == EOF ||
        fputs(steam->steam_devices_installed ? "true" : "false", stream) == EOF ||
        fputs(",\"controller_detected\":", stream) == EOF ||
        fputs(steam->controller_detected ? "true" : "false", stream) == EOF ||
        fputs(",\"controller_name\":", stream) == EOF || json_write_string(stream, steam->controller_name) != 0 ||
        fputs(",\"controllers\":[", stream) == EOF) return -1;
    for (index = 0U; index < steam->controller_count; index++) {
        if (index > 0U && fputc(',', stream) == EOF) return -1;
        if (fputs("{\"name\":", stream) == EOF || json_write_string(stream, steam->controllers[index].name) != 0 ||
            fputs(",\"kind\":", stream) == EOF || json_write_string(stream, steam->controllers[index].kind) != 0 ||
            fputc('}', stream) == EOF) return -1;
    }
    if (fputs("],\"controller_count\":", stream) == EOF || fprintf(stream, "%zu", steam->controller_count) < 0 ||
        fputs(",\"libraries\":[", stream) == EOF) return -1;
    for (index = 0; index < steam->library_count; index++) {
        const SteamLibrary *library = &steam->libraries[index];

        if (index > 0 && fputc(',', stream) == EOF) return -1;
        if (fputs("{\"path\":", stream) == EOF || json_write_string(stream, library->path) != 0 ||
            fputs(",\"volume_path\":", stream) == EOF || json_write_string(stream, library->volume_path) != 0 ||
            fputs(",\"filesystem\":", stream) == EOF || json_write_string(stream, library->filesystem) != 0 ||
            fprintf(stream, ",\"available_bytes\":%" PRIu64 ",\"game_bytes\":%" PRIu64
                ",\"tool_bytes\":%" PRIu64 ",\"game_count\":%zu,\"tool_count\":%zu,\"mounted\":%s,\"writable\":%s}",
                library->available_bytes, library->game_bytes, library->tool_bytes,
                library->game_count, library->tool_count,
                library->mounted ? "true" : "false", library->writable ? "true" : "false") < 0) return -1;
    }
    if (fputs("],\"games\":[", stream) == EOF) return -1;
    for (index = 0; index < steam->game_count; index++) {
        const SteamGame *game = &steam->games[index];

        if (index > 0 && fputc(',', stream) == EOF) return -1;
        if (fputs("{\"appid\":", stream) == EOF || json_write_string(stream, game->appid) != 0 ||
            fputs(",\"name\":", stream) == EOF || json_write_string(stream, game->name) != 0 ||
            fputs(",\"kind\":", stream) == EOF || json_write_string(stream, game->is_tool ? "tool" : "game") != 0 ||
            fprintf(stream, ",\"size_bytes\":%" PRIu64 ",\"library_index\":%zu,\"directory_present\":%s,\"icon_data_uri\":",
                game->size_bytes, game->library_index, game->directory_present ? "true" : "false") < 0 ||
            write_game_icon_data(stream, game->icon_path) != 0 || fputc('}', stream) == EOF) return -1;
    }
    return fputs("]}", stream) == EOF ? -1 : 0;
}

static int write_gfn_inventory(FILE *stream, const GeForceNowInfo *gfn)
{
    if (fputs("\"gfn_inventory\":{\"installed\":", stream) == EOF ||
        fputs(gfn->installed ? "true" : "false", stream) == EOF ||
        fputs(",\"official_flatpak\":", stream) == EOF ||
        fputs(gfn->official_flatpak ? "true" : "false", stream) == EOF ||
        fputs(",\"ubuntu_supported\":", stream) == EOF ||
        fputs(gfn->ubuntu_supported ? "true" : "false", stream) == EOF ||
        fputs(",\"wayland_session\":", stream) == EOF ||
        fputs(gfn->wayland_session ? "true" : "false", stream) == EOF ||
        fputs(",\"controller_available\":", stream) == EOF ||
        fputs(gfn->controller_available ? "true" : "false", stream) == EOF ||
        fputc('}', stream) == EOF) return -1;
    return 0;
}

static int write_gaming_knowledge(FILE *stream, const GamingKnowledgeBase *knowledge)
{
    bool first = true;
    size_t index;

    if (fputs("\"gaming_knowledge\":{\"available\":", stream) == EOF ||
        fputs(knowledge->available ? "true" : "false", stream) == EOF ||
        fputs(",\"truncated\":", stream) == EOF ||
        fputs(knowledge->truncated ? "true" : "false", stream) == EOF ||
        fputs(",\"source\":", stream) == EOF ||
        json_write_string(stream, !knowledge->available ? "unavailable" :
            knowledge->user_database ? "user-update" : "bundled") != 0 ||
        fprintf(stream, ",\"schema_version\":%u", knowledge->schema_version) < 0 ||
        fputs(",\"version\":", stream) == EOF || json_write_string(stream, knowledge->version) != 0 ||
        fputs(",\"reviewed_on\":", stream) == EOF || json_write_string(stream, knowledge->reviewed_on) != 0 ||
        fputs(",\"manual_update_command\":\"./scripts/update-knowledge.sh\"", stream) == EOF ||
        fprintf(stream, ",\"total_entries\":%zu,\"relevant_entries\":%zu,\"invalid_entries\":%zu,\"entries\":[",
            knowledge->entry_count, knowledge->relevant_count, knowledge->invalid_count) < 0) return -1;
    for (index = 0U; index < knowledge->entry_count; index++) {
        const GamingKnowledgeEntry *entry = &knowledge->entries[index];

        if (!entry->relevant) continue;
        if (!first && fputc(',', stream) == EOF) return -1;
        first = false;
        if (fputs("{\"kind\":", stream) == EOF || json_write_string(stream, entry->kind) != 0 ||
            fputs(",\"target\":", stream) == EOF || json_write_string(stream, entry->target) != 0 ||
            fputs(",\"severity\":", stream) == EOF || json_write_string(stream, entry->severity) != 0 ||
            fputs(",\"title\":", stream) == EOF || json_write_string(stream, entry->title) != 0 ||
            fputs(",\"summary\":", stream) == EOF || json_write_string(stream, entry->summary) != 0 ||
            fputs(",\"guidance\":", stream) == EOF || json_write_string(stream, entry->guidance) != 0 ||
            fputs(",\"source_url\":", stream) == EOF || json_write_string(stream, entry->source_url) != 0 ||
            fputs(",\"updated_on\":", stream) == EOF || json_write_string(stream, entry->updated_on) != 0 ||
            fputc('}', stream) == EOF) return -1;
    }
    return fputs("]}", stream) == EOF ? -1 : 0;
}

static int write_migration_plan(FILE *stream, const MigrationPlan *plan)
{
    size_t index;

    if (fputs("\"steam_migration_plan\":{\"available\":", stream) == EOF ||
        fputs(plan->available ? "true" : "false", stream) == EOF ||
        fputs(",\"destination_path\":", stream) == EOF || json_write_string(stream, plan->destination_path) != 0 ||
        fputs(",\"destination_volume\":", stream) == EOF || json_write_string(stream, plan->destination_volume) != 0 ||
        fprintf(stream, ",\"destination_available_bytes\":%" PRIu64 ",\"target_free_bytes\":%" PRIu64
            ",\"selected_bytes\":%" PRIu64 ",\"game_indexes\":[",
            plan->destination_available_bytes, plan->target_free_bytes, plan->selected_bytes) < 0) return -1;
    for (index = 0; index < plan->game_count; index++) {
        if (index > 0 && fputc(',', stream) == EOF) return -1;
        if (fprintf(stream, "%zu", plan->game_indexes[index]) < 0) return -1;
    }
    return fputs("]}", stream) == EOF ? -1 : 0;
}

static int write_steam_library_diagnostics(FILE *stream, const SteamInfo *steam)
{
    size_t index;
    bool first = true;

    for (index = 0; index < steam->library_count; index++) {
        const SteamLibrary *library = &steam->libraries[index];
        const char *severity = !library->mounted ? "warning" : !library->writable ? "warning" : "info";
        const char *title = !library->mounted ? "Bibliothèque Steam indisponible" : !library->writable
            ? "Bibliothèque Steam en lecture seule" : "Bibliothèque Steam inventoriée";

        if (!first && fputc(',', stream) == EOF) return -1;
        first = false;
        if (fputs("{\"id\":\"steam.library.status\",\"severity\":", stream) == EOF ||
            json_write_string(stream, severity) != 0 || fputs(",\"title\":", stream) == EOF ||
            json_write_string(stream, title) != 0 || fputs(",\"evidence\":[{\"label\":\"Bibliothèque\",\"value\":", stream) == EOF ||
            json_write_string(stream, library->path) != 0 ||
            fprintf(stream, "},{\"label\":\"Jeux installés\",\"value\":\"%zu\"}],", library->game_count) < 0 ||
            fputs("\"recommendations\":[{\"label\":\"Vérifier le montage et les permissions avant toute migration\",\"priority\":\"medium\"}],"
                "\"explanation\":{\"observed\":\"La bibliothèque a été lue localement, sans modifier Steam.\","
                "\"why\":\"Steam a besoin d'un emplacement monté et inscriptible pour installer, mettre à jour ou déplacer un jeu.\","
                "\"impact\":\"Une bibliothèque indisponible ou en lecture seule ne doit pas être choisie comme destination.\","
                "\"next_step\":\"Utilisez le gestionnaire de stockage Steam pour toute opération sur les jeux.\"},\"summary\":", stream) == EOF ||
            json_write_string(stream, title) != 0 || fputs("}", stream) == EOF) return -1;
    }
    return 0;
}

static const char *storage_score_explanation(const StorageInfo *storage)
{
    if (!storage->available) return "Score indisponible : la capacité de la partition système n'a pas pu être mesurée.";
    if (storage->used_percent >= 95U) return "45 % : la partition système dépasse 95 % d'utilisation, ce qui peut gêner Steam, les téléchargements et les mises à jour.";
    if (storage->used_percent >= 85U) return "75 % : la partition système dépasse 85 % d'utilisation et mérite de retrouver de l'espace libre.";
    return "96 % : l'espace système mesuré est confortable. Les 4 points restants couvrent les contrôles de santé matérielle et de fiabilité qui ne sont pas encore réalisés.";
}

static const char *gaming_score_explanation(const SteamInfo *steam)
{
    return steam->steam_devices_installed && (!steam->ubuntu_2604 || steam->i386_available)
        ? "95 % : les prérequis Steam mesurables sont présents. Les 5 points restants correspondent aux tests réels de Proton, Steam Input et du lancement des jeux, qui ne sont pas encore exécutés."
        : "60 % : au moins un prérequis local important, comme steam-devices ou l'architecture i386 sous Ubuntu 26.04, reste à vérifier.";
}

static const char *graphics_score_explanation(const GraphicsInfo *graphics)
{
    if (graphics_score(graphics) == 95) return "95 % : le pilote noyau, Vulkan, OpenGL et la session graphique sont détectés. Les 5 points restants nécessitent un vrai test de rendu et de versions de pilotes.";
    if (graphics_score(graphics) == 65) return "65 % : un élément du socle graphique local — pilote, Vulkan ou OpenGL — manque ou n'a pas pu être confirmé.";
    return "Score indisponible : l'inventaire du GPU ou le contexte de session graphique est incomplet.";
}

static const char *updates_score_explanation(const UpdatesInfo *updates)
{
    if (!updates->cache_available || !updates->inventory_available) return "Score indisponible : les index ou la simulation APT ne sont pas assez complets pour conclure.";
    if (updates->cache_age_days > 30U) return "45 % : les index APT ont plus de 30 jours et ne décrivent probablement plus les mises à jour actuelles.";
    if (updates->truncated || (updates->package_count > 0U &&
        (!updates->selection_available || !updates->metadata_available)))
        return "Score indisponible : l'inventaire ou les métadonnées APT sont partiels, donc Linux Doctor ne transforme pas cette absence en faux bon résultat.";
    if (updates->cache_age_days > 7U || updates->security_count > 0U) return "75 % : les index sont anciens ou des candidats proviennent d'un dépôt de sécurité ; aucune installation n'est automatique.";
    if (updates->package_count > 0U) return "95 % : l'inventaire APT est exploitable et des candidats sont expliqués ; ils restent à examiner avant installation.";
    return "100 % : les index locaux récents sont lisibles et aucune mise à jour candidate n'est signalée par la simulation.";
}

int report_write(FILE *stream, const StorageInfo *storage, const UpdatesInfo *updates, const AppsInfo *apps,
    const SteamInfo *steam, const VolumeInventory *volumes, const MigrationPlan *migration,
    const GeForceNowInfo *gfn, const GraphicsInfo *graphics, const GamingKnowledgeBase *knowledge,
    const FutureLabSnapshot *future_lab, const HistoryComparison *history)
{
    const char *severity;
    int score;

    if (stream == NULL || storage == NULL || updates == NULL || apps == NULL || steam == NULL ||
        volumes == NULL || migration == NULL || gfn == NULL || graphics == NULL || knowledge == NULL ||
        future_lab == NULL) return -1;
    severity = report_health_severity(storage, graphics);
    score = report_health_score(storage, graphics);
    if (fprintf(stream, "{\n  \"schema_version\": 2,\n  \"application\":{\"name\":\"Linux Doctor Gamer Edition\",\"version\":\"%s\",\"repository\":\"https://github.com/wildcat7534/linuxDoctor_Steam\"},\n  ", LINUX_DOCTOR_VERSION) < 0 ||
        write_generated_at(stream) != 0 ||
        fprintf(stream, ",\n  \"system_health\": {\"score\": %d, \"scope\":[\"storage\",\"graphics\"],\"complete\":%s,\"label\": ",
            score, report_health_complete(storage, graphics) ? "true" : "false") < 0 ||
        json_write_string(stream, severity) != 0 ||
        fputs("},\n  ", stream) == EOF || write_history(stream, storage, graphics, history) != 0 ||
        fputs(",\n  ", stream) == EOF || write_volumes(stream, volumes) != 0 ||
        fputs(",\n  ", stream) == EOF || write_steam_inventory(stream, steam) != 0 ||
        fputs(",\n  ", stream) == EOF || write_gfn_inventory(stream, gfn) != 0 ||
        fputs(",\n  ", stream) == EOF || write_gaming_knowledge(stream, knowledge) != 0 ||
        fputs(",\n  ", stream) == EOF || write_migration_plan(stream, migration) != 0 ||
        fputs(",\n  ", stream) == EOF || write_apps_inventory(stream, apps) != 0 ||
        fputs(",\n  ", stream) == EOF || write_updates_inventory(stream, updates) != 0 ||
        fputs(",\n  ", stream) == EOF || write_graphics_inventory(stream, graphics) != 0 ||
        fputs(",\n  ", stream) == EOF || write_future_lab(stream, future_lab) != 0 ||
        fputs(",\n  ", stream) == EOF || write_good_news(stream, storage, graphics) != 0 ||
        fputs(",\n  \"categories\": [{\"id\": \"storage\", \"name\": \"Stockage\", \"status\": ", stream) == EOF ||
        json_write_string(stream, severity_for(storage)) != 0 ||
        fprintf(stream, ", \"score\": %d, \"score_explanation\":", score_for(severity_for(storage))) < 0 ||
        json_write_string(stream, storage_score_explanation(storage)) != 0 ||
        fputs(",\"diagnostics\":[", stream) == EOF ||
        write_storage_diagnostic(stream, storage) != 0 ||
        fputc(',', stream) == EOF || write_steamapps_diagnostic(stream, storage) != 0 ||
        fputc(',', stream) == EOF || write_other_storage_diagnostic(stream, storage) != 0 ||
        fputs("],\"summary\":\"Capacité de la partition système et espace libre.\"},{\"id\":\"gaming\",\"name\":\"Gaming\",\"status\":", stream) == EOF ||
        json_write_string(stream, steam->steam_devices_installed && (!steam->ubuntu_2604 || steam->i386_available) ? "ok" : "warning") != 0 ||
        fputs(",\"score\":", stream) == EOF ||
        fprintf(stream, "%d", steam->steam_devices_installed && (!steam->ubuntu_2604 || steam->i386_available) ? 95 : 60) < 0 ||
        fputs(",\"score_explanation\":", stream) == EOF || json_write_string(stream, gaming_score_explanation(steam)) != 0 ||
        fputs(",\"diagnostics\":[", stream) == EOF ||
        write_gaming_diagnostic(stream, steam) != 0 || fputc(',', stream) == EOF ||
        write_controller_diagnostic(stream, steam) != 0 || fputc(',', stream) == EOF ||
        write_ubuntu_diagnostic(stream, steam) != 0 ||
        fputc(',', stream) == EOF || write_gaming_scope_diagnostic(stream) != 0 ||
        fputc(',', stream) == EOF || write_gfn_diagnostic(stream, gfn) != 0 ||
        (gfn->installed && gfn->wayland_session && gfn->controller_available &&
            (fputc(',', stream) == EOF || write_gfn_wayland_diagnostic(stream, gfn) != 0)) ||
        (steam->library_count > 0 && (fputc(',', stream) == EOF || write_steam_library_diagnostics(stream, steam) != 0)) ||
        fputs("],\"summary\":", stream) == EOF ||
        json_write_string(stream, steam->controller_count > 0U || steam->controller_detected ? "Manette détectée et environnement Steam vérifié."
            : "Règles Steam et compatibilité Ubuntu vérifiées.") != 0 ||
        fputs("},{\"id\":\"graphics\",\"name\":\"Graphismes\",\"icon\":\"⚡\",\"status\":", stream) == EOF ||
        json_write_string(stream, graphics_severity(graphics)) != 0 ||
        fputs(",\"score\":", stream) == EOF || fprintf(stream, "%d", graphics_score(graphics)) < 0 ||
        fputs(",\"score_explanation\":", stream) == EOF || json_write_string(stream, graphics_score_explanation(graphics)) != 0 ||
        fputs(",\"diagnostics\":[", stream) == EOF ||
        write_graphics_driver_diagnostic(stream, graphics) != 0 || fputc(',', stream) == EOF ||
        write_vulkan_diagnostic(stream, graphics) != 0 || fputc(',', stream) == EOF ||
        write_opengl_diagnostic(stream, graphics) != 0 || fputc(',', stream) == EOF ||
        write_graphics_session_diagnostic(stream, graphics) != 0 ||
        fputs("],\"summary\":", stream) == EOF ||
        json_write_string(stream, !graphics->device_inventory_available ? "Inventaire graphique local indisponible." :
            graphics->device_count == 0U ? "Aucun GPU DRM exposé par le système." :
            graphics_severity(graphics)[0] == 'i' ? "Pilote et composants graphiques natifs détectés, sans test de rendu." :
            "Le socle graphique natif mérite une vérification.") != 0 ||
        fputs("},", stream) == EOF || write_future_lab_category(stream, future_lab) != 0 ||
        fputs(",{\"id\":\"updates\",\"name\":\"Mises à jour\",\"status\":", stream) == EOF ||
        json_write_string(stream, updates_severity(updates)) != 0 ||
        fputs(",\"score\":", stream) == EOF ||
        fprintf(stream, "%d", updates_score(updates)) < 0 ||
        fputs(",\"score_explanation\":", stream) == EOF || json_write_string(stream, updates_score_explanation(updates)) != 0 ||
        fputs(",\"diagnostics\":[", stream) == EOF ||
        write_updates_diagnostic(stream, updates) != 0 ||
        fputc(',', stream) == EOF || write_updates_candidates_diagnostic(stream, updates) != 0 ||
        fputs("],\"summary\":", stream) == EOF ||
        json_write_string(stream, updates_summary(updates)) != 0 ||
        fputs("},{\"id\":\"apps\",\"name\":\"Apps utiles\",\"icon\":\"🛠\",\"status\":", stream) == EOF ||
        json_write_string(stream, apps_severity(apps)) != 0 ||
        fputs(",\"score\":", stream) == EOF ||
        fprintf(stream, "%d", apps->package_database_available ? 100 : 0) < 0 ||
        fputs(",\"score_explanation\":", stream) == EOF ||
        json_write_string(stream, apps->package_database_available
            ? "100 % : l'état des applications conseillées a été lu. Leur installation reste facultative."
            : "Score indisponible : la base locale des paquets n'est pas lisible.") != 0 ||
        fputs(",\"diagnostics\":[", stream) == EOF || write_apps_diagnostic(stream, apps) != 0 ||
        fputs("],\"summary\":", stream) == EOF ||
        json_write_string(stream, !apps->package_database_available ? "État des applications recommandé indisponible."
            : apps->gnome_tweaks_installed ? "Applications utiles installées." : "Une application utile est proposée, sans obligation.") != 0 ||
        fputs("}]\n}\n", stream) == EOF) return -1;
    return ferror(stream) ? -1 : 0;
}
