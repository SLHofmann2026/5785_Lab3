#include <stdio.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>

#define MAIN_TASK_PRIORITY      (tskIDLE_PRIORITY + 1UL)
#define MAIN_TASK_STACK_SIZE    configMINIMAL_STACK_SIZE
#define SIDE_TASK_PRIORITY      (tskIDLE_PRIORITY + 1UL)
#define SIDE_TASK_STACK_SIZE    configMINIMAL_STACK_SIZE

SemaphoreHandle_t semaphore;

int counter;
int on;

/* Functionality separated from the task loop */
BaseType_t increment_counter(TickType_t timeout)
{
    BaseType_t lock_taken = xSemaphoreTake(semaphore, timeout);

    if (lock_taken == pdTRUE) {
        counter++;
        xSemaphoreGive(semaphore);
    }

    return lock_taken;
}

void side_thread(void *params)
{
    while (1) {
        vTaskDelay(100);

        if (increment_counter(pdMS_TO_TICKS(50)) == pdTRUE) {
            printf("hello world from thread! Count %d\n", counter);
        }
    }
}

void main_thread(void *params)
{
    while (1) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
        vTaskDelay(100);

        if (increment_counter(pdMS_TO_TICKS(50)) == pdTRUE) {
            printf("hello world from main! Count %d\n", counter);
        }

        on = !on;
    }
}

int main(void)
{
    stdio_init_all();

    hard_assert(cyw43_arch_init() == PICO_OK);

    on = false;
    counter = 0;

    semaphore = xSemaphoreCreateCounting(1, 1);

    TaskHandle_t main_task_handle;
    TaskHandle_t side_task_handle;

    xTaskCreate(
        main_thread,
        "MainThread",
        MAIN_TASK_STACK_SIZE,
        NULL,
        MAIN_TASK_PRIORITY,
        &main_task_handle
    );

    xTaskCreate(
        side_thread,
        "SideThread",
        SIDE_TASK_STACK_SIZE,
        NULL,
        SIDE_TASK_PRIORITY,
        &side_task_handle
    );

    vTaskStartScheduler();

    return 0;
}