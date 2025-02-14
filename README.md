<div align="center">
  <img src="https://github.com/LucaScripts/ComunicaoSerial/blob/main/docs/Group%20658.png?raw=true" alt="Logo do Projeto" width="50%"/>
</div>

# **Conversores A/D no RP2040 com BitDogLab**

📌 **Autor**: Lucas Dias  
📆 **Data**: 14/02/2025  

Este projeto tem como objetivo explorar o funcionamento do conversor analógico-digital (ADC) no RP2040 e integrar diferentes componentes na placa de desenvolvimento BitDogLab. A atividade envolve a leitura dos valores de um joystick e seu uso para controlar LEDs RGB via PWM e exibir um quadrado móvel no display SSD1306 via I2C.

---

## 📌 **Objetivos**
✅ Compreender o funcionamento do conversor analógico-digital (ADC) no RP2040.  
✅ Controlar a intensidade de dois LEDs RGB com base nos valores do joystick.  
✅ Representar a posição do joystick no display SSD1306 com um quadrado móvel.  
✅ Aplicar o protocolo de comunicação I2C na integração com o display.  
✅ Implementar interrupções (IRQs) para os botões.  
✅ Implementar debounce via software.  

---

## 🛠 **Materiais Necessários**
🔹 1x **Placa BitDogLab**  
🔹 1x **LED RGB** (GPIOs 11, 12 e 13)  
🔹 1x **Joystick** (GPIOs 26 e 27)  
🔹 1x **Botão do Joystick** (GPIO 22)  
🔹 1x **Botão A** (GPIO 5)  
🔹 1x **Display SSD1306** (I2C - GPIOs 14 e 15)  

---

## 🔧 **Configuração dos Componentes**
- **LED Azul**: Intensidade ajustada pelo eixo Y do joystick.  
- **LED Vermelho**: Intensidade ajustada pelo eixo X do joystick.  
- **Quadrado no Display**: Move-se conforme os valores do joystick.  
- **Botão do Joystick**:
  - Alterna o estado do LED Verde.  
  - Modifica o estilo da borda do display SSD1306.  
- **Botão A**:
  - Liga ou desliga os LEDs PWM.  

---

## 🏗 **Esquema de Ligação**
| Componente  | Pino da BitDogLab |
|------------|------------------|
| LED RGB    | GPIO 11, 12, 13  |
| Joystick X | GPIO 26          |
| Joystick Y | GPIO 27          |
| Botão Joy  | GPIO 22          |
| Botão A    | GPIO 5           |
| Display    | GPIO 14, 15 (I2C) |

---

## 📜 **Implementação**
### 1️⃣ **Leitura do Joystick e Controle dos LEDs**
- Conversão A/D dos valores do joystick.
- Aplicação de PWM para ajustar o brilho dos LEDs RGB.

### 2️⃣ **Movimentação do Quadrado no Display**
- Exibição do quadrado 8x8 pixels.
- Movimento proporcional aos valores lidos do joystick.

### 3️⃣ **Interrupções e Debounce**
- Implementação de interrupções para os botões.
- Tratamento de bouncing via software.

---

## 📥 Clonando o Repositório e Compilando o Código

Para baixar o código e começar a trabalhar com ele, clone o repositório e carregue o código na placa seguindo os passos abaixo:

![Clonando o Repositório](https://github.com/LucaScripts/PWM/blob/main/docs/Bem-vindo%20-%20Visual%20Studio%20Code%202025-01-31%2018-49-32%20(1).gif?raw=true)

---

🚦 Demonstração da Simulação Wokwi
Abaixo está uma prévia da simulação da comunicação serial no Wokwi:

![Wokwi](https://github.com/LucaScripts/ConversoresAD/blob/main/docs/diagram.json%20-%20ConversoresAD%20-%20Visual%20Studio%20Code%202025-02-14%2018-49-32.gif?raw=true)

[Demonstração no Wokwi]([https://wokwi.com/projects/422902789255096321)

---
## 🚦 **Demonstração do Projeto**

Abaixo está uma prévia da execução do projeto:
![Projeto em Execução](https://github.com/LucaScripts/PWM/blob/main/docs/diagram.json%20-%20pwm.gif?raw=true)  

🔗 [Demonstração no Google Drive](https://drive.google.com/file/d/1_ax07drMcusfKGuXP3P1VEitiIQVuKGi/view?usp=sharing)

---

## 📌 **Melhorias Futuras**
- **📡 Comunicação Serial:** Adição de monitoramento via UART.  
- **🔄 Otimização do PWM:** Melhor precisão no controle dos LEDs.  
- **🖥️ Interface Gráfica:** Exibição avançada no display SSD1306.  

---

