#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer, *s_date_layer;
static Layer *s_battery_layer;
static BitmapLayer *s_ship_layer, *s_bt_layer;

static int s_battery_level;

static GBitmap *s_ship_bitmap, *s_bt_bitmap;

static void bluetooth_callback(bool connected) 
{
  layer_set_hidden(bitmap_layer_get_layer(s_bt_layer), connected); // Show icon if disconnected

  if(!connected)
    vibes_double_pulse();
}

static void battery_callback(BatteryChargeState state) 
{
  s_battery_level = state.charge_percent;
  layer_mark_dirty(s_battery_layer);
}

static void update_time() 
{
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write hours and minutes
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), 
           clock_is_24h_style() ? "%H:%M" : "%I:%M", 
           tick_time);
  text_layer_set_text(s_time_layer, s_buffer);
  
  // Write date
  static char date_buffer[16];
  strftime(date_buffer, sizeof(date_buffer), "%a %d %b", tick_time);
  text_layer_set_text(s_date_layer, date_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) 
{
  update_time();
}

static void battery_update_proc(Layer *layer, GContext *ctx) 
{
  GRect bounds = layer_get_bounds(layer);

  float battery_percent = (float)s_battery_level;
  int width = (int)(((float)battery_percent / 100.0F) * bounds.size.w);
  
  // choose color based on remaining battery
  #ifdef PBL_COLOR
    if(battery_percent >= 60)
    {
      graphics_context_set_fill_color(ctx, GColorIslamicGreen);
    }
    else if(battery_percent > 30 && battery_percent < 60)
    {
      graphics_context_set_fill_color(ctx, GColorOrange);
    }
    else
    {
      graphics_context_set_fill_color(ctx, GColorRed);
    }
  #else
    graphics_context_set_fill_color(ctx, GColorWhite);
  #endif

  // Draw the bar    
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
  
  graphics_context_set_stroke_width(ctx, 1);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_rect(ctx, GRect(0, 0, bounds.size.w, bounds.size.h));
}

static void main_window_load(Window *window) 
{
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);  
  GPoint center = grect_center_point(&bounds);
  
  // Ship layer
  s_ship_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ship_1);
  GSize image_size = gbitmap_get_bounds(s_ship_bitmap).size;
  GRect image_frame = GRect(center.x, center.y, image_size.w, image_size.h);
  image_frame.origin.y += 10;
  image_frame.origin.x -= image_size.w / 2;
  image_frame.origin.y -= image_size.h / 2;
  s_ship_layer = bitmap_layer_create(image_frame);
  bitmap_layer_set_background_color(s_ship_layer, GColorClear);
  bitmap_layer_set_compositing_mode(s_ship_layer, GCompOpSet);
  bitmap_layer_set_bitmap(s_ship_layer, s_ship_bitmap);
  
  // Bluetooth layer
  s_bt_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BLUETOOTH_ICON);
  GSize bt_image_size = gbitmap_get_bounds(s_bt_bitmap).size;
  GRect bt_image_frame = GRect(center.x, center.y, bt_image_size.w, bt_image_size.h);
  bt_image_frame.origin.x -= bt_image_size.w / 2;
  bt_image_frame.origin.y -= bt_image_size.h / 2;
  bt_image_frame.origin.y += 25;
  s_bt_layer = bitmap_layer_create(bt_image_frame);
  bitmap_layer_set_background_color(s_bt_layer, GColorClear);
  bitmap_layer_set_compositing_mode(s_bt_layer, GCompOpSet);
  bitmap_layer_set_bitmap(s_bt_layer, s_bt_bitmap);
  
  // Battery layer
  s_battery_layer = layer_create(GRect(image_frame.origin.x, image_frame.origin.y + image_size.h + 10, image_size.w, 8));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  
  // Time layer
  s_time_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(0, 0), bounds.size.w, 40));
  text_layer_set_background_color(s_time_layer, GColorBlack);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  // Date layer
  s_date_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(35, 35), bounds.size.w, 15));
  text_layer_set_background_color(s_date_layer, GColorBlack);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_text(s_date_layer, "");
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  
  // create all layers
  layer_add_child(window_layer, bitmap_layer_get_layer(s_ship_layer));
  layer_add_child(window_layer, s_battery_layer);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bt_layer));
  
  update_time(); // time init
  battery_callback(battery_state_service_peek()); // battery init
  bluetooth_callback(connection_service_peek_pebble_app_connection()); //bluetooth init
  
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler); // time update
  battery_state_service_subscribe(battery_callback); // battery update
  connection_service_subscribe((ConnectionHandlers) {.pebble_app_connection_handler = bluetooth_callback}); // bluetooth update
}

static void main_window_unload(Window *window) 
{
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();
  
  layer_destroy(s_battery_layer);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  bitmap_layer_destroy(s_ship_layer);
  bitmap_layer_destroy(s_bt_layer);
}

static void init() 
{
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack); // black BG
  
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  window_stack_push(s_main_window, true);  
}

static void deinit() 
{
  window_destroy(s_main_window);
}

int main(void) 
{
  init();
  app_event_loop();
  deinit();
}