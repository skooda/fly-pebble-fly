#include <pebble.h>
#include <math.h>

static Window *_window;
static Layer *_canvas_layer;

// forward declarations (fixes "not used" / implicit-declaration issues)
static void _window_load(Window *window);
static void _window_unload(Window *window);

static int16_t _center_x;
static int16_t _center_y;

static int16_t _att_x;
static int16_t _att_y;
static int16_t _att_z;

//---------------
// Configurations
//---------------
static const uint16_t _circle_radius = 15;
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

static void _draw_circle(Layer *layer, GContext *ctx) {
  int16_t circle_x = _center_x - (_att_x/_sensitivity);
  int16_t circle_y = _center_y + (_att_y/_sensitivity);

  graphics_fill_circle(ctx, GPoint(circle_x, circle_y), _circle_radius);
}

static void _draw_angled_line(Layer *layer, GContext *ctx, float angle_rad, int16_t cx, int16_t cy) {
  // Draw a line at a specific angle from the center
  int16_t length = 100; // Length of the line
  GPoint start = GPoint(
    cx - (int16_t)(length * cosf(angle_rad)),
    cy - (int16_t)(length * sinf(angle_rad))
  );
  GPoint end = GPoint(
    cx + (int16_t)(length * cosf(angle_rad)),
    cy + (int16_t)(length * sinf(angle_rad))
  );
  graphics_draw_line(ctx, start, end);
}

static void _draw_horizon(Layer *layer, GContext *ctx) {
  int16_t horizon_y = _center_y + (_att_z/_sensitivity);
  int16_t horizon_angle = (-_att_x / 16);
  int16_t horizon_angle_rad = horizon_angle * (PI / 180.0f);
  // Draw horizon line with rotation based on accelerometer data
  _draw_angled_line(layer, ctx, horizon_angle * (PI / 180.0f), _center_x, horizon_y);
  
}

static void _draw(Layer *layer, GContext *ctx) {
  _clear_screen(layer, ctx);
  _draw_circle(layer, ctx);
  _draw_horizon(layer, ctx);
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
