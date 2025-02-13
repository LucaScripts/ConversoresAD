/*
 * Projeto: Controle de Joystick com Display OLED e LEDs PWM
 * Autor: Lucas Dias da Silva
 * Descrição: 
 *   - Este código configura um joystick analógico para controle de um display OLED SSD1306.
 *   - Os LEDs RGB respondem aos movimentos do joystick através de PWM.
 *   - O botão do joystick e o botão adicional controlam funcionalidades como habilitação de PWM e mudança de borda na tela.
 *   - Suporte ao modo BOOTSEL ativado via botão dedicado.
 */

 #include <stdio.h>
 #include "pico/stdlib.h"
 #include "hardware/adc.h"
 #include "hardware/i2c.h"
 #include "hardware/pwm.h"
 #include "hardware/gpio.h"
 #include "lib/ssd1306.h"
 #include "pico/bootrom.h" // Para modo BOOTSEL
 
 // Definição de pinos e periféricos
 #define I2C_PORT i2c1
 #define I2C_SDA 14
 #define I2C_SCL 15
 #define DISPLAY_ADDR 0x3C
 
 #define JOYSTICK_X 26
 #define JOYSTICK_Y 27
 #define JOYSTICK_BTN 22
 #define BUTTON_A 5
 #define BUTTON_B 6 // Modo BOOTSEL
 
 #define LED_GREEN 11
 #define LED_BLUE 12
 #define LED_RED 13
 
 #define JOYSTICK_CENTER_X 1902
 #define JOYSTICK_CENTER_Y 1972
 
 // Estrutura do display OLED
 ssd1306_t ssd;
 bool pwm_enabled = true;
 int border_style = 0;
 
 // Função para configurar PWM nos LEDs
 void setup_pwm(uint pin) {
     gpio_set_function(pin, GPIO_FUNC_PWM);
     uint slice = pwm_gpio_to_slice_num(pin);
     pwm_set_wrap(slice, 4095);
     pwm_set_enabled(slice, true);
     pwm_set_gpio_level(pin, 0); // LEDs iniciam apagados
 }
 
 // Função para debounce de botões
 void debounce_delay() {
     sleep_ms(50);
 }
 
 int main() {
     stdio_init_all();
     
     // Configuração do botão B para modo BOOTSEL
     gpio_init(BUTTON_B);
     gpio_set_dir(BUTTON_B, GPIO_IN);
     gpio_pull_up(BUTTON_B);
     
     // Inicialização do ADC para leitura do joystick
     adc_init();
     adc_gpio_init(JOYSTICK_X);
     adc_gpio_init(JOYSTICK_Y);
     
     // Configuração do PWM para LEDs
     setup_pwm(LED_RED);
     setup_pwm(LED_BLUE);
     
     // Configuração do I2C e inicialização do display OLED
     i2c_init(I2C_PORT, 400 * 1000);
     gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
     gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
     gpio_pull_up(I2C_SDA);
     gpio_pull_up(I2C_SCL);
     ssd1306_init(&ssd, WIDTH, HEIGHT, false, DISPLAY_ADDR, I2C_PORT);
     ssd1306_fill(&ssd, false);
     ssd1306_send_data(&ssd);
     
     // Configuração dos botões
     gpio_init(JOYSTICK_BTN);
     gpio_set_dir(JOYSTICK_BTN, GPIO_IN);
     gpio_pull_up(JOYSTICK_BTN);
     
     gpio_init(BUTTON_A);
     gpio_set_dir(BUTTON_A, GPIO_IN);
     gpio_pull_up(BUTTON_A);
     
     // Configuração do LED verde
     gpio_init(LED_GREEN);
     gpio_set_dir(LED_GREEN, GPIO_OUT);
     gpio_put(LED_GREEN, false);
     
     bool last_joystick_btn_state = gpio_get(JOYSTICK_BTN);
     bool last_button_a_state = gpio_get(BUTTON_A);
 
     // Variáveis para armazenar os valores anteriores
     int last_x_val = -1;
     int last_y_val = -1;
     int last_adjusted_x = -1;
     int last_adjusted_y = -1;
     int last_x_pos = -1;
     int last_y_pos = -1;
 
     while (true) {
         // Verifica botão B para modo BOOTSEL
         if (!gpio_get(BUTTON_B)) {
             printf("Botão B pressionado - Entrando em modo BOOTSEL\n");
             reset_usb_boot(0, 0);
         }
         
         // Verifica botão do joystick
         bool current_joystick_btn_state = gpio_get(JOYSTICK_BTN);
         if (!current_joystick_btn_state && last_joystick_btn_state) {
             debounce_delay();
             static bool led_state = false;
             led_state = !led_state;
             gpio_put(LED_GREEN, led_state);
             border_style = (border_style + 1) % 3;
             printf("Joystick pressionado, LED Verde: %d, border_style: %d\n", led_state, border_style);
         }
         last_joystick_btn_state = current_joystick_btn_state;
         
         // Verifica botão A
         bool current_button_a_state = gpio_get(BUTTON_A);
         if (!current_button_a_state && last_button_a_state) {
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
         
         // Ajuste dos valores
         int adjusted_x = x_val - JOYSTICK_CENTER_X;
         int adjusted_y = y_val - JOYSTICK_CENTER_Y;
         adjusted_x = adjusted_x < -1902 ? -1902 : (adjusted_x > 2193 ? 2193 : adjusted_x);
         adjusted_y = adjusted_y < -1972 ? -1972 : (adjusted_y > 2123 ? 2123 : adjusted_y);
         
         if (pwm_enabled) {
             pwm_set_gpio_level(LED_BLUE, abs(adjusted_x) * 2);
             pwm_set_gpio_level(LED_RED, abs(adjusted_y) * 2);
         } else {
             pwm_set_gpio_level(LED_BLUE, 0);
             pwm_set_gpio_level(LED_RED, 0);
         }
         
         // Mapeamento para tela OLED
         uint8_t x_pos = ((adjusted_x + 1902) * (WIDTH - 8)) / 4095;
         uint8_t y_pos = ((adjusted_y + 1972) * (HEIGHT - 8)) / 4095;
         
         // Verificar se houve alteração nos valores antes de imprimir
         if (x_val != last_x_val || y_val != last_y_val || adjusted_x != last_adjusted_x || adjusted_y != last_adjusted_y || x_pos != last_x_pos || y_pos != last_y_pos) {
             // Atualizar os valores anteriores
             last_x_val = x_val;
             last_y_val = y_val;
             last_adjusted_x = adjusted_x;
             last_adjusted_y = adjusted_y;
             last_x_pos = x_pos;
             last_y_pos = y_pos;
 
             // Printar as alterações
             printf("x_val: %d, y_val: %d, adjusted_x: %d, adjusted_y: %d, x_pos: %d, y_pos: %d\n", x_val, y_val, adjusted_x, adjusted_y, x_pos, y_pos);
         }
 
         // Mapeamento e desenho no OLED
         ssd1306_fill(&ssd, false);
         if (border_style == 1) {
             ssd1306_rect(&ssd, 0, 0, WIDTH, HEIGHT, true, false);
         } else if (border_style == 2) {
             ssd1306_rect(&ssd, 0, 0, WIDTH, HEIGHT, true, false);
             ssd1306_rect(&ssd, 1, 1, WIDTH - 2, HEIGHT - 2, true, false);
         }
         ssd1306_rect(&ssd, x_pos, y_pos, 8, 8, true, true);
         ssd1306_send_data(&ssd);
         
         sleep_ms(50);
     }
 }
 