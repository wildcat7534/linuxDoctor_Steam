#include "report.h"

#include "json.h"

#include <inttypes.h>

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

int report_write(FILE *stream, const StorageInfo *storage)
{
    const char *severity;
    int score;

    if (stream == NULL || storage == NULL) return -1;
    severity = severity_for(storage);
    score = score_for(severity);
    if (fprintf(stream, "{\n  \"schema_version\": 1,\n  \"system_health\": {\"score\": %d, \"label\": ", score) < 0 ||
        json_write_string(stream, severity) != 0 ||
        fputs("},\n  \"categories\": [{\"id\": \"storage\", \"name\": \"Stockage\", \"status\": ", stream) == EOF ||
        json_write_string(stream, severity) != 0 ||
        fprintf(stream, ", \"score\": %d, \"diagnostics\": [", score) < 0 ||
        write_storage_diagnostic(stream, storage) != 0 ||
        fputs("]}]\n}\n", stream) == EOF) return -1;
    return ferror(stream) ? -1 : 0;
}
