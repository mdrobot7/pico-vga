#include "hardware/pio.h"
#include "hardware/sync.h"
#include "pico-vga.h"
#include "pico/stdlib.h"

#define RENDER_QUEUE_LEN (100)
vga_render_item_t render_queue[RENDER_QUEUE_LEN]; // The render queue

vga_config_t display_conf = {
  .pio                    = pio0,
  .base_resolution        = RES_800x600,
  .scaled_resolution      = RES_SCALED_400x300,
  .render_queue           = render_queue,
  .render_queue_len       = RENDER_QUEUE_LEN,
  .auto_render            = true,
  .antialiasing           = false,
  .num_interpolated_lines = 0,
  .color_delay_cycles     = 0
};

static void demo_lines_up() {
  int item = 0;
  draw_clear();
  for (int y = 0; y < vga_get_height(); y += 10) {
    draw2d_line(&render_queue[item++], vga_get_width() - 1, 0, 0, y, COLOR_RED);
    sleep_ms(100);
  }
}

static void demo_lines_down() {
  int item = 0;
  draw_clear();
  for (int y = 0; y < vga_get_height(); y += 10) {
    draw2d_line(&render_queue[item++], 0, 0, vga_get_width() - 1, y, COLOR_RED);
    sleep_ms(100);
  }
}

static void demo_rectangles() {
  int item = 0;
  draw_clear();
  for (int i = 0; i < 250; i += 10) {
    draw2d_rectangle(&render_queue[item++], 0 + i, 0 + i, vga_get_width() - 1 - i, vga_get_height() - 1 - i, COLOR_BLUE);
    sleep_ms(100);
  }
}

static void demo_filled_rectangles() {
  int item = 0;
  draw_clear();
  for (int y = 0; y < vga_get_height(); y += vga_get_height() / 3) {
    for (int x = 0; x < vga_get_width(); x += vga_get_width() / 4) {
      draw2d_rectangle_filled(&render_queue[item++], x + 25, y + 25, x + 75, y + 75, COLOR_BLUE);
      sleep_ms(100);
    }
  }
}

static void demo_triangles_up() {
  int item     = 0;
  int center_x = vga_get_width() / 2;
  draw_clear();
  for (int i = 0; i < 125; i++) {
    draw2d_triangle(&render_queue[item++], i, vga_get_height() - 1 - i, center_x, i, vga_get_width() - 1 - i, vga_get_height() - 1 - i, COLOR_GREEN);
    sleep_ms(100);
  }
}

static void demo_triangles_down() {
  int item     = 0;
  int center_x = vga_get_width() / 2;
  draw_clear();
  for (int i = 0; i < 125; i++) {
    draw2d_triangle(&render_queue[item++], i, i, center_x, vga_get_height() - 1 - i, vga_get_width() - 1 - i, i, COLOR_GREEN);
    sleep_ms(100);
  }
}

static void demo_filled_triangles() {
  int item = 0;
  draw_clear();
  for (int y = 0; y < vga_get_height(); y += vga_get_height() / 3) {
    for (int x = 0; x < vga_get_width(); x += vga_get_width() / 4) {
      draw2d_triangle_filled(&render_queue[item++], x + 25, y + 125, x + 125, y + 25, x + 100, y + 150, COLOR_MAGENTA);
      sleep_ms(100);
    }
  }
}

static void demo_circles() {
  int item = 0;
  draw_clear();
  for (int rad = 10; rad < 120; rad += 10) {
    draw2d_circle(&render_queue[item++], vga_get_width() / 2, vga_get_height() / 2, rad, COLOR_TEAL);
    sleep_ms(100);
  }
}

static void demo_filled_circles() {
  int item = 0;
  draw_clear();
  for (int y = 0; y < vga_get_height(); y += vga_get_height() / 3) {
    for (int x = 0; x < vga_get_width(); x += vga_get_width() / 4) {
      draw2d_circle_filled(&render_queue[item++], x + 50, y + 50, 50, COLOR_CYAN);
      sleep_ms(100);
    }
  }
}

static void demo_polygon() {
  uint16_t points[][2] = { { 87, 248 }, { 86, 19 }, { 141, 207 }, { 115, 175 }, { 1, 53 }, { 55, 151 }, { 4, 244 }, { 65, 197 }, { 211, 250 }, { 233, 77 } };
  draw_clear();
  draw2d_polygon(&render_queue[0], points, LEN(points), COLOR_GREEN);
  sleep_ms(500);
  draw_clear(); // Clear this one off immediately, since points[] is stack allocated
}

