#include "report.h"

#include "json.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

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
    const char *severity = steam->controller_detected ? "ok" : "info";
    const char *title = steam->controller_detected ? "Steam Controller détectée" : "Aucune Steam Controller connectée";
    const char *summary = steam->controller_detected
        ? "La manette est visible par le noyau Linux." : "Branchez ou connectez la manette pour vérifier sa détection.";

    if (fputs("{\"id\":\"steam.controller.detected\",\"severity\":", stream) == EOF ||
        json_write_string(stream, severity) != 0 || fputs(",\"title\":", stream) == EOF ||
        json_write_string(stream, title) != 0 || fputs(",\"evidence\":[{\"label\":\"Noyau Linux\",\"value\":", stream) == EOF ||
        json_write_string(stream, steam->controller_detected ? steam->controller_name : "Aucune Steam Controller détectée") != 0 ||
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
        "{\"id\":\"gaming.scope\",\"severity\":\"info\",\"title\":\"Périmètre gaming actuel : Steam\","
        "\"evidence\":[{\"label\":\"Inclus\",\"value\":\"Steam, contrôleurs, bibliothèques et plan de migration\"},"
        "{\"label\":\"À venir\",\"value\":\"Pilotes graphiques, Vulkan et performances\"}],"
        "\"recommendations\":[{\"label\":\"Consulter les mises à jour système avant une session de jeu importante\",\"priority\":\"low\"}],"
        "\"explanation\":{\"observed\":\"Linux Doctor évalue actuellement le socle Steam local.\","
        "\"why\":\"Les pilotes et la pile graphique nécessitent des collecteurs dédiés pour donner un verdict fiable.\","
        "\"impact\":\"L'absence d'alerte graphique ne signifie pas encore que les pilotes sont validés.\","
        "\"next_step\":\"Les diagnostics Vulkan, pilotes et performances seront ajoutés au domaine Gaming.\"},"
        "\"summary\":\"Steam est couvert ; les diagnostics graphiques arriveront ensuite.\"}", stream) == EOF ? -1 : 0;
}

static const char *updates_severity(const UpdatesInfo *updates)
{
    if (!updates->available) return "unknown";
    if (updates->age_days > 30U) return "problem";
    if (updates->age_days > 7U) return "warning";
    return "ok";
}

static int write_updates_diagnostic(FILE *stream, const UpdatesInfo *updates)
{
    const char *severity = updates_severity(updates);
    const char *title;
    const char *summary;
    const char *observed;
    const char *impact;
    const char *next_step;

    if (!updates->available) {
        title = "État des mises à jour indisponible";
        summary = "Le cache APT n'a pas pu être lu localement.";
        observed = "Aucun horodatage du cache APT n'est accessible sur cette machine.";
        impact = "Linux Doctor ne peut pas estimer si la liste des paquets est récente.";
        next_step = "Actualisez les informations de paquets avec votre gestionnaire habituel, puis relancez l'analyse.";
    } else if (updates->age_days > 30U) {
        title = "Informations de mises à jour très anciennes";
        summary = "Le cache APT n'a pas été actualisé depuis plus de 30 jours.";
        observed = "La dernière actualisation locale des paquets date de plus de 30 jours.";
        impact = "Des correctifs de sécurité et de stabilité peuvent ne pas être visibles.";
        next_step = "Actualisez la liste des paquets, examinez les mises à jour proposées, puis installez celles que vous validez.";
    } else if (updates->age_days > 7U) {
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
    if (updates->available) {
        if (fprintf(stream, "{\"label\":\"Âge du cache APT\",\"value\":\"%u jour%s\"}",
            updates->age_days, updates->age_days > 1U ? "s" : "") < 0) return -1;
    } else if (fputs("{\"label\":\"Source\",\"value\":\"cache APT inaccessible\"}", stream) == EOF) return -1;
    return fputs(
        "],\"recommendations\":[{\"label\":\"Actualiser les informations de paquets\",\"priority\":\"medium\"}],"
        "\"explanation\":{\"observed\":", stream) == EOF || json_write_string(stream, observed) != 0 ||
        fputs(",\"why\":\"Un cache de paquets récent permet de voir les mises à jour disponibles sans que Linux Doctor ne contacte Internet.\",", stream) == EOF ||
        fputs("\"impact\":", stream) == EOF || json_write_string(stream, impact) != 0 ||
        fputs(",\"next_step\":", stream) == EOF || json_write_string(stream, next_step) != 0 ||
        fputs("},\"summary\":", stream) == EOF || json_write_string(stream, summary) != 0 ||
        fputs("}", stream) == EOF ? -1 : 0;
}

static int write_history(FILE *stream, const StorageInfo *storage, const HistoryComparison *history)
{
    if (history == NULL || !history->enabled) return fputs("\"history\":{\"enabled\":false}", stream) == EOF ? -1 : 0;
    if (!history->has_previous) return fputs("\"history\":{\"enabled\":true,\"has_previous\":false}", stream) == EOF ? -1 : 0;
    return fprintf(stream,
        "\"history\":{\"enabled\":true,\"has_previous\":true,\"previous_score\":%d,\"score_delta\":%d,"
        "\"storage_root\":{\"previous_used_percent\":%u,\"current_used_percent\":%u}}",
        history->previous_score, score_for(severity_for(storage)) - history->previous_score,
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
            fprintf(stream, ",\"windows_protected\":%s}", volume->windows_protected ? "true" : "false") < 0) return -1;
    }
    return fputs("]}", stream) == EOF ? -1 : 0;
}

