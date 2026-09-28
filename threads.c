#include <stdio.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/cyw43_arch.h>

#define MAIN_TASK_PRIORITY      ( tskIDLE_PRIORITY + 1UL )
#define MAIN_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

#define SIDE_TASK_PRIORITY      ( tskIDLE_PRIORITY + 1UL )
#define SIDE_TASK_STACK_SIZE configMINIMAL_STACK_SIZE


// Edited By:
// Stephen Hofmann
// Abed Mbarushimana

// Date: SEPTEMBER 2026

// variable declarations
SemaphoreHandle_t semaphore;

int counter;
int on;


void side_thread(void *params)
{
	while (1) {
        vTaskDelay(100);
        counter += 1;
		printf("hello world from %s! Count %d\n", "thread", counter);
	}
}

void main_thread(void *params)
{
	while (1) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
        vTaskDelay(100);
		printf("hello world from %s! Count %d\n", "main", counter++);
        on = !on;
	}
}

int main(void)
{
    stdio_init_all();
    hard_assert(cyw43_arch_init() == PICO_OK);
    on = false;
    counter = 0;
    TaskHandle_t main, side;
    semaphore = xSemaphoreCreateCounting(1, 1);                                 // Semaphore definition

    xTaskCreate(main_thread, "MainThread",                                      // Create main thread
                MAIN_TASK_STACK_SIZE, NULL, MAIN_TASK_PRIORITY, &main);

    xTaskCreate(side_thread, "SideThread",                                      // Create second thread
                SIDE_TASK_STACK_SIZE, NULL, SIDE_TASK_PRIORITY, &side);

                
    vTaskStartScheduler();
	return 0;
}
