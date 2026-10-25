/* Extra diagnostics on top of the LW protocol: new fields and lines only, never a changed one. */
#ifndef DIAG_H
#define DIAG_H
#include <stdint.h>

/* health.c: ADC1 reads of VREFINT and the die temperature sensor (set up on first use) */
void health_print_fields(void); /* " vdd=<mV> temp=<C>", or nothing if the ADC gave no reading */

/* links.c: the A2-A3 "sensor" and A4-A5 "radio" jumper pairs, " sensor=.. radio=.. estop=.." */
void links_print_fields(void);

/* crashdump.c: one burst of LW DUMP lines from the fault handler */
void crashdump_print(const uint32_t *frame, uint32_t sp, uint32_t cfsr, uint32_t hfsr);
#endif
