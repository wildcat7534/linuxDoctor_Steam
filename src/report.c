#include "report.h"

#include "json.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static bool steam_devices_installed(void)
{
    FILE *stream = fopen("/var/lib/dpkg/status", "r");
    char line[512];
    bool in_package = false;

    if (stream == NULL) {
        return false;
    }
    while (fgets(line, sizeof(line), stream) != NULL) {
        if (strncmp(line, "Package: ", 9) == 0) {
            in_package = strcmp(line + 9, "steam-devices\n") == 0;
        } else if (in_package && strncmp(line, "Status: ", 8) == 0) {
            if (strstr(line, "install ok installed") != NULL) {
                (void)fclose(stream);
                return true;
            }
        } else if (line[0] == '\n' && in_package) {
            break;
        }
    }
    (void)fclose(stream);
    return false;
}

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

static int write_gaming_diagnostic(FILE *stream)
{
    const bool installed = steam_devices_installed();
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

int report_write(FILE *stream, const StorageInfo *storage, const UpdatesInfo *updates,
    const HistoryComparison *history)
{
    const char *severity;
    const bool steam_devices = steam_devices_installed();
    int score;

    if (stream == NULL || storage == NULL || updates == NULL) return -1;
    severity = severity_for(storage);
    score = score_for(severity);
    if (fprintf(stream, "{\n  \"schema_version\": 1,\n  \"system_health\": {\"score\": %d, \"label\": ", score) < 0 ||
        json_write_string(stream, severity) != 0 ||
        fputs("},\n  ", stream) == EOF || write_history(stream, storage, history) != 0 ||
        fputs(",\n  \"categories\": [{\"id\": \"storage\", \"name\": \"Stockage\", \"status\": ", stream) == EOF ||
        json_write_string(stream, severity) != 0 ||
        fprintf(stream, ", \"score\": %d, \"diagnostics\": [", score) < 0 ||
        write_storage_diagnostic(stream, storage) != 0 ||
        fputc(',', stream) == EOF || write_steamapps_diagnostic(stream, storage) != 0 ||
        fputc(',', stream) == EOF || write_other_storage_diagnostic(stream, storage) != 0 ||
        fputs("],\"summary\":\"Capacité de la partition système et espace libre.\"},{\"id\":\"steam\",\"name\":\"Steam & contrôleurs\",\"status\":", stream) == EOF ||
        json_write_string(stream, steam_devices ? "ok" : "warning") != 0 ||
        fputs(",\"score\":", stream) == EOF ||
        fprintf(stream, "%d", steam_devices ? 95 : 60) < 0 ||
        fputs(",\"diagnostics\":[", stream) == EOF ||
        write_gaming_diagnostic(stream) != 0 ||
        fputs("],\"summary\":", stream) == EOF ||
        json_write_string(stream, steam_devices ? "Règles des contrôleurs Steam installées." : "Règles des contrôleurs Steam absentes.") != 0 ||
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