static int write_steam_inventory(FILE *stream, const SteamInfo *steam)
{
    size_t index;

    if (fputs("\"steam_inventory\":{\"truncated\":", stream) == EOF ||
        fputs(steam->inventory_truncated ? "true" : "false", stream) == EOF ||
        fputs(",\"libraries\":[", stream) == EOF) return -1;
    for (index = 0; index < steam->library_count; index++) {
        const SteamLibrary *library = &steam->libraries[index];

        if (index > 0 && fputc(',', stream) == EOF) return -1;
        if (fputs("{\"path\":", stream) == EOF || json_write_string(stream, library->path) != 0 ||
            fputs(",\"volume_path\":", stream) == EOF || json_write_string(stream, library->volume_path) != 0 ||
            fputs(",\"filesystem\":", stream) == EOF || json_write_string(stream, library->filesystem) != 0 ||
            fprintf(stream, ",\"available_bytes\":%" PRIu64 ",\"game_bytes\":%" PRIu64
                ",\"game_count\":%zu,\"mounted\":%s,\"writable\":%s}",
                library->available_bytes, library->game_bytes, library->game_count,
                library->mounted ? "true" : "false", library->writable ? "true" : "false") < 0) return -1;
    }
    if (fputs("],\"games\":[", stream) == EOF) return -1;
    for (index = 0; index < steam->game_count; index++) {
        const SteamGame *game = &steam->games[index];

        if (index > 0 && fputc(',', stream) == EOF) return -1;
        if (fputs("{\"appid\":", stream) == EOF || json_write_string(stream, game->appid) != 0 ||
            fputs(",\"name\":", stream) == EOF || json_write_string(stream, game->name) != 0 ||
            fprintf(stream, ",\"size_bytes\":%" PRIu64 ",\"library_index\":%zu,\"directory_present\":%s}",
                game->size_bytes, game->library_index, game->directory_present ? "true" : "false") < 0) return -1;
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

int report_write(FILE *stream, const StorageInfo *storage, const UpdatesInfo *updates,
    const SteamInfo *steam, const VolumeInventory *volumes, const MigrationPlan *migration,
    const HistoryComparison *history)
{
    const char *severity;
    int score;

    if (stream == NULL || storage == NULL || updates == NULL || steam == NULL || volumes == NULL || migration == NULL) return -1;
    severity = severity_for(storage);
    score = score_for(severity);
    if (fprintf(stream, "{\n  \"schema_version\": 2,\n  \"system_health\": {\"score\": %d, \"label\": ", score) < 0 ||
        json_write_string(stream, severity) != 0 ||
        fputs("},\n  ", stream) == EOF || write_history(stream, storage, history) != 0 ||
        fputs(",\n  ", stream) == EOF || write_volumes(stream, volumes) != 0 ||
        fputs(",\n  ", stream) == EOF || write_steam_inventory(stream, steam) != 0 ||
        fputs(",\n  ", stream) == EOF || write_migration_plan(stream, migration) != 0 ||
        fputs(",\n  \"categories\": [{\"id\": \"storage\", \"name\": \"Stockage\", \"status\": ", stream) == EOF ||
        json_write_string(stream, severity) != 0 ||
        fprintf(stream, ", \"score\": %d, \"diagnostics\": [", score) < 0 ||
        write_storage_diagnostic(stream, storage) != 0 ||
        fputc(',', stream) == EOF || write_steamapps_diagnostic(stream, storage) != 0 ||
        fputc(',', stream) == EOF || write_other_storage_diagnostic(stream, storage) != 0 ||
        fputs("],\"summary\":\"Capacité de la partition système et espace libre.\"},{\"id\":\"gaming\",\"name\":\"Gaming\",\"status\":", stream) == EOF ||
        json_write_string(stream, steam->steam_devices_installed && (!steam->ubuntu_2604 || steam->i386_available) ? "ok" : "warning") != 0 ||
        fputs(",\"score\":", stream) == EOF ||
        fprintf(stream, "%d", steam->steam_devices_installed && (!steam->ubuntu_2604 || steam->i386_available) ? 95 : 60) < 0 ||
        fputs(",\"diagnostics\":[", stream) == EOF ||
        write_gaming_diagnostic(stream, steam) != 0 || fputc(',', stream) == EOF ||
        write_controller_diagnostic(stream, steam) != 0 || fputc(',', stream) == EOF ||
        write_ubuntu_diagnostic(stream, steam) != 0 ||
        fputc(',', stream) == EOF || write_gaming_scope_diagnostic(stream) != 0 ||
        (steam->library_count > 0 && (fputc(',', stream) == EOF || write_steam_library_diagnostics(stream, steam) != 0)) ||
        fputs("],\"summary\":", stream) == EOF ||
        json_write_string(stream, steam->controller_detected ? "Steam Controller détectée et environnement Steam vérifié."
            : "Règles Steam et compatibilité Ubuntu vérifiées.") != 0 ||
        fputs("},{\"id\":\"updates\",\"name\":\"Mises à jour\",\"status\":", stream) == EOF ||
        json_write_string(stream, updates_severity(updates)) != 0 ||
        fputs(",\"score\":", stream) == EOF ||
        fprintf(stream, "%d", !updates->available ? 0 : updates->age_days > 30U ? 45 : updates->age_days > 7U ? 75 : 100) < 0 ||
        fputs(",\"diagnostics\":[", stream) == EOF ||
        write_updates_diagnostic(stream, updates) != 0 ||
        fputs("],\"summary\":", stream) == EOF ||
        json_write_string(stream, !updates->available ? "Cache APT inaccessible." : updates->age_days > 7U ? "Informations de paquets à actualiser." : "Informations de paquets récentes.") != 0 ||
        fputs("}]\n}\n", stream) == EOF) return -1;
    return ferror(stream) ? -1 : 0;
}
