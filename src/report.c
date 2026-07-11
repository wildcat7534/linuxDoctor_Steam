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

    if (fprintf(stream, "{\"id\":\"gaming.steam.devices\",\"severity\":") < 0 ||
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

int report_write(FILE *stream, const StorageInfo *storage, const HistoryComparison *history)
{
    const char *severity;
    const bool steam_devices = steam_devices_installed();
    int score;

    if (stream == NULL || storage == NULL) return -1;
    severity = severity_for(storage);
    score = score_for(severity);
    if (fprintf(stream, "{\n  \"schema_version\": 1,\n  \"system_health\": {\"score\": %d, \"label\": ", score) < 0 ||
        json_write_string(stream, severity) != 0 ||
        fputs("},\n  ", stream) == EOF || write_history(stream, storage, history) != 0 ||
        fputs(",\n  \"categories\": [{\"id\": \"storage\", \"name\": \"Stockage\", \"status\": ", stream) == EOF ||
        json_write_string(stream, severity) != 0 ||
        fprintf(stream, ", \"score\": %d, \"diagnostics\": [", score) < 0 ||
        write_storage_diagnostic(stream, storage) != 0 ||
        fputs("]},{\"id\":\"gaming\",\"name\":\"Gaming\",\"status\":", stream) == EOF ||
        json_write_string(stream, steam_devices ? "ok" : "warning") != 0 ||
        fputs(",\"score\":", stream) == EOF ||
        fprintf(stream, "%d", steam_devices ? 95 : 60) < 0 ||
        fputs(",\"diagnostics\":[", stream) == EOF ||
        write_gaming_diagnostic(stream) != 0 ||
        fputs("]}]\n}\n", stream) == EOF) return -1;
    return ferror(stream) ? -1 : 0;
}
