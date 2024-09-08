add_library(libpicovga INTERFACE)

target_sources(libpicovga INTERFACE
    audio/audio.c

    # controller/controller.c

    vga/draw-2d.c
    vga/draw-3d.c
    vga/draw-common.c
    vga/render-2d.c
    vga/render-3d.c
    vga/render.c
    vga/vga.c
)

target_include_directories(libpicovga INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/../inc
    ${CMAKE_CURRENT_LIST_DIR}
)

target_compile_definitions(libpicovga INTERFACE PARAM_ASSERTIONS_ENABLE_ALL=1)
target_compile_options(libpicovga INTERFACE -Wno-discarded-qualifiers -Wno-discarded-array-qualifiers
    -Wno-unused-function -Wno-comment)

# Create C header file with the name <pio program>.pio.h
# Note: You need one call per file
pico_generate_pio_header(libpicovga
        ${CMAKE_CURRENT_LIST_DIR}/vga/color.pio
)

target_link_libraries(libpicovga INTERFACE
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