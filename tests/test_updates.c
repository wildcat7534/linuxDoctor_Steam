#include "updates.h"

#include <assert.h>

int main(void)
{
    UpdatesInfo updates = {.available = true, .age_days = 99U};

    assert(updates_collect(NULL, NULL, 0) == -1);
    assert(updates_collect(&updates, NULL, 0) == 0 || !updates.available);
    if (updates.available) assert(updates.age_days < 100000U);
    return 0;
}
