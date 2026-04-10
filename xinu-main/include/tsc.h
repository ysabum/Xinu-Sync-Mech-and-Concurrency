#ifndef _TSC_H_
#define _TSC_H_

/*
 * rdtsc()
 * --------------------------------------------------------------------
 * Reads the CPU's timestamp counter (TSC) using the x86 RDTSC
 * instruction. The TSC increments every CPU cycle and provides a
 * high‑resolution, low‑overhead timing source.
 *
 * On the Intel Galileo (Quark SoC), the TSC is stable and runs at
 * ~399 MHz (or 400 MHz), making it suitable for precise latency measurement
 * inside the Xinu kernel.
 *
 * Returns:
 *   64‑bit cycle count formed by combining the low (EAX) and
 *   high (EDX) halves of the TSC register.
 */
static inline uint64 rdtsc(void)
{
    uint32 lo, hi;

    /* Inline assembly:
     *   - Executes RDTSC
     *   - Places lower 32 bits in EAX -> lo
     *   - Places upper 32 bits in EDX -> hi
     */
    __asm__ __volatile__ (
        "rdtsc"
        : "=a"(lo), "=d"(hi)
    );

    // Combine high and low halves into a full 64‑bit value
    return ((uint64)hi << 32) | lo;
}

#endif