static void demo_polygon_filled() {
  uint16_t points[][2] = { { 209, 221 }, { 201, 132 }, { 250, 139 }, { 22, 223 }, { 249, 91 }, { 85, 86 }, { 193, 23 }, { 93, 185 }, { 173, 210 }, { 144, 14 } };
  draw_clear();
  draw2d_polygon_filled(&render_queue[0], points, LEN(points), COLOR_GRAY);
  sleep_ms(500);
  draw_clear(); // Clear this one off immediately, since points[] is stack allocated
}

static void demo_string() {
  const char str[] = "Hello, world!";
  draw_clear();
  draw2d_text(&render_queue[0], 100, 100, 300, str, COLOR_WHITE, false);
  sleep_ms(500);
  draw_clear(); // Clear this one off immediately, since str[] is stack allocated
}

static void demo_sprite() {
  // 0x739000 -> 0x43 -- olive drab
  // 0xFFA450 -> 0xC1 -- skin
  // 0xF41414 -> 0x90 -- red
  // Wa-hoo!
  static const vga_color_t sprite[16][16] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC1, 0xC1, 0xC1 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x00, 0x00, 0xC1, 0xC1, 0xC1 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0xC1, 0xC1 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x43, 0x43, 0xC1, 0xC1, 0x43, 0xC1, 0x00, 0x43, 0x43, 0x43 },
    { 0x00, 0x00, 0x00, 0x00, 0x43, 0xC1, 0x43, 0xC1, 0xC1, 0xC1, 0x43, 0xC1, 0xC1, 0x43, 0x43, 0x43 },
    { 0x00, 0x00, 0x00, 0x00, 0x43, 0xC1, 0x43, 0x43, 0xC1, 0xC1, 0xC1, 0x43, 0xC1, 0xC1, 0xC1, 0x43 },
    { 0x00, 0x00, 0x00, 0x00, 0x43, 0x43, 0xC1, 0xC1, 0xC1, 0xC1, 0x43, 0x43, 0x43, 0x43, 0x43, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0x43, 0x00, 0x00 },
    { 0x00, 0x00, 0x43, 0x43, 0x43, 0x43, 0x43, 0x90, 0x43, 0x43, 0x43, 0x90, 0x43, 0x00, 0x00, 0x00 },
    { 0x00, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x90, 0x43, 0x43, 0x43, 0x90, 0x00, 0x00, 0x43 },
    { 0xC1, 0xC1, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x90, 0x90, 0x90, 0x90, 0x90, 0x00, 0x00, 0x43 },
    { 0xC1, 0xC1, 0xC1, 0x00, 0x90, 0x90, 0x43, 0x90, 0x90, 0xC1, 0x90, 0x90, 0xC1, 0xC1, 0x43, 0x43 },
    { 0x00, 0xC1, 0x00, 0x43, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x43, 0x43 },
    { 0x00, 0x00, 0x43, 0x43, 0x43, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x43, 0x43 },
    { 0x00, 0x43, 0x43, 0x43, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x43, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  };

  draw_clear();
  draw2d_sprite(&render_queue[0], 100, 100, (vga_color_t *) sprite, 16, 16, COLOR_BLACK);
}

int main() {
  // DEBUG ONLY: The debugger pausing at the start of main() causes it to read all
  // spinlock regs, "claiming" all spinlocks. This should be fixed in future SDK revs,
  // see https://github.com/raspberrypi/pico-examples/issues/363. For now, this.
  spin_locks_reset();

  vga_init(&display_conf);

  demo_lines_up();
  sleep_ms(500);
  demo_lines_down();
  sleep_ms(500);
  demo_rectangles();
  sleep_ms(500);
  demo_filled_rectangles();
  sleep_ms(500);
  demo_triangles_up();
  sleep_ms(500);
  demo_triangles_down();
  sleep_ms(500);
  demo_filled_triangles();
  sleep_ms(500);
  demo_circles();
  sleep_ms(500);
  demo_filled_circles();
  sleep_ms(500);
  demo_polygon();
  sleep_ms(500);
  demo_polygon_filled();
  sleep_ms(500);
  demo_string();
  sleep_ms(500);
  demo_sprite();

  while (true) {}
}