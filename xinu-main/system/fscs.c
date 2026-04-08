/* fscs.c
 * -----------------------------------------------------------------------------
 * Global accounting of time spent in filesystem critical sections.
 *
 * RQ4: "How much time is spent inside interrupt-disabled critical sections,
 * and how does this correlate with latency spikes under load?"
 *
 * We approximate this by:
 *  - Marking entry/exit of filesystem critical sections
 *    (e.g., regions protected by Lf_data.lf_mutex).
 *  - Using fs_getticks() to measure elapsed ticks per section.
 *  - Accumulating a global counter fs_cs_total_ticks.
 *  - fs_cs_snapshot() returns the current total so callers can
 *    compute deltas per operation.
 * -----------------------------------------------------------------------------
 */

#include <xinu.h>
#include "fsbench.h"

static uint64 fs_cs_total_ticks = 0;     // cumulative ticks in critical sections
static uint64 fs_cs_current_start = 0;   // start time of current section
static bool8  fs_cs_active = FALSE;      // are we currently inside a section?

// Enter a filesystem critical section
void fs_cs_enter(void)
{
    intmask im = disable();

    if (!fs_cs_active) {
        fs_cs_active        = TRUE;
        fs_cs_current_start = fs_getticks();
    }

    restore(im);
}

// Exit a filesystem critical section
void fs_cs_exit(void)
{
    uint64 now = fs_getticks();
    intmask im = disable();

    if (fs_cs_active) {
        fs_cs_total_ticks += (now - fs_cs_current_start);
        fs_cs_active = FALSE;
    }

    restore(im);
}

// Snapshot the cumulative critical-section ticks
uint32 fs_cs_snapshot(void)
{
    uint64 total;
    intmask im = disable();

    total = fs_cs_total_ticks;

    if (fs_cs_active) {
        uint64 now = fs_getticks();
        total += (now - fs_cs_current_start);
    }

    restore(im);
    return (uint32)total;
}
