#include <pebble.h>
#include "main.h"
// ancho pantalla = 145 alto=160
//fonts: https://gist.github.com/sarfata/aec9c06ba9d66d159ed4
//Controles
static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_seconds_layer;
static TextLayer *s_dayWeek_layer;
static TextLayer *s_fecha_layer;
static TextLayer *s_battery_layer;
static TextLayer *s_bluetooth_layer;
//Layer grafico
static Layer *s_path_layer;
//Variables
static bool s_bSegundos = true; //-1 --> true
static char battery_text[] = "100%";
//Fuentes
static GFont s_time_font;
//Bitmaps
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;
//Graphs
static GPath *s_my_path_ptr = NULL;
static GPath *s_linea_horizontal_ptr = NULL;
static GPath *s_linea_vertical_ptr = NULL;
static const GPathInfo PATH_LINEA_HORZ = {
  .num_points = 4,
  .points = (GPoint []) {{0, 140}, {145, 140}, {145, 140+3}, {0, 140+3}} //recta
};
static const GPathInfo PATH_LINEA_VERT = {
  .num_points = 4,
  .points = (GPoint []) {{73, 90}, {73, 140}, {73+3, 140}, {73+3, 90}} //recta
};


//CONTROLADOR BATERÍA
static void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "100%";

  if (charge_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "C...");
  } else {
    snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
  }
  text_layer_set_text(s_battery_layer, battery_text);
  text_layer_set_background_color(s_battery_layer, charge_state.charge_percent > 20 ? GColorClear : GColorFromHEX(0xFF0000));
}  
//CONTROLADOR BLUETOOTH
static void handle_bluetooth(bool connected) {
  text_layer_set_text(s_bluetooth_layer, connected ? "OK" : "ER");
  text_layer_set_background_color(s_bluetooth_layer, connected ? GColorFromHEX(0x55FFAA) : GColorFromHEX(0xFF0000));
}
//CONTROLADOR TIMER
static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?"%H:%M" : "%I:%M", tick_time);
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
  
  //Segundos
  if (s_bSegundos){  
    static char s_SecsBuffer[3];
    strftime(s_SecsBuffer, sizeof(s_SecsBuffer), "%S", tick_time);
    text_layer_set_text(s_seconds_layer, s_SecsBuffer);
  }  
  //Dia Semana
  static char s_DiaSemana[]="XXX";
  strftime(s_DiaSemana, sizeof(s_DiaSemana), "%a", tick_time);
  text_layer_set_text(s_dayWeek_layer, s_DiaSemana);
  
  //Fecha
  static char s_DiaMesAnno[]="10/12/2015";
  strftime(s_DiaMesAnno, sizeof(s_DiaMesAnno), "%d/%m/20%y", tick_time);
  text_layer_set_text(s_fecha_layer, s_DiaMesAnno);
}
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}
// DIBUJO
void my_layer_update_proc(Layer *my_layer, GContext* ctx) {
  // Fill the path:
  graphics_context_set_fill_color(ctx, GColorBlack ); //GColorWhite
  gpath_draw_filled(ctx, s_linea_horizontal_ptr);
  gpath_draw_filled(ctx, s_linea_vertical_ptr);
  // Stroke the path:
  graphics_context_set_stroke_color(ctx, GColorBlack);
  gpath_draw_outline(ctx, s_linea_horizontal_ptr);
  gpath_draw_outline(ctx, s_linea_vertical_ptr);
}
  

