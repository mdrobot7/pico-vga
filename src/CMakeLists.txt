add_library(libpicovga STATIC)

target_sources(libpicovga PUBLIC
    audio.c
    controller.c
    draw-2d.c
    draw-3d.c
    init.c
    init-display.c
    peripheral-mode.c
    render/render.c
    render/render-2d.c
    sd.c
    stats.c
    utils-internal.c
    utils.c
)

target_include_directories(libpicovga PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/../inc
)

# Create C header file with the name <pio program>.pio.h
# Note: You need one call per file
pico_generate_pio_header(libpicovga
        ${CMAKE_CURRENT_LIST_DIR}/color.pio
)

target_link_libraries(libpicovga STATIC
    pico_stdlib
    hardware_gpio
    hardware_pio
    hardware_dma
    hardware_irq
    hardware_pwm
    hardware_i2c
    hardware_spi
    pico_multicore
    pico_time
)