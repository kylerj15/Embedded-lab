// xilinx headers
#include "xil_printf.h"
#include "xil_types.h"

// sensors
#include "xparameters.h"
#include "xgpio.h"
#include "Pmod_DHB1.h"
#include "PWM.h"

// Pmod DHB1
#define XPAR_PMOD_DHB1_0_GPIO_BASEADDR   0x44A10000
#define XPAR_PMOD_DHB1_0_PWM_BASEADDR   0x44A20000
#define PMOD_DHB1_CLOCK_FREQ_HZ   XPAR_CPU_CORE_CLOCK_FREQ_HZ
#define M1_CHANNEL 1
#define M2_CHANNEL 2

#define PWM_PERIOD 0x00029000 // 2ms
#define PWM_DUTY   0x00014800 // 50% duty cycle

XGpio DHB1_GPIO;
PmodDHB1 motor;

// Move to utils.h/.c in future
static void delay_ms(int ms) {
    for (int i = 0; i < 1500 * ms; i++)
        asm("nop");
}

int main() {
    // Initialize GPIO interface for DHB1
    DHB1_GPIO_Initialize(&DHB1_GPIO, XPAR_PMOD_DHB1_0_GPIO_BASEADDR);

    XGpio_SetDataDirection(&DHB1_GPIO, M1_CHANNEL, 0xC);
    XGpio_SetDataDirection(&DHB1_GPIO, M2_CHANNEL, 0xC);
    xil_printf("check 1\r\n");

    // Initialize motor instance
    DHB1_GPIO_Initialize(&DHB1_GPIO, XPAR_PMOD_DHB1_0_GPIO_BASEADDR);

    DHB1_begin(&motor,
            XPAR_PMOD_DHB1_0_GPIO_BASEADDR,
            XPAR_PMOD_DHB1_0_PWM_BASEADDR,
            PMOD_DHB1_CLOCK_FREQ_HZ,
            PWM_PERIOD * 3);
    xil_printf("check 2\r\n");

    // Set motor PWM duty cycle
    PWM_Set_Duty(XPAR_PMOD_DHB1_0_PWM_BASEADDR, PWM_PERIOD * 2, 0);
    PWM_Set_Duty(XPAR_PMOD_DHB1_0_PWM_BASEADDR, PWM_PERIOD * 2, 1);

    // Enable motor
    DHB1_motorEnable(&motor);
    xil_printf("check 3\r\n");

    u32 m1, m2, count = 0;
    while (count < 5) {
        // PWM status
        u32 PWM_ctrl_reg   = PWM_mReadReg(XPAR_PMOD_DHB1_0_PWM_BASEADDR, PWM_AXI_CTRL_REG_OFFSET);
        u32 PWM_status_reg = PWM_mReadReg(XPAR_PMOD_DHB1_0_PWM_BASEADDR, PWM_AXI_CTRL_REG_OFFSET);
        u32 PWM_period_reg = PWM_Get_Period(XPAR_PMOD_DHB1_0_PWM_BASEADDR);
        u32 PWM_duty_reg   = PWM_Get_Duty(XPAR_PMOD_DHB1_0_PWM_BASEADDR, 0);

        xil_printf("PWM Control: 0x%08x\r\n", PWM_ctrl_reg);
        xil_printf("PWM Status:  0x%08x\r\n", PWM_status_reg);
        xil_printf("PWM Period:  0x%08x\r\n", PWM_period_reg);
        xil_printf("PWM Duty:    0x%08x\r\n", PWM_duty_reg);

        // Read GPIO motor feedback
        m1 = XGpio_DiscreteRead(&DHB1_GPIO, M1_CHANNEL);
        m2 = XGpio_DiscreteRead(&DHB1_GPIO, M2_CHANNEL);
        xil_printf("Motor GPIO:  0x%08x, 0x%08x\r\n", m1, m2);

        delay_ms(3000);
        count++;
    }

    // Stop motor
    DHB1_motorDisable(&motor);
    return 0;
}
