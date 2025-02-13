/*
 * Projeto: Controle de Joystick com Display OLED e LEDs PWM
 * Autor: Lucas Dias da Silva
 * Descrição:
 *   - Controle de display OLED 128x64 com joystick analógico
 *   - LEDs PWM controlados pelo joystick
 *   - Botões para controle de borda e modo BOOTSEL
 *   - Compatível com Wokwi e hardware físico
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "lib/ssd1306.h"
#include "pico/bootrom.h"

// Definições de hardware
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define DISPLAY_ADDR 0x3C

#define JOYSTICK_X 26
#define JOYSTICK_Y 27
#define JOYSTICK_BTN 22
#define BUTTON_A 5
#define BUTTON_B 6

#define LED_GREEN 11
#define LED_BLUE 12
#define LED_RED 13

#define WIDTH 128
#define HEIGHT 64
#define JOYSTICK_CENTER_X 1929 // Ajustado para o valor correto do centro do joystick
#define JOYSTICK_CENTER_Y 2019 // Ajustado para o valor correto do centro do joystick
#define MARGEM_ERRO 50 // Margem de erro para considerar o centro

ssd1306_t ssd;
bool pwm_enabled = true;
int border_style = 1; // Começa com borda fina

void setup_pwm(uint pin)
{
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin);
    pwm_set_wrap(slice, 4095);
    pwm_set_enabled(slice, true);
    pwm_set_gpio_level(pin, 0);
}

void debounce_delay()
{
    sleep_ms(50);
}

int main()
{
    stdio_init_all();

    // Configuração do hardware
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    adc_init();
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);

    setup_pwm(LED_RED);
    setup_pwm(LED_BLUE);

    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicialização do display OLED
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, DISPLAY_ADDR, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    gpio_init(JOYSTICK_BTN);
    gpio_set_dir(JOYSTICK_BTN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BTN);

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_put(LED_GREEN, false);

    bool last_joystick_btn_state = gpio_get(JOYSTICK_BTN);
    bool last_button_a_state = gpio_get(BUTTON_A);

    int last_x_val = -1;
    int last_y_val = -1;

    // Posição inicial exatamente no centro da tela
    uint8_t x_pos = WIDTH / 2 - 4;
    uint8_t y_pos = HEIGHT / 2 - 4;

    while (true)
    {
        // Controle do modo BOOTSEL
        if (!gpio_get(BUTTON_B))
        {
            printf("[SISTEMA] Entrando em modo BOOTSEL\n");
            reset_usb_boot(0, 0);
        }

        // Leitura dos botões
        bool current_joystick_btn_state = gpio_get(JOYSTICK_BTN);
        if (!current_joystick_btn_state && last_joystick_btn_state)
        {
            debounce_delay();
            gpio_put(LED_GREEN, !gpio_get(LED_GREEN));
            border_style = (border_style == 1) ? 2 : 1; // Alterna entre fina (1) e grossa (2)
            printf("[BOTÃO] Bordas: %d\n", border_style);
        }
        last_joystick_btn_state = current_joystick_btn_state;

        bool current_button_a_state = gpio_get(BUTTON_A);
        if (!current_button_a_state && last_button_a_state)
        {
            debounce_delay();
            pwm_enabled = !pwm_enabled;
            printf("[PWM] Estado: %s\n", pwm_enabled ? "Ativado" : "Desativado");
        }
        last_button_a_state = current_button_a_state;

        // Leitura do joystick
        adc_select_input(0);
        uint16_t x_val = adc_read();
        adc_select_input(1);
        uint16_t y_val = adc_read();

        // Processamento dos valores
        int adjusted_x = x_val - JOYSTICK_CENTER_X;
        int adjusted_y = y_val - JOYSTICK_CENTER_Y;

        // Ajuste no cálculo da posição para centralizar corretamente
        x_pos = (WIDTH / 2) + (adjusted_x * (WIDTH / 2) / 2048) - 4;
        y_pos = (HEIGHT / 2) + (adjusted_y * (HEIGHT / 2) / 2048) - 4;

        // Garante que o cursor não saia da tela
        if (x_pos < 0)
            x_pos = 0;
        if (x_pos > WIDTH - 8)
            x_pos = WIDTH - 8;
        if (y_pos < 0)
            y_pos = 0;
        if (y_pos > HEIGHT - 8)
            y_pos = HEIGHT - 8;

        // Controle dos LEDs
        if (pwm_enabled)
        {
            if (abs(adjusted_x) < MARGEM_ERRO && abs(adjusted_y) < MARGEM_ERRO)
            {
                pwm_set_gpio_level(LED_BLUE, 0);
                pwm_set_gpio_level(LED_RED, 0);
            }
            else
            {
                pwm_set_gpio_level(LED_BLUE, abs(adjusted_x) * 2);
                pwm_set_gpio_level(LED_RED, abs(adjusted_y) * 2);
            }
        }
        else
        {
            pwm_set_gpio_level(LED_BLUE, 0);
            pwm_set_gpio_level(LED_RED, 0);
        }

        // Atualização do display
        ssd1306_fill(&ssd, false);

        // Desenha a borda de acordo com o estilo atual
        if (border_style == 1)
        {
            ssd1306_rect(&ssd, 0, 0, WIDTH, HEIGHT, true, false); // Borda fina
        }
        else if (border_style == 2)
        {
            ssd1306_rect(&ssd, 0, 0, WIDTH, HEIGHT, true, false); // Borda grossa
            ssd1306_rect(&ssd, 1, 1, WIDTH - 2, HEIGHT - 2, true, false);
            ssd1306_rect(&ssd, 2, 2, WIDTH - 4, HEIGHT - 4, true, false); // Borda mais grossa
        }

        // Desenho do cursor
        ssd1306_rect(&ssd, x_pos, y_pos, 8, 8, true, true);
        ssd1306_send_data(&ssd);

        // Debug serial
        if (x_val != last_x_val || y_val != last_y_val)
        {
            printf("[JOYSTICK] X: %4d | Y: %4d | Pos: (%3d, %3d)\n",
                   x_val, y_val, x_pos, y_pos);
            last_x_val = x_val;
            last_y_val = y_val;
        }

        sleep_ms(20);
    }
}
