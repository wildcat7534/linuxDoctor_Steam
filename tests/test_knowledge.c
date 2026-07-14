#include "knowledge.h"

#include <assert.h>
#include <string.h>

int main(void)
{
    SteamInfo steam = {.game_count = 1U, .controller_detected = true, .controller_count = 1U,
        .controllers = {{.name = "Steam Controller", .kind = "steam"}},
        .games = {{.appid = "4242", .name = "Fixture Game"}}};
    GeForceNowInfo gfn = {0};
    GamingKnowledgeBase knowledge;
    char error[128];

    assert(gaming_knowledge_load(NULL, "tests/fixtures/gaming-knowledge.tsv",
        &steam, &gfn, error, sizeof(error)) == -1);
    assert(gaming_knowledge_load(&knowledge, "tests/fixtures/missing.tsv",
        &steam, &gfn, error, sizeof(error)) == -1);
    assert(!knowledge.available);
    assert(gaming_knowledge_load(&knowledge, "tests/fixtures/gaming-knowledge.tsv",
        &steam, &gfn, error, sizeof(error)) == 0);
    assert(knowledge.available);
    assert(knowledge.entry_count == 3U);
    assert(knowledge.relevant_count == 2U);
    assert(knowledge.invalid_count == 1U);
    assert(knowledge.entries[0].relevant);
    assert(strcmp(knowledge.entries[0].target, "4242") == 0);
    assert(!knowledge.entries[1].relevant);
    assert(knowledge.entries[2].relevant);
    return 0;
}
