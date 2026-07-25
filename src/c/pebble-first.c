#include "message_keys.auto.h"
#include <pebble.h>
#include <stdio.h>
#include <string.h>

#define SETTINGS_KEY 1

typedef struct ClaySettings {
  char ShowSeconds[20];
} ClaySettings;

static ClaySettings settings;

static void prv_default_settings() {
  snprintf(settings.ShowSeconds, sizeof(settings.ShowSeconds), "screenon");
}

static void prv_save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void prv_load_settings() {
  prv_default_settings();
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

//Layers to draw
static Window *s_window;
static TextLayer *s_time_layer;
static TextLayer *s_second_layer;
static TextLayer *s_date_layer;
static TextLayer *s_weather_layer;
static Layer *s_battery_layer;
static BitmapLayer *s_bt_icon_layer;

//Variables that will be updated with the event callbacks
static GBitmap *s_bt_icon_bitmap;
static int s_battery_level;
static bool is_charging;

// Callbacks & Hanlders
static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char s_time_buffer[8];
  strftime(s_time_buffer,sizeof(s_time_buffer), clock_is_24h_style()? "%H:%M": "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, s_time_buffer);

  static char s_date_buffer[16];
  strftime(s_date_buffer, sizeof(s_date_buffer), "%a %m/%d", tick_time);
  text_layer_set_text(s_date_layer, s_date_buffer);

  //Only show seconds when the light is on (Saves battery)
  if (strcmp(settings.ShowSeconds,"always") == 0 || (strcmp(settings.ShowSeconds,"screenon") == 0 && light_is_on())){
    static char s_second_buffer[3];
    strftime(s_second_buffer,sizeof(s_second_buffer), "%S", tick_time);
    text_layer_set_text(s_second_layer, s_second_buffer);
  } else {
    text_layer_set_text(s_second_layer, "");
  }
}
static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
  update_time();

  if (tick_time->tm_min % 15 == 0) {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter, MESSAGE_KEY_REQUEST_WEATHER, 1);
    app_message_outbox_send();
  }
}

static void battery_callback(BatteryChargeState batteryState) {
  s_battery_level = batteryState.charge_percent;
  is_charging = batteryState.is_charging;

  layer_mark_dirty(s_battery_layer);
}

static void bt_callback(bool connected) {
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);
}

static void battery_update_proc(Layer *layer, GContext *context){
  GRect bounds = layer_get_bounds(layer);

  int bar_width = ((s_battery_level * (bounds.size.w -4))/100);

  graphics_context_set_stroke_color(context, GColorWhite);
  graphics_draw_round_rect(context,bounds, 2);

  GColor bar_color;

  if(is_charging) {
    bar_color = PBL_IF_COLOR_ELSE(GColorBrightGreen, GColorWhite);
  }
  if(s_battery_level <= 20) {
    bar_color = PBL_IF_COLOR_ELSE(GColorRed, GColorWhite);
  } else if(s_battery_level < 60) {
    bar_color = PBL_IF_COLOR_ELSE(GColorYellow, GColorWhite);
  } else {
    bar_color = PBL_IF_COLOR_ELSE(GColorGreen, GColorWhite);
  }

  graphics_context_set_fill_color(context, bar_color);
  graphics_fill_rect(context, GRect(2,2,bar_width, bounds.size.h - 4), 1, GCornerNone);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  //Temperature changes
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);

  if (temp_tuple && conditions_tuple) {
    static char temperature_buffer[8];
    static char conditions_buffer[32];
    static char weather_layer_buffer[42];

    //Get your dirty freedom units out of here
    snprintf(temperature_buffer,sizeof(temperature_buffer), "%d°C", (int) temp_tuple->value->int32);
    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s %s", temperature_buffer, conditions_buffer);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
    //we're spoiled on other high level languages by not having to do all of this to copy strings over
  }

  //Settings changes
  Tuple *show_seconds = dict_find(iterator, MESSAGE_KEY_ShowSeconds);
  if(show_seconds) {
    snprintf(settings.ShowSeconds, sizeof(settings.ShowSeconds), "%s", show_seconds->value->cstring);
  }
}
static void inbox_dropped_callback(AppMessageResult reason, void *context){
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped");
}
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason,void *context){
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed");
}
static void outbox_sent_callback(DictionaryIterator *iterator, void *context){
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send success");
}

//Text layer reusable setup
static void setup_text_layer(TextLayer *text_layer, char *font) {
  text_layer_set_background_color(text_layer, GColorClear);
  text_layer_set_text_color(text_layer, GColorWhite);
  text_layer_set_font(text_layer, fonts_get_system_font(font));
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
}


//Lifecycle
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Set dimensions
  int bar_width = bounds.size.w / 2;
  int bar_x = (bounds.size.w - bar_width) / 2;
  int bar_y = PBL_IF_ROUND_ELSE(bounds.size.h / 8, bounds.size.h / 28);
  int bt_y = bar_y + 12;

  s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);

  //Create all layers
  s_time_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(58,52), bounds.size.w, 50));
  s_second_layer = text_layer_create(GRect(0, 90, bounds.size.w, 50));
  s_date_layer = text_layer_create(GRect(0, 135, bounds.size.w, 50));
  s_weather_layer = text_layer_create(GRect(0, 150, bounds.size.w, 25));

  s_battery_layer = layer_create(GRect(bar_x, bar_y, bar_width, 8));
  s_bt_icon_layer = bitmap_layer_create(GRect((bounds.size.w - 30)/2, bt_y, 30, 30));

  //Setup all text layers
  setup_text_layer(s_time_layer, FONT_KEY_BITHAM_34_MEDIUM_NUMBERS);
  setup_text_layer(s_second_layer, FONT_KEY_BITHAM_42_BOLD);
  setup_text_layer(s_date_layer, FONT_KEY_GOTHIC_18_BOLD);
  setup_text_layer(s_weather_layer, FONT_KEY_GOTHIC_18_BOLD);

  text_layer_set_text(s_weather_layer, "Fetching...");

  //Update the layer based on the battery readings
  layer_set_update_proc(s_battery_layer, battery_update_proc);

  //Bitmap layer (bt icon) setup
  bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
  bitmap_layer_set_compositing_mode(s_bt_icon_layer, GCompOpSet);


  //Add all layers to the main window
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_second_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));
  layer_add_child(window_layer, s_battery_layer);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bt_icon_layer));

}
static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_second_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_weather_layer);
  layer_destroy(s_battery_layer);
}


static void init(void) {
  prv_load_settings();
  s_window = window_create();

  window_set_background_color(s_window, GColorBlue);

  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  window_stack_push(s_window, true);
  update_time();
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);

  battery_state_service_subscribe(battery_callback);

  battery_callback(battery_state_service_peek());

  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bt_callback
  });

  bt_callback(connection_service_peek_pebble_app_connection());

  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  const int inbox_size =  256;
  const int outbox_size =  256;
  app_message_open(inbox_size,outbox_size);
}

static void deinit(void) {
  window_destroy(s_window);
}

//Main
int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

  app_event_loop();
  deinit();
}
