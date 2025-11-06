#include <pebble.h>

static Window *s_window;
static TextLayer *s_text_layer;

static void _handle_click_select(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_text_layer, "Select");
}

static void _handle_click_up(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_text_layer, "Up");
}

static void _handle_click_down(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_text_layer, "Down");
}

static void _handle_accelerometer(AccelData *data, uint32_t num_samples) {
  static int16_t x = 0, y = 0, z = 0;
  for (uint32_t i = 0; i < num_samples; i++) {
    x += data[i].x;
    y += data[i].y;
    z += data[i].z;
  }
  x /= num_samples;
  y /= num_samples;
  z /= num_samples;
  if (x > 2000 || x < -2000 || y > 2000 || y < -2000 || z > 2000 || z < -2000) {
    text_layer_set_text(s_text_layer, "Shake detected!");
  }
}

static void _click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, _handle_click_select);
  window_single_click_subscribe(BUTTON_ID_UP, _handle_click_up);
  window_single_click_subscribe(BUTTON_ID_DOWN, _handle_click_down);
  accel_data_service_subscribe(5, _handle_accelerometer);
  accel_service_set_sampling_rate(ACCEL_SAMPLING_100HZ);
}

static void _window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_text_layer = text_layer_create(GRect(0, 72, bounds.size.w, 20));
  text_layer_set_text(s_text_layer, "Press a button");
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_text_layer));
}

static void _window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
}

static void _init(void) {
  s_window = window_create();
  window_set_click_config_provider(s_window, _click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = _window_load,
    .unload = _window_unload
  ,
  });
  const bool animated = true;
  window_stack_push(s_window, animated);
}

static void _destroy(void) {
  window_destroy(s_window);
}

int main(void) {
  _init();
  app_event_loop();
  _destroy();
}
