#include <stdio.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <pico/stdlib.h>

#define TASK_PRIORITY   (tskIDLE_PRIORITY + 1UL)
#define TASK_STACK_SIZE configMINIMAL_STACK_SIZE

SemaphoreHandle_t lock;

int counter = 0;

/*
 * Activity 5:
 * This function intentionally contains an orphaned-lock bug.
 * If skip_update is true, continue skips xSemaphoreGive().
 */
void worker_task(void *params)
{
    while (1) {
        xSemaphoreTake(lock, portMAX_DELAY);

        if (counter == 3) {
            printf("Skipping update at counter %d\n", counter);

            /*
             * BUG:
             * The lock is not released before continue.
             * The next iteration will wait forever for the same lock.
             */
            continue;
        }

        counter++;

        printf("Counter = %d\n", counter);

        xSemaphoreGive(lock);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int main(void)
{
    stdio_init_all();

    lock = xSemaphoreCreateMutex();

    if (lock == NULL) {
        printf("Failed to create mutex\n");
        return 1;
    }

    TaskHandle_t worker_handle;

    xTaskCreate(
        worker_task,
        "Worker",
        TASK_STACK_SIZE,
        NULL,
        TASK_PRIORITY,
        &worker_handle
    );

    vTaskStartScheduler();

    return 0;
}