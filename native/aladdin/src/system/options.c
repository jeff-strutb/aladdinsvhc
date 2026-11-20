/*
 * options.c: the option settings, and the sum the original kept beside them
 * to tell a reset (the settings kept) from a power-on. The native game has
 * no reset of its own and no saves yet (plan decision 4), so the settings
 * live as long as the run; the sum is kept as the options screen keeps it.
 */
#include "system/state.h"
#include "system/system.h"

/*
 * @routine 80:80FE KeptOptionsSum
 * WHAT IT DOES: Adds up the three kept option bytes (8 bits).
 */
uint8_t KeptOptionsSum(void)
{
    return (uint8_t)(g.kept_options[0] + g.kept_options[1] + g.kept_options[2]);
}

/*
 * @routine 80:810A KeptOptionsSeal
 * WHAT IT DOES: Stores the kept option bytes' sum beside them, after one of
 * them has changed.
 */
void KeptOptionsSeal(void)
{
    g.kept_options_sum = KeptOptionsSum();
}
