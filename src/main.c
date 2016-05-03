#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static Layer *s_battery_layer;
static BitmapLayer *s_ship_layer;

static int s_battery_percent;

static GBitmap *ship_bitmap;

static void battery_update_proc(Layer *layer, GContext *ctx) 
{
  GRect bounds = layer_get_bounds(layer);

  int width = (int)(float)(((float)s_battery_percent / 100.0F) * bounds.size.w);

  // Draw the background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  
  // choose color based on remaining battery
  #ifdef PBL_COLOR
    if(s_battery_percent >= 60)
    {
      graphics_context_set_fill_color(ctx, GColorIslamicGreen);
    }
    else if(s_battery_percent > 30 && s_battery_percent < 60)
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
  //center.y += 20;
  
  ship_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ship_1);
  GSize image_size = gbitmap_get_bounds(ship_bitmap).size;
  GRect image_frame = GRect(center.x, center.y, image_size.w, image_size.h);
  image_frame.origin.y += 10;
  image_frame.origin.x -= image_size.w / 2;
  image_frame.origin.y -= image_size.h / 2;
  
  // Ship layer
  s_ship_layer = bitmap_layer_create(
      image_frame
  );
  bitmap_layer_set_background_color(s_ship_layer, GColorClear);
  bitmap_layer_set_compositing_mode(s_ship_layer, GCompOpSet);
  bitmap_layer_set_bitmap(s_ship_layer, ship_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_ship_layer));
  
  
  // Battery layer
  s_battery_layer = layer_create(GRect(image_frame.origin.x, image_frame.origin.y + image_size.h + 10, image_size.w, 8));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  layer_add_child(window_layer, s_battery_layer);
  
  
  // Time layer
  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(0, 0), bounds.size.w, 40)
  );

  text_layer_set_background_color(s_time_layer, GColorBlack);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
}

static void main_window_unload(Window *window) 
{

}

static void battery_callback(BatteryChargeState state) 
{
  s_battery_percent = state.charge_percent;
  layer_mark_dirty(s_battery_layer);
}

static void update_time() 
{
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), 
           clock_is_24h_style() ? "%H:%M" : "%I:%M", 
           tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) 
{
  update_time();
}

static void init() 
{
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack); // black BG
  
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  battery_state_service_subscribe(battery_callback);
  battery_callback(battery_state_service_peek());
  
  window_stack_push(s_main_window, true);
  update_time();
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