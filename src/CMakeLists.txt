add_library(libpicovga STATIC)

target_sources(libpicovga PUBLIC
    audio/audio.c

    # controller/controller.c

    vga/draw-2d.c
    vga/draw-3d.c
    vga/vga.c
    vga/render.c
    vga/render-2d.c

    utils-internal.c
)

target_include_directories(libpicovga PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/../inc
)

# Create C header file with the name <pio program>.pio.h
# Note: You need one call per file
pico_generate_pio_header(libpicovga
        ${CMAKE_CURRENT_LIST_DIR}/vga/color.pio
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