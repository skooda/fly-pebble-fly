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
static const uint16_t _border_width = 16;
static const uint16_t _sensitivity = 10;
static const float PI = 3.14159265f;

//---------------
// Handlers
///---------------
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


static void _draw_angled_line(GContext *ctx, float angle_rad, int16_t sx, int16_t sy, int16_t start_at, int16_t length) {
  // Draw a line at a specific angle
  GPoint start = GPoint(
    sx + (int16_t)(start_at * cosf(angle_rad)),
    sy + (int16_t)(start_at * sinf(angle_rad))
  );
  GPoint end = GPoint(
    start.x + (int16_t)(length * cosf(angle_rad)),
    start.y + (int16_t)(length * sinf(angle_rad))
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
  _draw_angled_line(ctx, _horizon_angle_rad, _center_x, horizon_y, 0, _display_diameter);
  _draw_angled_line(ctx, _horizon_angle_rad+PI, _center_x, horizon_y, 0, _display_diameter);
  graphics_context_set_stroke_width(ctx, 1);

  // Draw horizon segments by 30-degree increments
  _draw_angled_line(ctx, _horizon_angle_rad+(PI/6), _center_x, horizon_y, 0, _display_diameter);
  _draw_angled_line(ctx, _horizon_angle_rad+(2*PI/6), _center_x, horizon_y, 0, _display_diameter);
  _draw_angled_line(ctx, _horizon_angle_rad+(4*PI/6), _center_x, horizon_y, 0, _display_diameter);
  _draw_angled_line(ctx, _horizon_angle_rad+(5*PI/6), _center_x, horizon_y, 0, _display_diameter);

  // Draw nose attitude horizontal lines
  int16_t line_offset = _display_diameter/8;
  int16_t line_width = _display_diameter/24;
  _draw_angled_offset_line(ctx, _horizon_angle_rad+PI, _center_x, horizon_y, line_width, -line_offset);
  _draw_angled_offset_line(ctx, _horizon_angle_rad+PI, _center_x, horizon_y, line_width*2, -line_offset*2);
  _draw_angled_offset_line(ctx, _horizon_angle_rad+PI, _center_x, horizon_y, line_width, line_offset);
  _draw_angled_offset_line(ctx, _horizon_angle_rad+PI, _center_x, horizon_y, line_width*2, line_offset*2);
  _draw_angled_offset_line(ctx, _horizon_angle_rad+PI, _center_x, horizon_y, line_width*3, -line_offset*3);


  _draw_angled_offset_line(ctx, _horizon_angle_rad, _center_x, horizon_y, line_width, line_offset);
  _draw_angled_offset_line(ctx, _horizon_angle_rad, _center_x, horizon_y, line_width*2, line_offset*2);
  _draw_angled_offset_line(ctx, _horizon_angle_rad, _center_x, horizon_y, line_width*3, line_offset*3);
  _draw_angled_offset_line(ctx, _horizon_angle_rad, _center_x, horizon_y, line_width, -line_offset);
  _draw_angled_offset_line(ctx, _horizon_angle_rad, _center_x, horizon_y, line_width*2, -line_offset*2);
  
}

static GPoint rotate_point(GPoint p, float angle_rad) {
  int16_t x_new = (int16_t)(p.x * cosf(angle_rad) - p.y * sinf(angle_rad));
  int16_t y_new = (int16_t)(p.x * sinf(angle_rad) + p.y * cosf(angle_rad));
  return GPoint(x_new + _center_x, y_new + _center_y);
}

static void _draw_filled_polygon(GContext *ctx, GPoint *points, int num_points) {
  // Zajistíme, že polygon má aspoň 3 body
  if(num_points < 3) return;

  GPathInfo polygon_info = {
    .num_points = num_points,
    .points = points
  };
  GPath *polygon_path = gpath_create(&polygon_info);

  graphics_context_set_fill_color(ctx, GColorBlack);
  gpath_draw_filled(ctx, polygon_path);

  graphics_context_set_stroke_color(ctx, GColorBlack);
  gpath_draw_outline(ctx, polygon_path);

  gpath_destroy(polygon_path);
}

static void _draw_crown(GContext *ctx) {
  int16_t base_y = _center_y - (_display_diameter / 2) + 17;

  GPoint crown_top = GPoint(0, base_y - _center_y);
  GPoint crown_left = GPoint(-5, crown_top.y + 10);
  GPoint crown_right = GPoint(5, crown_top.y + 10);

  GPoint rotated_top = rotate_point(crown_top, _horizon_angle_rad);
  GPoint rotated_left = rotate_point(crown_left, _horizon_angle_rad);
  GPoint rotated_right = rotate_point(crown_right, _horizon_angle_rad);
  
  graphics_draw_line(ctx, rotated_top, rotated_left);
  graphics_draw_line(ctx, rotated_top, rotated_right);
  graphics_draw_line(ctx, rotated_left, rotated_right);

  _draw_filled_polygon(ctx, (GPoint[]) {rotated_top, rotated_left, rotated_right}, 3);
}

static void _draw_border(GContext *ctx) {
  int16_t border_offset = _display_diameter/2 - _border_width;

  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, _border_width*2);
  graphics_draw_circle(ctx, GPoint(_center_x, _center_y), _display_diameter/2);

  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 100);
  graphics_draw_circle(ctx, GPoint(_center_x, _center_y), _display_diameter/2+50);
  
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_circle(ctx, GPoint(_center_x, _center_y), border_offset+1);

  // Draw border horizon
  graphics_context_set_stroke_width(ctx, 2);
  _draw_angled_line(ctx, _horizon_angle_rad, _center_x, _center_y, border_offset+2, _border_width);
  _draw_angled_line(ctx, _horizon_angle_rad+PI, _center_x, _center_y, border_offset+2, _border_width);

  // Draw border lines 
  graphics_context_set_stroke_width(ctx, 3);
  _draw_angled_line(ctx, 0, _center_x, _center_y, border_offset+3, _border_width*0.6);
  _draw_angled_line(ctx, PI, _center_x, _center_y, border_offset+3, _border_width*0.6);
  _draw_angled_line(ctx, PI/6*7, _center_x, _center_y, border_offset+3, _border_width*0.6);
  _draw_angled_line(ctx, PI/6*8, _center_x, _center_y, border_offset+3, _border_width*0.6);
  _draw_angled_line(ctx, PI/6*9, _center_x, _center_y, border_offset+3, _border_width*0.6);
  _draw_angled_line(ctx, PI/6*10, _center_x, _center_y, border_offset+3, _border_width*0.6);
  _draw_angled_line(ctx, PI/6*11, _center_x, _center_y, border_offset+3, _border_width*0.6);

  graphics_context_set_stroke_width(ctx, 2);
  _draw_angled_line(ctx, PI/18*25, _center_x, _center_y, border_offset+3, _border_width*0.4);
  _draw_angled_line(ctx, PI/18*26, _center_x, _center_y, border_offset+3, _border_width*0.4);
  _draw_angled_line(ctx, PI/18*28, _center_x, _center_y, border_offset+3, _border_width*0.4);
  _draw_angled_line(ctx, PI/18*29, _center_x, _center_y, border_offset+3, _border_width*0.4);

  // Draw pointer needle
  int16_t ratio=_display_diameter/65;

  GPoint needle[] = {
    GPoint(-10*ratio+_center_x,22*ratio+_center_y),
    GPoint(-1*ratio+_center_x,11*ratio+_center_y),
    GPoint(-1*ratio+_center_x,8*ratio+_center_y),
    GPoint(-5*ratio+_center_x,8*ratio+_center_y),
    GPoint(-7*ratio+_center_x,7*ratio+_center_y),
    GPoint(-8*ratio+_center_x,5*ratio+_center_y),
    GPoint(-8*ratio+_center_x,1*ratio+_center_y),
    GPoint(-17*ratio+_center_x,1*ratio+_center_y),
    GPoint(-17*ratio+_center_x,0*ratio+_center_y),
    GPoint(-7*ratio+_center_x,0*ratio+_center_y),
    GPoint(-7*ratio+_center_x,4*ratio+_center_y),
    GPoint(-6*ratio+_center_x,6*ratio+_center_y),
    GPoint(-4*ratio+_center_x,7*ratio+_center_y),
    GPoint(-1*ratio+_center_x,7*ratio+_center_y),
    GPoint(-1*ratio+_center_x,2*ratio+_center_y),
    GPoint(-2*ratio+_center_x,1*ratio+_center_y),
    GPoint(-2*ratio+_center_x,0*ratio+_center_y),
    GPoint(-1*ratio+_center_x,-1*ratio+_center_y),
    GPoint(0*ratio+_center_x,-1*ratio+_center_y),
    GPoint(1*ratio+_center_x,0*ratio+_center_y),
    GPoint(1*ratio+_center_x,1*ratio+_center_y),
    GPoint(0*ratio+_center_x,2*ratio+_center_y),
    GPoint(0*ratio+_center_x,7*ratio+_center_y),
    GPoint(3*ratio+_center_x,7*ratio+_center_y),
    GPoint(5*ratio+_center_x,6*ratio+_center_y),
    GPoint(6*ratio+_center_x,4*ratio+_center_y),
    GPoint(6*ratio+_center_x,0*ratio+_center_y),
    GPoint(16*ratio+_center_x,0*ratio+_center_y),
    GPoint(16*ratio+_center_x,1*ratio+_center_y),
    GPoint(7*ratio+_center_x,1*ratio+_center_y),
    GPoint(7*ratio+_center_x,5*ratio+_center_y),
    GPoint(6*ratio+_center_x,7*ratio+_center_y),
    GPoint(4*ratio+_center_x,8*ratio+_center_y),
    GPoint(0*ratio+_center_x,8*ratio+_center_y),
    GPoint(0*ratio+_center_x,11*ratio+_center_y),
    GPoint(9*ratio+_center_x,22*ratio+_center_y),
    GPoint(29*ratio+_center_x,22*ratio+_center_y),
    GPoint(29*ratio+_center_x,43*ratio+_center_y),
    GPoint(-30*ratio+_center_x,42*ratio+_center_y),
    GPoint(-30*ratio+_center_x,22*ratio+_center_y)
  };
  _draw_filled_polygon(ctx, needle, 40);
  
  // Draw around
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_circle(ctx, GPoint(_center_x, _center_y), _display_diameter/2+3);

  // Draw calibration knob
  graphics_draw_circle(ctx, GPoint(_center_x, _display_diameter*1.05), _display_diameter*0.08);

  // Draw bolts
  int16_t bolt_size = _display_diameter*0.1;

  int16_t bolt_x, bolt_y;
  bolt_x = _center_x - _display_diameter/2.1;
  bolt_y = _center_y - _display_diameter/2.1;
  graphics_draw_circle(ctx, GPoint(bolt_x, bolt_y), bolt_size/2+1);
  _draw_angled_line(ctx, PI*0.3, bolt_x, bolt_y, -bolt_size/2, bolt_size);

  bolt_x = _center_x + _display_diameter/2.1;
  bolt_y = _center_y - _display_diameter/2.1;
  graphics_draw_circle(ctx, GPoint(bolt_x, bolt_y), bolt_size/2+1);
  _draw_angled_line(ctx, PI*1.2, bolt_x, bolt_y, -bolt_size/2, bolt_size);

  bolt_x = _center_x - _display_diameter/2.1;
  bolt_y = _center_y + _display_diameter/2.1;
  graphics_draw_circle(ctx, GPoint(bolt_x, bolt_y), bolt_size/2+1);
  _draw_angled_line(ctx, PI*1.7, bolt_x, bolt_y, -bolt_size/2, bolt_size);

  bolt_x = _center_x + _display_diameter/2.1;
  bolt_y = _center_y + _display_diameter/2.1;
  graphics_draw_circle(ctx, GPoint(bolt_x, bolt_y), bolt_size/2+1);
  _draw_angled_line(ctx, PI*0.6, bolt_x, bolt_y, -bolt_size/2, bolt_size);



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
  light_enable(true);

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
  light_enable(false);
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
