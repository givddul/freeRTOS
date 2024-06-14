#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include <math.h>
#include "hardware/gpio.h"
#include "pico/time.h"
#include <time.h>

#define LED_PIN 22
#define BUZZER_PIN 21
#define BUTTON_PIN 20

const uint DHT_PIN = 15;
const uint MAX_TIMINGS = 85;
bool button_pressed = false;
bool alarm_triggered = false;

typedef struct {
    float humidity;
    float temp_celsius;
} dht_reading;

typedef struct {
    uint32_t button_press_event;
    absolute_time_t press_time;
} button_event;

QueueHandle_t xQueue;

void read_from_dht(dht_reading *result);
void sensor_task(void *pvParameters);
void alarm_task(void *pvParameters);
void button_isr(uint gpio, uint32_t events);
void intensive_task1(void *pvParameters);
void intensive_task2(void *pvParameters);


int main() {
    stdio_init_all();

    gpio_init(DHT_PIN);
#ifdef LED_PIN
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
#endif
#ifdef BUZZER_PIN
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
#endif
#ifdef BUTTON_PIN
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &button_isr);
#endif
    xQueue = xQueueCreate(10, sizeof(dht_reading));

    if (xQueue != NULL) {
        xTaskCreate(sensor_task, "Sensor Task", 1024, NULL, 2, NULL);
        xTaskCreate(alarm_task, "Alarm Task", 1024, NULL, 2, NULL);
        xTaskCreate(intensive_task1, "Factorial Task", 1024, NULL, 1, NULL); 
        xTaskCreate(intensive_task2, "Intensive Task 2", 1024, NULL, 1, NULL); 

        vTaskStartScheduler();
    } else {
        printf("Queue could not be created.\n");
    }

    while (1) { }

    return 0;
}


void sensor_task(void *pvParameters) {
    dht_reading reading;
    while (1) {
        absolute_time_t start_time = get_absolute_time(); // start time
        printf("    sensor_task running...");
        read_from_dht(&reading);
        if (xQueueSend(xQueue, &reading, (TickType_t)10) != pdPASS) {
            printf("    Failed to post the message.\n");
        }

        float fahrenheit = (reading.temp_celsius * 9 / 5) + 32;
        printf(" Humidity = %.1f%%, Temperature = %.1fC,",
               reading.humidity, reading.temp_celsius, fahrenheit);

        absolute_time_t end_time = get_absolute_time(); // end time
        int64_t time_taken = absolute_time_diff_us(start_time, end_time) / 1000; // time difference in milliseconds
        printf("   sensor_task finishing in %lld milliseconds\n", time_taken);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}



void alarm_task(void *pvParameters) {
    dht_reading received_reading;
    while (1) {
        absolute_time_t start_time = get_absolute_time(); // start time
        printf("        alarm_task running\n");
        if (xQueueReceive(xQueue, &received_reading, portMAX_DELAY)) {
            if (!button_pressed && received_reading.humidity > 90.0 && !alarm_triggered) {
                alarm_triggered = true; // alarm is triggered
#ifdef LED_PIN
                gpio_put(LED_PIN, 1);
#endif
#ifdef BUZZER_PIN
                gpio_put(BUZZER_PIN, 1);
#endif
            } else if (button_pressed || received_reading.humidity <= 90.0) {
                alarm_triggered = false; // reset alarm state
#ifdef LED_PIN
                gpio_put(LED_PIN, 0);
#endif
#ifdef BUZZER_PIN
                gpio_put(BUZZER_PIN, 0);
#endif
            }
        }
        absolute_time_t end_time = get_absolute_time(); // end time
        int64_t time_taken = absolute_time_diff_us(start_time, end_time) / 1000; // time difference in milliseconds
        printf("        Alarm task finishing in %lld milliseconds\n", time_taken);
    }
}


//  Interrupt Service Routine (ISR) for button press
void button_isr(uint gpio, uint32_t events) {
    absolute_time_t press_time = get_absolute_time();  // Get the press time
    button_pressed = true;  // Button has been pressed

#ifdef LED_PIN
    gpio_put(LED_PIN, 0);  // Turn off the LED
#endif

#ifdef BUZZER_PIN
    gpio_put(BUZZER_PIN, 0);  // Turn off the buzzer
#endif

    absolute_time_t task_end_time = get_absolute_time();  // Get the time after the operations
    int64_t reaction_time = absolute_time_diff_us(press_time, task_end_time);  // Calculate reaction time

    printf("            Reacting to button and resetting led&buzzer in %lld milliseconds\n", reaction_time);  // Print the reaction time

    button_pressed = false;  // Reset the button state
}


void read_from_dht(dht_reading *result) {
    int data[5] = {0, 0, 0, 0, 0};
    uint last = 1;
    uint j = 0;

    gpio_set_dir(DHT_PIN, GPIO_OUT);
    gpio_put(DHT_PIN, 0);
    sleep_ms(18);
    gpio_set_dir(DHT_PIN, GPIO_IN);
    sleep_us(40);

    for (uint i = 0; i < MAX_TIMINGS; i++) {
        uint count = 0;
        while (gpio_get(DHT_PIN) == last) {
            count++;
            sleep_us(1);
            if (count == 255)
                break;
        }
        last = gpio_get(DHT_PIN);
        if (count == 255)
            break;

        if ((i >= 4) && (i % 2 == 0)) {
            data[j / 8] <<= 1;
            if (count > 46)
                data[j / 8] |= 1;
            j++;
        }
    }

    if ((j >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
        result->humidity = (float)((data[0] << 8) + data[1]) / 10;
        if (result->humidity > 100) {
            result->humidity = data[0];
        }
        result->temp_celsius = (float)(((data[2] & 0x7F) << 8) + data[3]) / 10;
        if (result->temp_celsius > 125) {
            result->temp_celsius = data[2];
        }
        if (data[2] & 0x80) {
            result->temp_celsius = -result->temp_celsius;
        }
    } else {
        
    }
}


void intensive_task1(void *pvParameters) {
    int outer_loop = 7000;
    int inner_loop = 7000;
    volatile int sink = 0;

    for (;;) {
        printf("                intensive_task_1 started\n");

        absolute_time_t start_time = get_absolute_time(); // start time

        for (int i = 0; i < outer_loop; i++) {
            for (int j = 0; j < inner_loop; j++) {
                sink++;
            }
        }

        absolute_time_t end_time = get_absolute_time(); // end time
        int64_t time_taken = absolute_time_diff_us(start_time, end_time) / 1000; // time difference in milliseconds

        printf("                intensive_task_1 finished. Time taken: %lld milliseconds\n", time_taken);
    }
    vTaskDelay(pdMS_TO_TICKS(10));

}


void intensive_task2(void *pvParameters) {
    int outer_loop = 7000;
    int inner_loop = 7000;
    volatile int sink = 0;

    for (;;) {
        printf("                      intensive_task_2 started\n");

        absolute_time_t start_time = get_absolute_time(); // start time

        for (int i = 0; i < outer_loop; i++) {
            for (int j = 0; j < inner_loop; j++) {
                sink++;
            }
        }

        absolute_time_t end_time = get_absolute_time(); // end time
        int64_t time_taken = absolute_time_diff_us(start_time, end_time) / 1000; // time difference in milliseconds

        printf("                      intensive_task_2 finished. Time taken: %lld milliseconds\n", time_taken);
    }
    vTaskDelay(pdMS_TO_TICKS(10));
}
