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

#define JOYSTICK_X 27
#define JOYSTICK_Y 26
#define JOYSTICK_BTN 22
#define BUTTON_A 5
#define BUTTON_B 6

#define LED_GREEN 11
#define LED_BLUE 12
#define LED_RED 13

#define WIDTH 128
#define HEIGHT 64
#define JOYSTICK_CENTER_X 1929
#define JOYSTICK_CENTER_Y 2019
#define DEADZONE 100 // Zona morta para evitar tremores

ssd1306_t ssd;
bool pwm_enabled = true;
int border_style = 1;

// Função para configurar o PWM em um pino específico
void setup_pwm(uint pin)
{
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin);
    pwm_set_wrap(slice, 4095); // Configura o limite do PWM para 4095
    pwm_set_enabled(slice, true);
    pwm_set_gpio_level(pin, 0); // Inicializa o PWM com 0 (sem brilho)
}

// Função para adicionar um pequeno atraso para debounce
void debounce_delay()
{
    sleep_ms(50);
}

int main()
{
    stdio_init_all();

    // Configuração dos botões
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    adc_init();
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);

    // Configuração dos LEDs PWM
    setup_pwm(LED_RED);
    setup_pwm(LED_BLUE);
    setup_pwm(LED_GREEN); // Adiciona o PWM para o LED verde

    // Configuração do I2C para o display OLED
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

    // Configuração do botão do joystick
    gpio_init(JOYSTICK_BTN);
    gpio_set_dir(JOYSTICK_BTN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BTN);

    // Configuração do botão A
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    bool last_joystick_btn_state = gpio_get(JOYSTICK_BTN);
    bool last_button_a_state = gpio_get(BUTTON_A);

    // Posição inicial ajustada para (28, 56)
    int x_pos = 59;
    int y_pos = 29;

    while (true)
    {
        // Ajusta a intensidade do PWM dos LEDs com base no movimento do joystick
        int pwm_red = 0;
        int pwm_blue = 0;

        // Verifica se o botão B foi pressionado para entrar em modo BOOTSEL
        if (!gpio_get(BUTTON_B))
        {
            printf("[SISTEMA] Entrando em modo BOOTSEL\n");
            reset_usb_boot(0, 0);
        }

        // Verifica o estado do botão do joystick
        bool current_joystick_btn_state = gpio_get(JOYSTICK_BTN);
        if (!current_joystick_btn_state && last_joystick_btn_state)
        {
            debounce_delay();
            static bool green_led_state = false;
            green_led_state = !green_led_state;
            pwm_set_gpio_level(LED_GREEN, green_led_state ? 2047 : 0);
            border_style = (border_style == 1) ? 2 : 1;
            printf("[BOTÃO] Bordas: %d\n", border_style);
        }
        last_joystick_btn_state = current_joystick_btn_state;

        // Verifica o estado do botão A
        bool current_button_a_state = gpio_get(BUTTON_A);
        if (!current_button_a_state && last_button_a_state)
        {
            debounce_delay();
            pwm_enabled = !pwm_enabled;
            printf("[PWM] Estado: %s\n", pwm_enabled ? "Ativado" : "Desativado");
        }
        last_button_a_state = current_button_a_state;

        // Leitura dos valores do joystick
        adc_select_input(0);
        uint16_t x_val = adc_read();
        adc_select_input(1);
        uint16_t y_val = adc_read();

        int adjusted_x = x_val - JOYSTICK_CENTER_X; // Movimentos no eixo X (direita/esquerda)
        int adjusted_y = y_val - JOYSTICK_CENTER_Y; // Movimentos no eixo Y (cima/baixo)

        // Movimentos corretos para o eixo X (cima/baixo) - Trocar com Y
        if (abs(adjusted_y) > DEADZONE)
        {
            x_pos += (adjusted_y * 5) / 2048; // Ajuste proporcional ao valor do joystick
        }

        // Calcula intensidade para o LED Vermelho (eixo Y)
        if (abs(adjusted_y) > DEADZONE)
        {
            int intensidade = abs(adjusted_y) - DEADZONE;
            pwm_red = (intensidade * 4095) / (4095 - JOYSTICK_CENTER_X - DEADZONE);
        }

        // Calcula intensidade para o LED Azul (eixo X)
        if (abs(adjusted_x) > DEADZONE)
        {
            int intensidade = abs(adjusted_x) - DEADZONE;
            pwm_blue = (intensidade * 4095) / (4095 - JOYSTICK_CENTER_Y - DEADZONE);
        }

        // Movimentos corretos para o eixo Y (direita/esquerda) - Trocar com X
        if (abs(adjusted_x) > DEADZONE)
        {
            y_pos -= (adjusted_x * 5) / 2048; // Ajuste proporcional ao valor do joystick
        }

        // Limitações da tela para não ultrapassar os limites
        if (x_pos < 0)
            x_pos = 0;
        if (x_pos > WIDTH - 8)
            x_pos = WIDTH - 8;
        if (y_pos < 0)
            y_pos = 0;
        if (y_pos > HEIGHT - 8)
            y_pos = HEIGHT - 8;

        // Inverte os valores para ajustar a posição
        int inverted_x_pos = y_pos; // Agora y_pos vai para o eixo X
        int inverted_y_pos = x_pos; // Agora x_pos vai para o eixo Y

        // Limpa a tela
        ssd1306_fill(&ssd, false);

        // Desenha as bordas
        if (border_style == 1)
        {
            ssd1306_rect(&ssd, 0, 0, WIDTH, HEIGHT, true, false); // Manter o tamanho da borda original
        }
        else if (border_style == 2)
        {
            ssd1306_rect(&ssd, 0, 0, WIDTH, HEIGHT, true, false); // Manter o tamanho da borda original
            ssd1306_rect(&ssd, 1, 1, WIDTH - 2, HEIGHT - 2, true, false);
            ssd1306_rect(&ssd, 2, 2, WIDTH - 4, HEIGHT - 4, true, false);
        }

        // Desenha o quadrado com as coordenadas invertidas
        ssd1306_rect(&ssd, inverted_x_pos, inverted_y_pos, 8, 8, true, true);
        ssd1306_send_data(&ssd);

        printf("[JOYSTICK] X: %4d | Y: %4d | Pos: (%3d, %3d)\n", x_val, y_val, inverted_x_pos, inverted_y_pos);

        // Ajusta a intensidade do PWM dos LEDs com base na posição do joystick
        if (pwm_enabled)
        {
            pwm_set_gpio_level(LED_RED, pwm_red);
            pwm_set_gpio_level(LED_BLUE, pwm_blue);
        }
        else
        {
            pwm_set_gpio_level(LED_RED, 0);
            pwm_set_gpio_level(LED_BLUE, 0);
        }

        // Pequeno atraso para evitar leituras muito rápidas
        sleep_ms(20);
    }
}
