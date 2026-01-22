/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"

// Priorities of our threads - higher numbers are higher priority
#define TASK1_PRIORITY            ( tskIDLE_PRIORITY + 1UL )
#define TASK2_PRIORITY            ( tskIDLE_PRIORITY + 2UL )
#define TASK2_HIGHER_PRIORITY      ( tskIDLE_PRIORITY + 5UL )

// Stack sizes of our threads in words (4 bytes)
#define MAIN_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

static void task_1(__unused void *params);
static void task_2(__unused void *params);

static TaskHandle_t task1_handle, task2_handle;
SemaphoreHandle_t xSemaphore;

void task_1(__unused void *params) {

    uint32_t count = 0, core_id = 0xDEADBEE1;
    UBaseType_t task1_pri = tskIDLE_PRIORITY;

    xSemaphore = xSemaphoreCreateMutex();

    if( xSemaphore != NULL )
    {
        // take the mutex, then never give it back.
        if( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE )
        {
            xTaskCreate(task_2, "Thread2_core", MAIN_TASK_STACK_SIZE, NULL, TASK2_PRIORITY, &task2_handle);
            printf("task1 took the mutex and created task2.\n");
        }
    }

    while(true) {

        count++;
        core_id = portGET_CORE_ID();
        task1_pri = uxTaskPriorityGet(task1_handle);

        printf("task1 on core %ld, pri=%d, ct=%ld\n", core_id, task1_pri, count);

        vTaskDelay(1000 / portTICK_PERIOD_MS);

    }
}

void task_2(__unused void *params) {

    uint32_t count = 0, core_id = 0xDEADBEE2;
    UBaseType_t task2_pri = tskIDLE_PRIORITY;

    /* delay 500ms first, so the execution timing of task1 and task2 will interleave with each other. */
    vTaskDelay(500 / portTICK_PERIOD_MS);

    while(true) {

        count++;
        core_id = portGET_CORE_ID();
        task2_pri = uxTaskPriorityGet(task2_handle);

        printf("task2 on core %ld, pri=%d, ct=%ld\n", core_id, task2_pri, count);


#ifdef portREMOVE_STATIC_QUALIFIER
    	extern volatile UBaseType_t uxTopReadyPriority;
    	UBaseType_t TopReadyPriority_temp = tskIDLE_PRIORITY;
    
    	TopReadyPriority_temp = uxTopReadyPriority;
    
    	printf("Ready priorities= 0x%x\n", TopReadyPriority_temp);
    	printf("Highest ready Priority= %d\n", ( 31 - __builtin_clz( (TopReadyPriority_temp) ) ));
#endif
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    printf("Hello FreeRTOS test!\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    printf("===========================\n");
    printf("Start FreeRTOS test\n");
    printf("===========================\n");
    printf("Create task_1.\n");
    xTaskCreate(task_1, "Thread1_core", MAIN_TASK_STACK_SIZE, NULL, TASK1_PRIORITY, &task1_handle);

    // Suspend itself.
    printf("Going to suspend startup main_task itself.\n");
    vTaskSuspend(NULL);

    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
