#include <stdio.h>
#include <stdint.h>
#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <unity.h>
#include "unity_config.h"

/*
 * Lab 3 tests
 *
 * Activity 2:
 *   - Test lock behavior.
 *   - Test the counter side effect.
 *
 * Activity 4:
 *   - Create two tasks that deadlock.
 *   - Wait briefly, suspend them, inspect their states,
 *     and delete them.
 *
 * Activity 5:
 *   - Test the functionality.
 *   - Test the orphaned-lock deadlock.
 *   - Test the corrected version.
 */

static SemaphoreHandle_t test_lock;
static int test_counter;


/* =========================================================
 * Activity 2
 * ========================================================= */

static BaseType_t increment_counter(TickType_t timeout)
{
    BaseType_t status = xSemaphoreTake(test_lock, timeout);

    if (status == pdTRUE) {
        test_counter++;
        xSemaphoreGive(test_lock);
    }

    return status;
}


static void test_activity2_lock_and_side_effect(void)
{
    test_counter = 0;
    test_lock = xSemaphoreCreateMutex();

    TEST_ASSERT_NOT_NULL(test_lock);

    /* Lock available: operation should succeed. */
    BaseType_t status = increment_counter(0);

    TEST_ASSERT_EQUAL(pdTRUE, status);
    TEST_ASSERT_EQUAL_INT(1, test_counter);

    /* Hold the lock so increment_counter cannot acquire it. */
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(test_lock, 0));

    status = increment_counter(0);

    TEST_ASSERT_EQUAL(pdFALSE, status);

    /* Counter must not change when lock acquisition fails. */
    TEST_ASSERT_EQUAL_INT(1, test_counter);

    xSemaphoreGive(test_lock);
    vSemaphoreDelete(test_lock);
}


/* =========================================================
 * Activity 4
 * ========================================================= */

typedef struct
{
    SemaphoreHandle_t first_lock;
    SemaphoreHandle_t second_lock;
} DeadlockParameters;

static TaskHandle_t deadlock_task1_handle;
static TaskHandle_t deadlock_task2_handle;

static SemaphoreHandle_t deadlock_lock_a;
static SemaphoreHandle_t deadlock_lock_b;

static void deadlock_task(void *params)
{
    DeadlockParameters *locks = (DeadlockParameters *)params;

    xSemaphoreTake(locks->first_lock, portMAX_DELAY);

    /*
     * Give the other task a chance to acquire its first lock.
     */
    vTaskDelay(pdMS_TO_TICKS(50));

    xSemaphoreTake(locks->second_lock, portMAX_DELAY);

    xSemaphoreGive(locks->second_lock);
    xSemaphoreGive(locks->first_lock);

    vTaskDelete(NULL);
}


static void activity4_test_task(void *params)
{
    (void)params;

    DeadlockParameters task1_params = {
        deadlock_lock_a,
        deadlock_lock_b
    };

    DeadlockParameters task2_params = {
        deadlock_lock_b,
        deadlock_lock_a
    };

    xTaskCreate(
        deadlock_task,
        "Deadlock1",
        configMINIMAL_STACK_SIZE,
        &task1_params,
        tskIDLE_PRIORITY + 1,
        &deadlock_task1_handle
    );

    xTaskCreate(
        deadlock_task,
        "Deadlock2",
        configMINIMAL_STACK_SIZE,
        &task2_params,
        tskIDLE_PRIORITY + 1,
        &deadlock_task2_handle
    );

    /*
     * Wait long enough for both tasks to acquire their first
     * lock and block waiting for their second lock.
     */
    vTaskDelay(pdMS_TO_TICKS(200));

    eTaskState state1_before = eTaskGetState(deadlock_task1_handle);
    eTaskState state2_before = eTaskGetState(deadlock_task2_handle);

    /*
     * Both should be blocked waiting for the other lock.
     */
    TEST_ASSERT_EQUAL(eBlocked, state1_before);
    TEST_ASSERT_EQUAL(eBlocked, state2_before);

    /*
     * Lab requirement: suspend the tasks, check their states,
     * and then delete them.
     */
    vTaskSuspend(deadlock_task1_handle);
    vTaskSuspend(deadlock_task2_handle);

    TEST_ASSERT_EQUAL(
        eSuspended,
        eTaskGetState(deadlock_task1_handle)
    );

    TEST_ASSERT_EQUAL(
        eSuspended,
        eTaskGetState(deadlock_task2_handle)
    );

    vTaskDelete(deadlock_task1_handle);
    vTaskDelete(deadlock_task2_handle);

    vSemaphoreDelete(deadlock_lock_a);
    vSemaphoreDelete(deadlock_lock_b);

    vTaskDelete(NULL);
}


