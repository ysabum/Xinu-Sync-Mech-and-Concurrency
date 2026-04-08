#ifndef _TSC_H_
#define _TSC_H_

static inline uint64 rdtsc(void)
{
    uint32 lo, hi;
    __asm__ __volatile__ (
        "rdtsc"
        : "=a"(lo), "=d"(hi)
    );
    return ((uint64)hi << 32) | lo;
}

#endif
