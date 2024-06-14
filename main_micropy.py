import dht
import time
from machine import Pin
import math
import _thread

DHT_PIN = 15
LED_PIN = 22
BUZZER_PIN = 21
BUTTON_PIN = 20

dht_sensor = dht.DHT11(Pin(DHT_PIN))
led = Pin(LED_PIN, Pin.OUT)
buzzer = Pin(BUZZER_PIN, Pin.OUT)
button = Pin(BUTTON_PIN, Pin.IN, Pin.PULL_DOWN)

button_pressed = False
alarm_triggered = False

def read_from_dht():
    retries = 3
    for i in range(retries):
        try:
            dht_sensor.measure()
            return dht_sensor.humidity(), dht_sensor.temperature()
        except OSError as e:
            if i < retries - 1:  # If not the last retry
                time.sleep(2)  # Wait a bit before retrying
                continue
            else:  # Last retry
                print("Failed to read from DHT sensor: ", e)
                return None, None


def button_isr(pin):
    print("button pressed...")
    global button_pressed
    if not button_pressed:  # button can only be pressed again after flag was reset
        button_pressed = True
        led.value(0)  # Turn off the LED
        buzzer.value(0)  # Turn off the buzzer


button.irq(trigger=Pin.IRQ_RISING, handler=button_isr)

def alarm_task(humidity):
    global alarm_triggered
    print("        alarm_task running...")
    if humidity > 90.0 and not button_pressed and not alarm_triggered:
        alarm_triggered = True
        led.value(1)
        buzzer.value(1)
    elif humidity <= 90.0 or button_pressed:
        alarm_triggered = False
        led.value(0)
        buzzer.value(0)

def intensive_task2():
    while True:
        outer_loop = 1500
        inner_loop = 1100
        sink = 0
        start_time = time.ticks_ms()
        print("                      intensive_task_2 running...")
        for i in range(outer_loop):
            for j in range(inner_loop):
                sink += 1
        end_time = time.ticks_diff(time.ticks_ms(), start_time)
        print("                      intensive_task_2 finished in {} milliseconds <---- time taken not determanistic ".format(end_time))

def intensive_task1():
    outer_loop = 1500
    inner_loop = 1000
    sink = 0
    start_time = time.ticks_ms()
    print("                intensive_task_1 running...")
    for i in range(outer_loop):
        for j in range(inner_loop):
            sink += 1
    end_time = time.ticks_diff(time.ticks_ms(), start_time)
    print("                intensive_task_1 finished in {} milliseconds".format(end_time))

def sensor_task():
    start_time = time.ticks_ms()
    print("    sensor_task running...")
    global humidity, temperature
    humidity, temperature = read_from_dht()
    print("    Humidity = {:.1f}%".format(humidity))
    end_time = time.ticks_diff(time.ticks_ms(), start_time)
    print("    sensor_task finished in {} milliseconds".format(end_time))



def superloop():
    while True:
        print("superloop running...")
        start_time = time.ticks_ms()
        time.sleep(2)  # DHT11 needs a 2-second pause between readings
        sensor_task()
        alarm_task(humidity)
        intensive_task1()
        global button_pressed
        if button_pressed:
            time.sleep(5)  # debounce delay
            button_pressed = False
        end_time = time.ticks_diff(time.ticks_ms(), start_time)
        print("superloop finished in {} milliseconds".format(end_time))
        
intensive_task2  = _thread.start_new_thread(intensive_task2,())
        
# call superloop
superloop()