void setup_my_path(void) {
  s_linea_horizontal_ptr = gpath_create(&PATH_LINEA_HORZ);
  s_linea_vertical_ptr   = gpath_create(&PATH_LINEA_VERT);
  // Rotate 15 degrees:
  //gpath_rotate_to(s_linea_horizontal_ptr, TRIG_MAX_ANGLE / 360 * 15);
  // Translate by (5, 5):
  //gpath_move_to(s_linea_horizontal_ptr, GPoint(5, 5));
}
static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds  = layer_get_bounds(window_layer);
    
  
  //BATERIA
  int intAnchoBat = bounds.size.w/2 + 25;
  int intMrg = 2;
  s_battery_layer = text_layer_create(GRect(intMrg, PBL_IF_ROUND_ELSE(10, 8), intAnchoBat, 40));
  text_layer_set_background_color(s_battery_layer, GColorClear );
  text_layer_set_text_color(s_battery_layer, GColorBlack); 
  text_layer_set_text(s_battery_layer, "---%");
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer)); 
  
  //BLUETOOTH
  s_bluetooth_layer = text_layer_create(GRect(intMrg +intAnchoBat, PBL_IF_ROUND_ELSE(10, 8), bounds.size.w-intAnchoBat-2*intMrg, 40));
  text_layer_set_background_color(s_bluetooth_layer, GColorFromHEX(0x55FFAA));
  text_layer_set_text_color(s_bluetooth_layer, GColorBlack);
  text_layer_set_text(s_bluetooth_layer, "OK");
  text_layer_set_font(s_bluetooth_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_bluetooth_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_bluetooth_layer));

  // HORA 
  s_time_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(60, 54), bounds.size.w, 55));
  text_layer_set_background_color(s_time_layer, GColorBlack );
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  //Segundos
  if (s_bSegundos){
    s_seconds_layer = text_layer_create(GRect( bounds.size.w/2 +2 , PBL_IF_ROUND_ELSE(60, 54)+52, bounds.size.w/2-2, 50));
    text_layer_set_background_color(s_seconds_layer, GColorClear);
    text_layer_set_text_color(s_seconds_layer, GColorBlack);
    text_layer_set_text(s_seconds_layer, "00");
    text_layer_set_font(s_seconds_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
    text_layer_set_text_alignment(s_seconds_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_seconds_layer));
  }
  
  //DIA SEMANA
  s_dayWeek_layer = text_layer_create(GRect( 2 , PBL_IF_ROUND_ELSE(60, 54)+52, bounds.size.w/2-2, 50));
  text_layer_set_background_color(s_dayWeek_layer, GColorClear);
  text_layer_set_text_color(s_dayWeek_layer, GColorBlack);
  text_layer_set_text(s_dayWeek_layer, "Mon");
  text_layer_set_font(s_dayWeek_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_dayWeek_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_dayWeek_layer));

  //FECHA
  s_fecha_layer = text_layer_create(GRect( 2 , PBL_IF_ROUND_ELSE(145, 145-6), bounds.size.w-2, 50));
  text_layer_set_background_color(s_fecha_layer, GColorClear);
  text_layer_set_text_color(s_fecha_layer, GColorBlack);
  text_layer_set_text(s_fecha_layer, "Mon");
  text_layer_set_font(s_fecha_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_fecha_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_fecha_layer));
  
  //Layer de Graficos
  s_path_layer = layer_create(GRect(0, 0, 144, 168)); 
  layer_set_update_proc(s_path_layer, my_layer_update_proc);
  layer_add_child(window_layer, s_path_layer);
  //Alta de lineas
  setup_my_path();
  
  
  // Register with TickTimerService
  tick_timer_service_subscribe(s_bSegundos ? SECOND_UNIT: MINUTE_UNIT, tick_handler);
  //Control de batería
  battery_state_service_subscribe(handle_battery);
  //Control de bluetooth
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = handle_bluetooth
  });
  
}

static void main_window_unload(Window *window) {
  //Cierro servicios
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();
  
  // Destroy TextLayer
  text_layer_destroy(s_battery_layer);
  text_layer_destroy(s_bluetooth_layer);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_seconds_layer);
  text_layer_destroy(s_dayWeek_layer);
  text_layer_destroy(s_fecha_layer);
  
  layer_destroy(s_path_layer);
  //Destruyo Dibujo
  gpath_destroy(s_linea_horizontal_ptr);
  gpath_destroy(s_linea_vertical_ptr);
}

static void init() {
 // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load   = main_window_load,
    .unload = main_window_unload
  });

  
  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Make sure the time is displayed from the start
  update_time();
}
static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

