cmake_minimum_required(VERSION 3.13)

# Set the C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_BOARD pico_w)

include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

project(PICOWGAMETEST C CXX ASM)

pico_sdk_init()

include_directories(
	include
)

add_executable(${PROJECT_NAME}
	src/main.c
)

target_compile_definitions(${PROJECT_NAME}
	PUBLIC
	CYW43_LWIP=0
	PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64
	#PICO_DEFAULT_UART_TX_PIN=12
	#PICO_DEFAULT_UART_RX_PIN=13
	#PICO_DEFAULT_BOOT_STAGE2=boot2_generic_03h
	#PICO_FLASH_SPI_CLKDIV=8
	)


target_link_libraries(${PROJECT_NAME}
pico_stdlib
pico_bootrom
pico_multicore
pico_platform
pico_rand

hardware_gpio
hardware_adc
hardware_flash
hardware_pio
hardware_dma
hardware_pwm
hardware_spi
hardware_sync
hardware_timer
hardware_exception
pico_btstack_cyw43
pico_btstack_hci_transport_cyw43
pico_btstack_ble
pico_btstack_classic
pico_cyw43_arch_threadsafe_background

)

pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

pico_add_extra_outputs(${PROJECT_NAME})