#include <pebble.h>
#include <math.h>

static Window *_window;
static Layer *_canvas_layer;

// forward declarations (fixes "not used" / implicit-declaration issues)
static void _window_load(Window *window);
static void _window_unload(Window *window);
static GPoint _translate_point(int16_t x, int16_t y, float angle_rad, int16_t distance);

static int16_t _center_x;
static int16_t _center_y;

static int16_t _att_x;
static int16_t _att_y;
static int16_t _att_z;

static float _horizon_angle_rad;

//---------------
// Configurations
//---------------
static const uint16_t _display_diameter = 130;
static const uint16_t _sensitivity = 10;
static const float PI = 3.14159265f;

//---------------
// Handlers
///---------------
static void _handle_click_select(ClickRecognizerRef recognizer, void *context) {
  // Todo: Implement select button functionality
}

static void _handle_click_up(ClickRecognizerRef recognizer, void *context) {
  // Todo: Implement up button functionality
}

static void _handle_click_down(ClickRecognizerRef recognizer, void *context) {
  // Todo: Implement down button functionality
}

static void _handle_accelerometer(AccelData *data, uint32_t num_samples) {
  int32_t sum_x = 0, sum_y = 0, sum_z = 0;
  for (uint32_t i = 0; i < num_samples; i++) {
    sum_x += data[i].x;
    sum_y += data[i].y;
    sum_z += data[i].z;
  }
  _att_x = sum_x / (int32_t)num_samples;
  _att_y = sum_y / (int32_t)num_samples;
  _att_z = sum_z / (int32_t)num_samples;
  
  int16_t horizon_angle = (-_att_x / 16);
  _horizon_angle_rad = horizon_angle * (PI / 180.0f);
  
  layer_mark_dirty(_canvas_layer);
}

//---------------
// Drawing
//---------------
static void _clear_screen(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorBlack);
}


static void _draw_angled_line(GContext *ctx, float angle_rad, int16_t sx, int16_t sy, int16_t length) {
  // Draw a line at a specific angle
  GPoint start = GPoint(sx, sy);
  GPoint end = GPoint(
    sx + (int16_t)(length * cosf(angle_rad)),
    sy + (int16_t)(length * sinf(angle_rad))
  );
  graphics_draw_line(ctx, start, end);
}

static GPoint _translate_point(int16_t x, int16_t y, float angle_rad, int16_t distance) {
  return GPoint(
    x + (int16_t)(distance * cosf(angle_rad)),
    y + (int16_t)(distance * sinf(angle_rad))
  );
}

static void _draw_angled_offset_line(GContext *ctx, float angle_rad, int16_t sx, int16_t sy, int16_t length, int16_t offset) {
  GPoint start = _translate_point(sx, sy, angle_rad + (PI / 2), offset);
  GPoint end = GPoint(
    start.x + (int16_t)(length * cosf(angle_rad)),
    start.y + (int16_t)(length * sinf(angle_rad))
  );
  graphics_draw_line(ctx, start, end);
}
  
