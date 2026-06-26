#include <pebble.h>

static Window *s_window;
static TextLayer *s_time_layer;
static TextLayer *s_second_layer;
static TextLayer *s_date_layer;

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char s_time_buffer[8];
  strftime(s_time_buffer,sizeof(s_time_buffer), clock_is_24h_style()? "%H:%M": "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, s_time_buffer);

  static char s_second_buffer[10];
  strftime(s_second_buffer,sizeof(s_second_buffer), "%S", tick_time);
  text_layer_set_text(s_second_layer, s_second_buffer);

  static char s_date_buffer[16];
  strftime(s_date_buffer, sizeof(s_date_buffer), "%a %m/%d", tick_time);
  text_layer_set_text(s_date_layer, s_date_buffer);
}


static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
  update_time();
}

static void setup_text_layer(TextLayer *text_layer, char *font) {
  text_layer_set_background_color(text_layer, GColorClear);
  text_layer_set_text_color(text_layer, GColorWhite);
  text_layer_set_font(text_layer, fonts_get_system_font(font));
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
}


static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_time_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(58,52), bounds.size.w, 50));
  s_second_layer = text_layer_create(GRect(0, 90, bounds.size.w, 50));
  s_date_layer = text_layer_create(GRect(0, 30, bounds.size.w, 50));

  setup_text_layer(s_time_layer, FONT_KEY_BITHAM_34_MEDIUM_NUMBERS);
  setup_text_layer(s_second_layer, FONT_KEY_BITHAM_42_BOLD);
  setup_text_layer(s_date_layer,FONT_KEY_GOTHIC_18_BOLD);


  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_second_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
}
static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_second_layer);
  text_layer_destroy(s_date_layer);
}


static void init(void) {
  s_window = window_create();

  window_set_background_color(s_window, GColorBlue);

  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  window_stack_push(s_window, true);
  update_time();
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

static void deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

  app_event_loop();
  deinit();
}