/* =========================================================
 * Activity 5
 * ========================================================= */

/*
 * Functionality separated from the infinite thread loop.
 */
static BaseType_t orphaned_lock_iteration(
    SemaphoreHandle_t semaphore,
    int *counter)
{
    if (xSemaphoreTake(semaphore, 0) != pdTRUE) {
        return pdFALSE;
    }

    (*counter)++;

    /*
     * Intentional bug:
     * odd values return without releasing the semaphore.
     */
    if ((*counter) % 2) {
        return pdTRUE;
    }

    xSemaphoreGive(semaphore);

    return pdTRUE;
}


/*
 * Corrected version.
 * Every successful take has a corresponding give.
 */
static BaseType_t fixed_lock_iteration(
    SemaphoreHandle_t semaphore,
    int *counter)
{
    if (xSemaphoreTake(semaphore, 0) != pdTRUE) {
        return pdFALSE;
    }

    (*counter)++;

    xSemaphoreGive(semaphore);

    return pdTRUE;
}


static void test_activity5_functionality(void)
{
    SemaphoreHandle_t semaphore = xSemaphoreCreateMutex();
    int counter = 0;

    TEST_ASSERT_NOT_NULL(semaphore);

    BaseType_t status =
        orphaned_lock_iteration(semaphore, &counter);

    TEST_ASSERT_EQUAL(pdTRUE, status);

    /* The functionality still increments the counter. */
    TEST_ASSERT_EQUAL_INT(1, counter);

    vSemaphoreDelete(semaphore);
}


static void test_activity5_orphaned_lock_deadlocks(void)
{
    SemaphoreHandle_t semaphore = xSemaphoreCreateMutex();
    int counter = 0;

    TEST_ASSERT_NOT_NULL(semaphore);

    /*
     * First call increments to 1 and accidentally keeps lock.
     */
    TEST_ASSERT_EQUAL(
        pdTRUE,
        orphaned_lock_iteration(semaphore, &counter)
    );

    TEST_ASSERT_EQUAL_INT(1, counter);

    /*
     * Second call cannot acquire the orphaned lock.
     * Timeout is zero, so the test itself cannot hang.
     */
    TEST_ASSERT_EQUAL(
        pdFALSE,
        orphaned_lock_iteration(semaphore, &counter)
    );

    TEST_ASSERT_EQUAL_INT(1, counter);

    vSemaphoreDelete(semaphore);
}


static void test_activity5_fixed_version_does_not_deadlock(void)
{
    SemaphoreHandle_t semaphore = xSemaphoreCreateMutex();
    int counter = 0;

    TEST_ASSERT_NOT_NULL(semaphore);

    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL(
            pdTRUE,
            fixed_lock_iteration(semaphore, &counter)
        );
    }

    TEST_ASSERT_EQUAL_INT(4, counter);

    /*
     * If this succeeds, the corrected function released
     * the semaphore after every iteration.
     */
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(semaphore, 0));
    xSemaphoreGive(semaphore);

    vSemaphoreDelete(semaphore);
}


/* =========================================================
 * Unity setup
 * ========================================================= */

void setUp(void)
{
}

void tearDown(void)
{
}


static void test_runner_task(void *params)
{
    (void)params;

    UNITY_BEGIN();

    RUN_TEST(test_activity2_lock_and_side_effect);

    RUN_TEST(test_activity5_functionality);
    RUN_TEST(test_activity5_orphaned_lock_deadlocks);
    RUN_TEST(test_activity5_fixed_version_does_not_deadlock);

    UNITY_END();

    /*
     * Activity 4 requires real FreeRTOS tasks, so start its
     * test after the normal unit tests.
     */
    deadlock_lock_a = xSemaphoreCreateMutex();
    deadlock_lock_b = xSemaphoreCreateMutex();

    TEST_ASSERT_NOT_NULL(deadlock_lock_a);
    TEST_ASSERT_NOT_NULL(deadlock_lock_b);

    xTaskCreate(
        activity4_test_task,
        "Activity4Test",
        configMINIMAL_STACK_SIZE * 2,
        NULL,
        tskIDLE_PRIORITY + 2,
        NULL
    );

    vTaskDelete(NULL);
}


int main(void)
{
    stdio_init_all();

    sleep_ms(5000);

    printf("Start Lab 3 tests\n");

    xTaskCreate(
        test_runner_task,
        "TestRunner",
        configMINIMAL_STACK_SIZE * 2,
        NULL,
        tskIDLE_PRIORITY + 2,
        NULL
    );

    vTaskStartScheduler();

    while (1) {
    }

    return 0;
}