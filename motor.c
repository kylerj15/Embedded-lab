#include "FreeRTOS.h"
#include "task.h"
#include "xgpio_l.h"
#include "xil_printf.h"
#include "semphr.h"


#define LEDS_BASE_ADDR     0x40010000  // RGB LED GPIO
#define BUTTONS_BASE_ADDR  0x40000000  // Buttons GPIO
#define MOTOR_GPIO_ADDR    0x40020000   // Motor GPIO 

#define RGB_WHITE 0xFFFF
#define RGB_GREEN 02222
#define MOTOR_ENABLE  (1 << 0)  // EN1
#define MOTOR_DIR     (1 << 1)  // DIR1
#define MOTOR_SLEEP   (1 << 2)  // NSLEEP

#define MOTOR_ON  (MOTOR_ENABLE | MOTOR_DIR | MOTOR_SLEEP)
#define MOTOR_OFF 0x00  // All low

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
        XGpio_WriteReg(MOTOR_GPIO_ADDR, XGPIO_DATA_OFFSET, MOTOR_ON);

        uint32_t btn = XGpio_ReadReg(BUTTONS_BASE_ADDR, XGPIO_DATA2_OFFSET) & 0xF;
        if (btn)
        {
            if (xSemaphoreTake(state_mutex, 10))
            {
                xil_printf("Button pressed, switching to RUN\r\n");
                programOn = 1;
                state = RUN;
                xSemaphoreGive(state_mutex);
                vTaskResume(SupervisorTaskHandle);
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

        // Future: Add sensor check here

        vTaskSuspend(NULL);
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

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

int main(void)
{
    xil_printf("System Booting...\r\n");

    XGpio_WriteReg(LEDS_BASE_ADDR, XGPIO_TRI_OFFSET, 0x0);           // LED out
    XGpio_WriteReg(BUTTONS_BASE_ADDR, XGPIO_TRI2_OFFSET, 0xF);       // Buttons in
    XGpio_WriteReg(MOTOR_GPIO_ADDR, XGPIO_TRI_OFFSET, 0x0);          // Motor control out

    state_mutex = xSemaphoreCreateMutex();
    if (!state_mutex)
    {
        xil_printf("Mutex creation failed.\r\n");
        return -1;
    }

    xTaskCreate(SupervisorTask, "Supervisor", configMINIMAL_STACK_SIZE, NULL, 1, &SupervisorTaskHandle);
    xTaskCreate(WhiteTask, "White", configMINIMAL_STACK_SIZE, NULL, 1, &WhiteTaskHandle);
    xTaskCreate(RunTask, "Run", configMINIMAL_STACK_SIZE, NULL, 1, &RunTaskHandle);

    vTaskSuspend(RunTaskHandle);

    xil_printf("FreeRTOS Scheduler Starting...\r\n");
    vTaskStartScheduler();

    xil_printf("Scheduler stopped unexpectedly.\r\n");
    return 0;
}
