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

#define JOYSTICK_X 26 // GPIO para eixo X
#define JOYSTICK_Y 27 // GPIO para eixo Y
#define JOYSTICK_BTN 22 // GPIO para botão do Joystick
#define BUTTON_A 5

#define LED_GREEN 11
#define LED_BLUE 12
#define LED_RED 13

#define JOYSTICK_CENTER_X 1902
#define JOYSTICK_CENTER_Y 1972

//Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botaoB 6

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

int main() {
    stdio_init_all();
    
    // Para ser utilizado o modo BOOTSEL com botão B
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    
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
    
    // Configura botões
    gpio_init(JOYSTICK_BTN);
    gpio_set_dir(JOYSTICK_BTN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BTN);
    
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    
    // Configura LED Verde
    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_put(LED_GREEN, false); // Garante que o LED Verde inicia apagado
    
    bool last_joystick_btn_state = gpio_get(JOYSTICK_BTN);
    bool last_button_a_state = gpio_get(BUTTON_A);
    
    while (true) {
        // Verifica estado do botão B para modo BOOTSEL
        if (!gpio_get(botaoB)) {
            printf("Botão B pressionado\n");
            reset_usb_boot(0, 0);
        }
        
        // Verifica estado do botão do joystick
        bool current_joystick_btn_state = gpio_get(JOYSTICK_BTN);
        if (current_joystick_btn_state == false && last_joystick_btn_state == true) {
            debounce_delay();
            static bool led_state = false;
            led_state = !led_state;
            gpio_put(LED_GREEN, led_state);
            border_toggle = !border_toggle;
            printf("Joystick pressionado, LED Verde: %d, border_toggle: %d\n", led_state, border_toggle);
        }
        last_joystick_btn_state = current_joystick_btn_state;
        
        // Verifica estado do botão A
        bool current_button_a_state = gpio_get(BUTTON_A);
        if (current_button_a_state == false && last_button_a_state == true) {
            debounce_delay();
            pwm_enabled = !pwm_enabled;
            printf("Botão A pressionado, PWM habilitado: %d\n", pwm_enabled);
        }
        last_button_a_state = current_button_a_state;
        
        // Leitura do joystick
        adc_select_input(0);
        uint16_t x_val = adc_read();
        adc_select_input(1);
        uint16_t y_val = adc_read();
        
        // Ajusta os valores para o novo centro
        int adjusted_x = x_val - JOYSTICK_CENTER_X;
        int adjusted_y = y_val - JOYSTICK_CENTER_Y;
        adjusted_x = adjusted_x < -1902 ? -1902 : (adjusted_x > 2193 ? 2193 : adjusted_x);
        adjusted_y = adjusted_y < -1972 ? -1972 : (adjusted_y > 2123 ? 2123 : adjusted_y);
        
        if (pwm_enabled) {
            // Calcula brilho do LED Azul com base no eixo X
            uint16_t blue_brightness = abs(adjusted_x) * 2;
            pwm_set_gpio_level(LED_BLUE, blue_brightness);
            
            // Calcula brilho do LED Vermelho com base no eixo Y
            uint16_t red_brightness = abs(adjusted_y) * 2;
            pwm_set_gpio_level(LED_RED, red_brightness);
        } else {
            pwm_set_gpio_level(LED_BLUE, 0);
            pwm_set_gpio_level(LED_RED, 0);
        }
        
        // Mapeia joystick para a tela corretamente
        uint8_t x_pos = ((adjusted_x + 1902) * (WIDTH - 8)) / 4095;
        uint8_t y_pos = ((adjusted_y + 1972) * (HEIGHT - 8)) / 4095;
        
        ssd1306_fill(&ssd, false);
        if (border_toggle) {
            ssd1306_rect(&ssd, 0, 0, WIDTH, HEIGHT, true, false);
        }
        ssd1306_rect(&ssd, x_pos, y_pos, 8, 8, true, true);
        ssd1306_send_data(&ssd);
        
        printf("x_val: %d, y_val: %d, adjusted_x: %d, adjusted_y: %d, x_pos: %d, y_pos: %d\n", x_val, y_val, adjusted_x, adjusted_y, x_pos, y_pos);
        
        sleep_ms(50);
    }
}
