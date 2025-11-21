
#include "PmodMAXSONAR.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xgpio.h"
#include "xintc.h"
#include "xtmrctr.h"
#include "xgpio_l.h"
#include "xil_types.h"

#define PMOD_SONAR0_BASEADDR 0x44A30000

// #ifdef __MICROBLAZE__
// #define CLK_FREQ XPAR_CPU_M_AXI_DP_FREQ_HZ
// #else
// #define CLK_FREQ 83333333 // FCLK0 frequency not found in xparameters.h
// #endif

#define CLK_FREQ 83333333

PmodMAXSONAR Sonar;

void delay_ms(int ms)
{
	for (int i = 0; i < 1500 * ms; i++)
	{
		asm("nop");
	}
}

int main()
{
	// Sonar example
	MAXSONAR_begin(&Sonar, PMOD_SONAR0_BASEADDR, CLK_FREQ);

	u32 dist1;
	while (1)
	{
		dist1 = MAXSONAR_getDistance(&Sonar);

		xil_printf("sonar = %3d, right = %3d\r", dist1);
		delay_ms(1000);
	}

	return 0;
}
