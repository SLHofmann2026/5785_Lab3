#include <stdio.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <pico/stdlib.h>

#define TASK_PRIORITY   (tskIDLE_PRIORITY + 1UL)
#define TASK_STACK_SIZE configMINIMAL_STACK_SIZE

SemaphoreHandle_t lock1;
SemaphoreHandle_t lock2;

TaskHandle_t task1_handle;
TaskHandle_t task2_handle;

/* Task 1 takes lock1 first, then waits for lock2. */
void task1(void *params)
{
    while (1) {
        xSemaphoreTake(lock1, portMAX_DELAY);

        printf("Task 1 acquired lock 1\n");

        vTaskDelay(pdMS_TO_TICKS(100));

        printf("Task 1 waiting for lock 2\n");

        xSemaphoreTake(lock2, portMAX_DELAY);

        printf("Task 1 acquired lock 2\n");

        xSemaphoreGive(lock2);
        xSemaphoreGive(lock1);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* Task 2 takes lock2 first, then waits for lock1. */
void task2(void *params)
{
    while (1) {
        xSemaphoreTake(lock2, portMAX_DELAY);

        printf("Task 2 acquired lock 2\n");

        vTaskDelay(pdMS_TO_TICKS(100));

        printf("Task 2 waiting for lock 1\n");

        xSemaphoreTake(lock1, portMAX_DELAY);

        printf("Task 2 acquired lock 1\n");

        xSemaphoreGive(lock1);
        xSemaphoreGive(lock2);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void deadlock_test(void *params)
{
    /* Give the two tasks time to deadlock. */
    vTaskDelay(pdMS_TO_TICKS(1000));

    /* Suspend both tasks before inspecting them. */
    vTaskSuspend(task1_handle);
    vTaskSuspend(task2_handle);

    eTaskState state1 = eTaskGetState(task1_handle);
    eTaskState state2 = eTaskGetState(task2_handle);

    printf("Task 1 state after suspend: %d\n", state1);
    printf("Task 2 state after suspend: %d\n", state2);
    printf("Deadlock test complete\n");

    /* Delete the deadlocked tasks. */
    vTaskDelete(task1_handle);
    vTaskDelete(task2_handle);

    vTaskDelete(NULL);
}

int main(void)
{
    stdio_init_all();

    lock1 = xSemaphoreCreateMutex();
    lock2 = xSemaphoreCreateMutex();

    xTaskCreate(
        task1,
        "Task1",
        TASK_STACK_SIZE,
        NULL,
        TASK_PRIORITY,
        &task1_handle
    );

    xTaskCreate(
        task2,
        "Task2",
        TASK_STACK_SIZE,
        NULL,
        TASK_PRIORITY,
        &task2_handle
    );

    xTaskCreate(
        deadlock_test,
        "DeadlockTest",
        TASK_STACK_SIZE,
        NULL,
        TASK_PRIORITY + 1,
        NULL
    );

    vTaskStartScheduler();

    return 0;
}