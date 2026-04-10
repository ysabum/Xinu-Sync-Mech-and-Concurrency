/*  main.c  - main */

#include <xinu.h>
#include <stdio.h>
#include "fsbench.h"
#include "tsc.h"

extern uint32 nsaddr;

process	main(void)
{
	/* Start the network, if possible */

	netstart();

	nsaddr = 0x800a0c10;

	recvclr();
	resume(create(shell, 8192, 50, "shell", 1, CONSOLE));


	// // Measure tick frequency for experiment documentation
	// kprintf("Measuring TSC frequency...\n");

	// uint64 t1 = rdtsc();
	// sleep(1);
	// uint64 t2 = rdtsc();

	// uint64 cycles = t2 - t1;
	// kprintf("Cycles per second = %u\n", (uint32)cycles);


	/* Wait for shell to exit and recreate it */

	while (TRUE) {
		receive();
		sleepms(200);
		kprintf("\n\nMain process recreating shell\n\n");
		resume(create(shell, 4096, 20, "shell", 1, CONSOLE));
	}

	return OK;		/* never reached */
}