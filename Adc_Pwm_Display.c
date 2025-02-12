#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "lib/ssd1306.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define DISPLAY_ADDR 0x3C

#define JOYSTICK_X 26
#define JOYSTICK_Y 27
#define JOYSTICK_BTN 22
#define BUTTON_A 5

#define LED_GREEN 11
#define LED_BLUE 12
#define LED_RED 13

ssd1306_t ssd;
bool pwm_enabled = true;
bool border_toggle = false;

void setup_pwm(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin);
    pwm_set_wrap(slice, 4095);
    pwm_set_enabled(slice, true);
    pwm_set_gpio_level(pin, 0); // Garante que os LEDs iniciam apagados
}

void debounce_delay() {
    sleep_ms(50);
}

void joystick_irq_handler(uint gpio, uint32_t events) {
    debounce_delay();
    static bool led_state = false;
    if (gpio == JOYSTICK_BTN) {
        led_state = !led_state;
        gpio_put(LED_GREEN, led_state);
        border_toggle = !border_toggle;
    }
}

void button_a_irq_handler(uint gpio, uint32_t events) {
    debounce_delay();
    if (gpio == BUTTON_A) {
        pwm_enabled = !pwm_enabled;
    }
}

int main() {
    stdio_init_all();
    
    // Inicializa ADC
    adc_init();
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);
    
    // Configura PWM para LEDs
    setup_pwm(LED_RED);
    setup_pwm(LED_BLUE);
    
    // Inicializa Display
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, DISPLAY_ADDR, I2C_PORT);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
    
    // Configura botões com interrupção
    gpio_init(JOYSTICK_BTN);
    gpio_set_dir(JOYSTICK_BTN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BTN);
    gpio_set_irq_enabled_with_callback(JOYSTICK_BTN, GPIO_IRQ_EDGE_FALL, true, &joystick_irq_handler);
    
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &button_a_irq_handler);
    
    while (true) {
        adc_select_input(0);
        uint16_t x_val = adc_read();
        adc_select_input(1);
        uint16_t y_val = adc_read();
        
        if (pwm_enabled) {
            // Calcula brilho do LED Azul com base no eixo X
            uint16_t blue_brightness = (x_val > 2048) ? (x_val - 2048) * 2 : (2048 - x_val) * 2;
            pwm_set_gpio_level(LED_BLUE, blue_brightness);
            
            // Calcula brilho do LED Vermelho com base no eixo Y
            uint16_t red_brightness = (y_val > 2048) ? (y_val - 2048) * 2 : (2048 - y_val) * 2;
            pwm_set_gpio_level(LED_RED, red_brightness);
        } else {
            pwm_set_gpio_level(LED_BLUE, 0);
            pwm_set_gpio_level(LED_RED, 0);
        }
        
        ssd1306_fill(&ssd, false);
        if (border_toggle) {
            ssd1306_rect(&ssd, 0, 0, WIDTH, HEIGHT, true, false);
        }
        ssd1306_rect(&ssd, (x_val * (WIDTH - 8)) / 4095, (y_val * (HEIGHT - 8)) / 4095, 8, 8, true, true);
        ssd1306_send_data(&ssd);
        
        sleep_ms(50);
    }
}