static void _draw_horizon(GContext *ctx) {
  int16_t horizon_y = _center_y + (_att_z/_sensitivity);

  // Draw horizon line with rotation based on accelerometer data
  graphics_context_set_stroke_width(ctx, 2);
  _draw_angled_line(ctx, _horizon_angle_rad, _center_x, horizon_y, _display_diameter);
  _draw_angled_line(ctx, _horizon_angle_rad+PI, _center_x, horizon_y, _display_diameter);
  graphics_context_set_stroke_width(ctx, 1);

  // Draw horizon segments by 30-degree increments
  _draw_angled_line(ctx, _horizon_angle_rad+(PI/6), _center_x, horizon_y, _display_diameter);
  _draw_angled_line(ctx, _horizon_angle_rad+(2*PI/6), _center_x, horizon_y, _display_diameter);
  _draw_angled_line(ctx, _horizon_angle_rad+(4*PI/6), _center_x, horizon_y, _display_diameter);
  _draw_angled_line(ctx, _horizon_angle_rad+(5*PI/6), _center_x, horizon_y, _display_diameter);

  // Draw nose attitude horizontal lines
  _draw_angled_offset_line(ctx, _horizon_angle_rad+PI, _center_x, horizon_y, _display_diameter/14, -_display_diameter/6);
  _draw_angled_offset_line(ctx, _horizon_angle_rad+PI, _center_x, horizon_y, _display_diameter/8, -_display_diameter/4);
  _draw_angled_offset_line(ctx, _horizon_angle_rad, _center_x, horizon_y, _display_diameter/14, -_display_diameter/6);
  _draw_angled_offset_line(ctx, _horizon_angle_rad, _center_x, horizon_y, _display_diameter/8, -_display_diameter/4);
 
  _draw_angled_offset_line(ctx, _horizon_angle_rad+PI, _center_x, horizon_y, _display_diameter/15, _display_diameter/6);
  _draw_angled_offset_line(ctx, _horizon_angle_rad+PI, _center_x, horizon_y, _display_diameter/10, _display_diameter/4);
  _draw_angled_offset_line(ctx, _horizon_angle_rad, _center_x, horizon_y, _display_diameter/15, _display_diameter/6);
  _draw_angled_offset_line(ctx, _horizon_angle_rad, _center_x, horizon_y, _display_diameter/10, _display_diameter/4);
}

static void _draw_crown(GContext *ctx) {
  // Draw crown indicator at the top center
  GPoint crown_top = GPoint(_center_x, _center_y - (_display_diameter / 2) + 15);
  GPoint crown_left = GPoint(_center_x - 5, crown_top.y + 10);
  GPoint crown_right = GPoint(_center_x + 5, crown_top.y + 10);

  graphics_draw_line(ctx, crown_top, crown_left);
  graphics_draw_line(ctx, crown_top, crown_right);
  graphics_draw_line(ctx, crown_left, crown_right);
}

static void _draw_border(GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, (int)(_display_diameter * 0.2f)+20);
  graphics_draw_circle(ctx, GPoint(_center_x, _center_y), _display_diameter/2+10);

  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_circle(ctx, GPoint(_center_x, _center_y), _display_diameter/2+30);
  
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_circle(ctx, GPoint(_center_x, _center_y), _display_diameter/2.4);

} 

static void _draw(Layer *layer, GContext *ctx) {
  _clear_screen(layer, ctx);
  _draw_horizon(ctx);
  _draw_crown(ctx);
  _draw_border(ctx);
}

//---------------
// Initialization
//---------------
static void _click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, _handle_click_select);
  window_single_click_subscribe(BUTTON_ID_UP, _handle_click_up);
  window_single_click_subscribe(BUTTON_ID_DOWN, _handle_click_down);
  accel_data_service_subscribe(5, _handle_accelerometer);
  accel_service_set_sampling_rate(ACCEL_SAMPLING_100HZ);
}

static void _init(void) {
  _window = window_create();
  window_set_click_config_provider(_window, _click_config_provider);
  window_set_window_handlers(_window, (WindowHandlers) {
    .load = _window_load,
    .unload = _window_unload,
  });
  const bool animated = true;
  window_stack_push(_window, animated);
}

static void _window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // calculate center of screen
  _center_x = bounds.size.w / 2;
  _center_y = bounds.size.h / 2;

  // create full-screen canvas and use update proc to draw circle
  _canvas_layer = layer_create(bounds);
  layer_set_update_proc(_canvas_layer, _draw);
  layer_add_child(window_layer, _canvas_layer);
}

// Cleanup
static void _window_unload(Window *window) {
  layer_destroy(_canvas_layer);
  accel_data_service_unsubscribe();
}

static void _destroy(void) {
  window_destroy(_window);
}

// Main
int main(void) {
  _init();
  app_event_loop();
  _destroy();
}
