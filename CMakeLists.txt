# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define o tipo de placa (Pico ou Pico W)
set(PICO_BOARD pico CACHE STRING "Board type")

# Importa o Raspberry Pi Pico SDK
include(pico_sdk_import.cmake)

# Nome do projeto
project(joystick_led_display C CXX ASM)

# Inicializa o Raspberry Pi Pico SDK
pico_sdk_init()

# Adiciona o código-fonte do projeto
add_executable(joystick_led_display  
    Adc_Pwm_Display.c   
    lib/ssd1306.c
)

# Configuração do nome e versão do programa
pico_set_program_name(joystick_led_display "joystick_led_display")
pico_set_program_version(joystick_led_display "0.1")

# Habilita saída UART e USB
pico_enable_stdio_uart(joystick_led_display 1)
pico_enable_stdio_usb(joystick_led_display 1)

# Adiciona bibliotecas necessárias para ADC, PWM e I2C
target_link_libraries(joystick_led_display 
    pico_stdlib 
    hardware_adc 
    hardware_pwm
    hardware_i2c
)

# Adiciona diretórios de inclusão para o projeto
target_include_directories(joystick_led_display PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}
    lib
)

# Gera arquivos extras para o Pico
pico_add_extra_outputs(joystick_led_display)
