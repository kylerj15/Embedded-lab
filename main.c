#include "FreeRTOS.h"
#include "task.h"
#include "xil_printf.h"
#include "semphr.h"
#include "xil_types.h"
#include "xgpio.h"
#include "xparameters.h"
#include "Pmod_DHB1.h"
#include "Pmod_Dual_MAXSONAR.h"
#include "PWM.h"
#include "xintc.h"
#include "xtmrctr.h"

#define LEDS_BASE_ADDR             0x40010000
#define BUTTONS_BASE_ADDR          0x40000000
#define AXI_GPIO_2_BASE_ADDR       0x40020000
#define XPAR_PMOD_DHB1_0_GPIO_BASEADDR 0x44A10000
#define XPAR_PMOD_DHB1_0_PWM_BASEADDR  0x44A20000
#define PMOD_DHB1_CLOCK_FREQ_HZ    XPAR_CPU_CORE_CLOCK_FREQ_HZ
#define PMOD_SONAR0_BASEADDR       0x44A30000
#define RGB_WHITE 0xFFFF
#define RGB_GREEN 02222

// PWM Settings
#define PWM_PERIOD 0x00029000  // 2ms

// Light Sensor Masks 
#define L_SENSOR 0x1
#define R_SENSOR 0x2

PmodDHB1 motor;
PMOD_DUAL_MAXSONAR Sonar;

static void delay_ms(int ms) {
    for (int i = 0; i < 1500 * ms; i++) asm("nop");
}

int main(void) {
    xil_printf("System Booting...\r\n");

    // Initialize LEDs and buttons
    XGpio_WriteReg(LEDS_BASE_ADDR, XGPIO_TRI_OFFSET, 0x0);             // LEDs as output
    XGpio_WriteReg(BUTTONS_BASE_ADDR, XGPIO_TRI2_OFFSET, 0xF);         // Buttons as input

    // Initialize DHB1 motor driver
    DHB1_begin(&motor,
        XPAR_PMOD_DHB1_0_GPIO_BASEADDR,
        XPAR_PMOD_DHB1_0_PWM_BASEADDR,
        PMOD_DHB1_CLOCK_FREQ_HZ,
        PWM_PERIOD * 3);
    DHB1_setDirs(&motor, 0x1, 0x1); // Forward
    DHB1_motorEnable(&motor);
    DHB1_setMotorSpeeds(&motor, 0, 0);

    // Initialize sonar 
    MAXSONAR_begin(&Sonar, PMOD_SONAR0_BASEADDR, PMOD_DHB1_CLOCK_FREQ_HZ);

    // Configure light sensors
    volatile u32 *SensorData = (u32 *)(AXI_GPIO_2_BASE_ADDR + XGPIO_DATA_OFFSET);
    volatile u32 *SensorTristateReg = (u32 *)(AXI_GPIO_2_BASE_ADDR + XGPIO_TRI_OFFSET);
    *SensorTristateReg = 0xF;

    // Wait for button press to start
    while (1) {
        uint32_t btn = XGpio_ReadReg(BUTTONS_BASE_ADDR, XGPIO_DATA2_OFFSET) & 0xF;
        if (btn) break;
        XGpio_WriteReg(LEDS_BASE_ADDR, XGPIO_DATA_OFFSET, RGB_WHITE);
        delay_ms(50);
    }

    xil_printf("Starting motor control loop...\r\n");

    while (1) {
        XGpio_WriteReg(LEDS_BASE_ADDR, XGPIO_DATA_OFFSET, RGB_GREEN);

        // Read distance from sonar
        u32 distance = MAXSONAR_getDistance(&Sonar, 2);
        xil_printf("Sonar Distance: %3d cm\r\n", distance);

        //  stop if obstacle close
        if (distance > 0 && distance < 7) {
            xil_printf("Obstacle detected!\r\n");
            DHB1_setMotorSpeeds(&motor, 0, 0);
            delay_ms(200);
            return 0;
        }

        // Light sensor logic
        u32 sensor = *SensorData;

        if (sensor & L_SENSOR) {
            xil_printf("Left sensor\r\n");
            DHB1_setMotorSpeeds(&motor, 80, 0);
            delay_ms(5);
        }
        else if (sensor & R_SENSOR) {
            xil_printf("Right sensor\r\n");
            DHB1_setMotorSpeeds(&motor, 0, 80);
            delay_ms(5);
        }
        else {
            xil_printf("DRIVE STRAIGHT\r\n");
            DHB1_setMotorSpeeds(&motor, 30, 30);
        }

        xil_printf("Sensor Raw: 0x%08x\r\n", sensor);
        delay_ms(2);
    }

    return 0;
}
