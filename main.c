#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
/* Xilinx includes. */
#include "xil_printf.h"
#include "xparameters.h"
#include "xgpio.h"
#include "xgpio_l.h"
#include "xil_types.h"

#define LEDS_BASE_ADDR       0x40010000  // RGB LED GPIO
#define BUTTONS_BASE_ADDR    0x40000000  // Button GPIO
#define MOTOR_GPIO_ADDR      0x40020000  // Motor control GPIO
#define SENSOR_GPIO_ADDR     0x40030000  // Sonar sensor (Pmod LS1) GPIO

// LED Colors
#define RGB_WHITE 0xFFFF
#define RGB_GREEN 02222

// Motor Bits
#define MOTOR_ENABLE (1 << 0)
#define MOTOR_DIR    (1 << 1)
#define MOTOR_SLEEP  (1 << 2)
#define MOTOR_ON     (MOTOR_ENABLE | MOTOR_DIR | MOTOR_SLEEP)
#define MOTOR_OFF    0x00

// Sensor Bits
#define L_SENSOR 0x1  // S1
#define R_SENSOR 0x2  // S2

typedef enum {
    WHITE,
    RUN
} State;

State state = WHITE;
int programOn = 0;

SemaphoreHandle_t state_mutex;
TaskHandle_t WhiteTaskHandle = NULL;
TaskHandle_t RunTaskHandle = NULL;
TaskHandle_t SupervisorTaskHandle = NULL;

void WhiteTask(void *arg)
{
    xil_printf("WhiteTask started\r\n");

    while (1)
    {
        XGpio_WriteReg(LEDS_BASE_ADDR, XGPIO_DATA_OFFSET, RGB_WHITE);
        XGpio_WriteReg(MOTOR_GPIO_ADDR, XGPIO_DATA_OFFSET, MOTOR_OFF);

        uint32_t btn = XGpio_ReadReg(BUTTONS_BASE_ADDR, XGPIO_DATA2_OFFSET) & 0xF;
        if (btn)
        {
            if (xSemaphoreTake(state_mutex, 10))
            {
                xil_printf("Button pressed\r\n");
                programOn = 1;
                state = RUN;
                xSemaphoreGive(state_mutex);
                vTaskResume(SupervisorTaskHandle);
                vTaskSuspend(NULL);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void RunTask(void *arg)
{
    xil_printf("RunTask started\r\n");

    while (1)
    {
        XGpio_WriteReg(LEDS_BASE_ADDR, XGPIO_DATA_OFFSET, RGB_GREEN);
        XGpio_WriteReg(MOTOR_GPIO_ADDR, XGPIO_DATA_OFFSET, MOTOR_ON);

        uint32_t sensor = XGpio_ReadReg(SENSOR_GPIO_ADDR, XGPIO_DATA_OFFSET);
        if (sensor & L_SENSOR)
            xil_printf("Sensor: LEFT triggered\r\n");
        if (sensor & R_SENSOR)
            xil_printf("Sensor: RIGHT triggered\r\n");

        vTaskDelay(pdMS_TO_TICKS(500));  // Control loop delay
    }
}

void SupervisorTask(void *arg)
{
    xil_printf("SupervisorTask started\r\n");

    while (1)
    {
        if (xSemaphoreTake(state_mutex, 10))
        {
            switch (state)
            {
            case WHITE:
                vTaskResume(WhiteTaskHandle);
                break;
            case RUN:
                if (programOn)
                    vTaskResume(RunTaskHandle);
                break;
            }
            xSemaphoreGive(state_mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int main(void)
{
    xil_printf("System Booting...\r\n");

    // GPIO directions
    XGpio_WriteReg(LEDS_BASE_ADDR, XGPIO_TRI_OFFSET, 0x0);           // LED output
    XGpio_WriteReg(BUTTONS_BASE_ADDR, XGPIO_TRI2_OFFSET, 0xF);       // Buttons input
    XGpio_WriteReg(MOTOR_GPIO_ADDR, XGPIO_TRI_OFFSET, 0x0);          // Motor output
    XGpio_WriteReg(SENSOR_GPIO_ADDR, XGPIO_TRI_OFFSET, 0xF);         // Sensor input

    // Create Mutex
    state_mutex = xSemaphoreCreateMutex();
    if (!state_mutex)
    {
        xil_printf("Mutex creation failed.\r\n");
        return -1;
    }

    // Tasks
    xTaskCreate(SupervisorTask, "Supervisor", configMINIMAL_STACK_SIZE, NULL, 2, &SupervisorTaskHandle);
    xTaskCreate(WhiteTask, "White", configMINIMAL_STACK_SIZE, NULL, 2, &WhiteTaskHandle);
    xTaskCreate(RunTask, "Run", configMINIMAL_STACK_SIZE, NULL, 2, &RunTaskHandle);

    vTaskSuspend(RunTaskHandle);  // Start in WHITE mode

    xil_printf("FreeRTOS Kernel Starting...\r\n");
    vTaskStartScheduler();

    xil_printf("Scheduler exited unexpectedly.\r\n");
    return 0;
}
