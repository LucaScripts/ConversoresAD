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
 #define JOYSTICK_CENTER_X 1929
 #define JOYSTICK_CENTER_Y 2019
 #define DEADZONE 100  // Zona morta para evitar tremores
 
 ssd1306_t ssd;
 bool pwm_enabled = true;
 int border_style = 1;
 
 void setup_pwm(uint pin)
 {
     gpio_set_function(pin, GPIO_FUNC_PWM);
     uint slice = pwm_gpio_to_slice_num(pin);
     pwm_set_wrap(slice, 4095);  // Configura o limite do PWM para 4095
     pwm_set_enabled(slice, true);
     pwm_set_gpio_level(pin, 0);  // Inicializa o PWM com 0 (sem brilho)
 }
 
 void debounce_delay()
 {
     sleep_ms(50);
 }
 
 int main()
 {
     stdio_init_all();
 
     gpio_init(BUTTON_B);
     gpio_set_dir(BUTTON_B, GPIO_IN);
     gpio_pull_up(BUTTON_B);
 
     adc_init();
     adc_gpio_init(JOYSTICK_X);
     adc_gpio_init(JOYSTICK_Y);
 
     setup_pwm(LED_RED);
     setup_pwm(LED_BLUE);
     setup_pwm(LED_GREEN);  // Adiciona o PWM para o LED verde
 
     i2c_init(I2C_PORT, 400 * 1000);
     gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
     gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
     gpio_pull_up(I2C_SDA);
     gpio_pull_up(I2C_SCL);
 
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
 
     bool last_joystick_btn_state = gpio_get(JOYSTICK_BTN);
     bool last_button_a_state = gpio_get(BUTTON_A);
 
     // Posição inicial ajustada para (28, 56)
     int x_pos = 59;
     int y_pos = 29;
 
     while (true)
     {
         if (!gpio_get(BUTTON_B))
         {
             printf("[SISTEMA] Entrando em modo BOOTSEL\n");
             reset_usb_boot(0, 0);
         }
 
         bool current_joystick_btn_state = gpio_get(JOYSTICK_BTN);
         if (!current_joystick_btn_state && last_joystick_btn_state)
         {
             debounce_delay();
             gpio_put(LED_GREEN, !gpio_get(LED_GREEN));
             border_style = (border_style == 1) ? 2 : 1;
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
 
         adc_select_input(0);
         uint16_t x_val = adc_read();
         adc_select_input(1);
         uint16_t y_val = adc_read();
 
         int adjusted_x = x_val - JOYSTICK_CENTER_X; // Movimentos no eixo X (direita/esquerda)
         int adjusted_y = y_val - JOYSTICK_CENTER_Y;  // Movimentos no eixo Y (cima/baixo)
 
         // Movimentos corretos para o eixo X (cima/baixo) - Trocar com Y
         if (abs(adjusted_y) > DEADZONE) {
             x_pos += (adjusted_y * 5) / 2048;  // Ajuste proporcional ao valor do joystick
         }
 
         // Movimentos corretos para o eixo Y (direita/esquerda) - Trocar com X
         if (abs(adjusted_x) > DEADZONE) {
             y_pos += (adjusted_x * 5) / 2048;  // Ajuste proporcional ao valor do joystick
         }
 
         // Limitações da tela para não ultrapassar os limites
         if (x_pos < 0) x_pos = 0;
         if (x_pos > WIDTH - 8) x_pos = WIDTH - 8;
         if (y_pos < 0) y_pos = 0;
         if (y_pos > HEIGHT - 8) y_pos = HEIGHT - 8;
 
         // Inverte os valores para ajustar a posição
         int inverted_x_pos = y_pos;  // Agora y_pos vai para o eixo X
         int inverted_y_pos = x_pos;  // Agora x_pos vai para o eixo Y
 
         ssd1306_fill(&ssd, false);
 
         // Desenha as bordas
         if (border_style == 1)
         {
             ssd1306_rect(&ssd, 0, 0, WIDTH, HEIGHT, true, false);  // Manter o tamanho da borda original
         }
         else if (border_style == 2)
         {
             ssd1306_rect(&ssd, 0, 0, WIDTH, HEIGHT, true, false);  // Manter o tamanho da borda original
             ssd1306_rect(&ssd, 1, 1, WIDTH - 2, HEIGHT - 2, true, false);
             ssd1306_rect(&ssd, 2, 2, WIDTH - 4, HEIGHT - 4, true, false);
         }
 
         // Desenha o quadrado com as coordenadas invertidas
         ssd1306_rect(&ssd, inverted_x_pos, inverted_y_pos, 8, 8, true, true);
         ssd1306_send_data(&ssd);
 
         printf("[JOYSTICK] X: %4d | Y: %4d | Pos: (%3d, %3d)\n", x_val, y_val, inverted_x_pos, inverted_y_pos);
 
         // Ajusta a intensidade do PWM dos LEDs com base na posição do joystick (ajustando a intensidade com base no eixo X)
         if (pwm_enabled)
         {
             pwm_set_gpio_level(LED_RED, (adjusted_x + 2048) * 2047 / 4096);  // Redimensiona para o intervalo PWM
             pwm_set_gpio_level(LED_BLUE, (adjusted_y + 2048) * 2047 / 4096);  // Redimensiona para o intervalo PWM
         }
 
         sleep_ms(20);
     }
 }
 