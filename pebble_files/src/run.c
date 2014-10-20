/*
 * pebble pedometer
 * @author jathusant
 * @author Roger, Harnek, Hazma, Erin
 */

#include <pebble.h>
#include <run.h>
#include <math.h>

// Total Steps (TS)
#define TS 1
// Total Steps Default (TSD)
#define TSD 1

static Window *window;
static Window *menu_window;
static Window *set_stepGoal;
static Window *pedometer;
static Window *dev_info;

static SimpleMenuLayer *pedometer_settings;
static SimpleMenuItem menu_items[8];
static SimpleMenuSection menu_sections[1];
ActionBarLayer *stepGoalSetter;

// Menu Item names and subtitles
char *item_names[8] = { "Start", "Step Goal", "Overall Steps",
		"Overall Calories", "Sensitivity", "Theme", "Version", "About" };
char *item_sub[8] = { "Lets Exercise!", "Not Set", "0 in Total", "0 Burned",
		"", "", "v0.3-RELEASE", "Us" };

// Timer used to determine next step check
static AppTimer *timer;

// Text Layers
TextLayer *main_message;
TextLayer *main_message2;
TextLayer *hitBack;
TextLayer *stepGoalView;
TextLayer *pedCount;
TextLayer *infor;
TextLayer *calories;
TextLayer *stepGoalText;
TextLayer *street;
TextLayer *meters;
TextLayer *meterskm;
TextLayer *text_layer;



// Bitmap Layers
static GBitmap *btn_dwn;
static GBitmap *btn_up;
static GBitmap *btn_sel;
static GBitmap *statusBar;
GBitmap *arrow_left;
GBitmap *arrow_right;
BitmapLayer *arrow_layer;
GBitmap *pedometerBack;
BitmapLayer *pedometerBack_layer;
GBitmap *splash;
BitmapLayer *splash_layer;
GBitmap *flame;
BitmapLayer *flame_layer;

// interval to check for next step (in ms)
const int ACCEL_STEP_MS = 475;
// value to auto adjust step acceptance 
const int PED_ADJUST = 2;
// steps required per calorie
const int STEPS_PER_CALORIE = 22;
// value by which step goal is incremented
const int STEP_INCREMENT = 50;
// values for max/min number of calibration options 
const int MAX_CALIBRATION_SETTINGS = 3;
const int MIN_CALIBRATION_SETTINGS = 1;

int X_DELTA = 35;
int Y_DELTA, Z_DELTA = 185;
int YZ_DELTA_MIN = 175;
int YZ_DELTA_MAX = 195; 
int X_DELTA_TEMP, Y_DELTA_TEMP, Z_DELTA_TEMP = 0;
int lastX, lastY, lastZ, currX, currY, currZ = 0;
int sensitivity = 1;

long stepGoal = 0;
long pedometerCount = 0;
long caloriesBurned = 0;
long tempTotal = 0;

bool did_pebble_vibrate = false;
bool validX, validY, validZ = false;
bool SID;
bool isDark;
bool startedSession = false;

// Strings used to display theme and calibration options
char *theme;
char *cal = "Regular Sensitivity";

// stores total steps since app install
static long totalSteps = TSD;

TextLayer* big_time_layer;
TextLayer* seconds_time_layer;

// Actually keeping track of time
double elapsed_time = 0;
bool started = false;
AppTimer* update_timer = NULL;

#define TIMER_UPDATE 1
double start_time = 0;
double pause_time = 0;
int busy_animating = 0;
#define BUTTON_LAP BUTTON_ID_DOWN
#define BUTTON_RUN BUTTON_ID_SELECT
#define BUTTON_RESET BUTTON_ID_UP

void toggle_stopwatch_handler(ClickRecognizerRef recognizer, Window *window);
void config_provider(Window *window);
void handle_init();
time_t time_seconds();
void stop_stopwatch();
void start_stopwatch();
void toggle_stopwatch_handler(ClickRecognizerRef recognizer, Window *window);
void reset_stopwatch_handler(ClickRecognizerRef recognizer, Window *window);
void update_stopwatch();
void handle_timer(void* data);
int main();
  
  
struct StopwatchState {
	bool started;
	double elapsed_time;
	double start_time;
	double pause_time;
	double last_lap_time;
} __attribute__((__packed__));  
	
TextLayer *text_layer;
int indexload;
int dirindex;
char str[75];

typedef struct lor
  {
  bool name;
} lor;


typedef struct dist
  {
  char name[5];
}dist;


typedef struct loc
  {
  char name[20];
}loc;
lor lors [15];
dist dists [15];
loc locs [15];

// Key values for AppMessage Dictionary
enum {
	STATUS_KEY = 0,	
	MESSAGE_KEY = 1,
  DIRECTION_KEY = 2
};

// Write message to buffer & send
void send_message(void){
	DictionaryIterator *iter;
	
	app_message_outbox_begin(&iter);
	dict_write_uint8(iter, STATUS_KEY, 0x1);
	
	dict_write_end(iter);
  	app_message_outbox_send();
}

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
	Tuple *tuple;
  
  tuple = dict_find(received, STATUS_KEY);
  if (tuple){
    char sup [5];
    snprintf(dists[indexload].name, 6, "%s", tuple->value->cstring);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Message:12");
  }
  
	tuple = dict_find(received, MESSAGE_KEY);
	if(tuple) {
    char sup [30]; 
    snprintf(locs[indexload].name, 30, "%s", tuple->value->cstring);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Message:120");
	}
  tuple = dict_find(received, DIRECTION_KEY);
  if (tuple){
    if ((int)(tuple->value->uint32) == 1)
      {
      lors[indexload].name = true;
      char* buffer = "Right";
    }
    else
      {
      lors[indexload].name = false;
      char* buffer = "Left";
    }
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Message:1220");
    
  }
  char buffer [20];
  snprintf(buffer, 20, "Loaded: %d", indexload);
  text_layer_set_text(text_layer, buffer); 
  indexload = indexload + 1;
}

// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {	
  text_layer_set_text(text_layer, "we dun fucked");

}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
}



void start_callback(int index, void *ctx) {
	accel_data_service_subscribe(0, NULL);

	menu_items[0].title = "Continue Run";
	menu_items[0].subtitle = "Ready for more?";
	layer_mark_dirty(simple_menu_layer_get_layer(pedometer_settings));

	pedometer = window_create();

	window_set_window_handlers(pedometer, (WindowHandlers ) { .load = ped_load,
					.unload = ped_unload, });

	window_stack_push(pedometer, true);
	timer = app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
}

void info_callback(int index, void *ctx) {
	dev_info = window_create();

	window_set_window_handlers(dev_info, (WindowHandlers ) { .load = info_load,
					.unload = info_unload, });

	window_stack_push(dev_info, true);
}

void stepGoal_callback(int index, void *ctx) {
	set_stepGoal = window_create();

	window_set_window_handlers(set_stepGoal, (WindowHandlers ) { .load =
					stepGoal_load, .unload = stepGoal_unload, });

	window_stack_push(set_stepGoal, true);

	static char buf[] = "1234567890";
	snprintf(buf, sizeof(buf), "%ld", stepGoal);

	if (stepGoal != 0) {
		menu_items[1].subtitle = buf;
	}
	layer_mark_dirty(simple_menu_layer_get_layer(pedometer_settings));
}

void calibration_callback(int index, void *ctx) {
	
	if (sensitivity >= MIN_CALIBRATION_SETTINGS && sensitivity < MAX_CALIBRATION_SETTINGS){
		sensitivity++;
	} else if (sensitivity == MAX_CALIBRATION_SETTINGS) {
		sensitivity = MIN_CALIBRATION_SETTINGS;
	}

	cal = determineCal(sensitivity);
	
	menu_items[4].subtitle = cal;
	layer_mark_dirty(simple_menu_layer_get_layer(pedometer_settings));
}

void theme_callback(int index, void *ctx) {
	if (isDark) {
		isDark = false;
		theme = "Light";
	} else {
		isDark = true;
		theme = "Dark";
	}

	char* new_string;
	new_string = malloc(strlen(theme) + 10);
	strcpy(new_string, "Current: ");
	strcat(new_string, theme);
	menu_items[5].subtitle = new_string;

	layer_mark_dirty(simple_menu_layer_get_layer(pedometer_settings));
}

char* determineCal(int cal){
	switch(cal){
		case 2:
		X_DELTA = 45;
		Y_DELTA = 235;
		Z_DELTA = 235;
		YZ_DELTA_MIN = 225;
		YZ_DELTA_MAX = 245; 
		return "Not Sensitive";
		case 3:
		X_DELTA = 25;
		Y_DELTA = 110;
		Z_DELTA = 110;
		YZ_DELTA_MIN = 100;
		YZ_DELTA_MAX = 120; 
		return "Very Sensitive";
		default:
		X_DELTA = 35;
		Y_DELTA = 185;
		Z_DELTA = 185;
		YZ_DELTA_MIN = 175;
		YZ_DELTA_MAX = 195; 
		return "Regular Sensitivity";
	}
}
void changeFontToFit() {
	if (stepGoal > 99900) {
		text_layer_set_font(stepGoalView, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BEBAS_30)));
	} else {
		text_layer_set_font(stepGoalView, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BEBAS_35)));
	}
}

void inc_click_handler(ClickRecognizerRef recognizer, void *context) {
	stepGoal += STEP_INCREMENT;
	static char buf[] = "123456";
	snprintf(buf, sizeof(buf), "%ld", stepGoal);
	text_layer_set_text(stepGoalView, buf);

	changeFontToFit();

	if (stepGoal != 0) {
		menu_items[1].subtitle = buf;
	} else {
		menu_items[1].subtitle = "Not Set";
	}
	layer_mark_dirty(simple_menu_layer_get_layer(pedometer_settings));
}

void dec_click_handler(ClickRecognizerRef recognizer, void *context) {
	if (stepGoal >= STEP_INCREMENT) {
		stepGoal -= STEP_INCREMENT;
		static char buf[] = "123456";
		snprintf(buf, sizeof(buf), "%ld", stepGoal);
		text_layer_set_text(stepGoalView, buf);

		changeFontToFit();

		if (stepGoal != 0) {
			menu_items[1].subtitle = buf;
		} else {
			menu_items[1].subtitle = "Not Set";
		}
		layer_mark_dirty(simple_menu_layer_get_layer(pedometer_settings));
	}
}

void set_click_handler(ClickRecognizerRef recognizer, void *context) {
	window_stack_pop(true);
}

void goal_set_click_config(void *context) {
	const uint16_t rep_interval = 50;

	window_single_repeating_click_subscribe(BUTTON_ID_DOWN, rep_interval,
			(ClickHandler) dec_click_handler);
	window_single_repeating_click_subscribe(BUTTON_ID_UP, rep_interval,
			(ClickHandler) inc_click_handler);
	window_single_click_subscribe(BUTTON_ID_SELECT,
			(ClickHandler) set_click_handler);
}

void setup_menu_items() {
	static char buf[] = "1234567890abcdefg";
	snprintf(buf, sizeof(buf), "%ld in Total", totalSteps);

	static char buf2[] = "1234567890abcdefg";
	snprintf(buf2, sizeof(buf2), "%ld Burned",
			(long) (totalSteps / STEPS_PER_CALORIE));

	for (int i = 0; i < (int) (sizeof(item_names) / sizeof(item_names[0]));
			i++) {
		menu_items[i] = (SimpleMenuItem ) { .title = item_names[i], .subtitle =
						item_sub[i], };

		//Setting Callbacks
		if (i == 0) {
			menu_items[i].callback = start_callback;
		} else if (i == 1) {
			menu_items[i].callback = stepGoal_callback;
		} else if (i == 2) {
			menu_items[i].subtitle = buf;
		} else if (i == 3) {
			menu_items[i].subtitle = buf2;
		} else if (i ==4){
			menu_items[i].subtitle = determineCal(sensitivity);
			menu_items[i].callback = calibration_callback;
		} else if (i == 5) {
			menu_items[i].subtitle = theme;
			menu_items[i].callback = theme_callback;
		} else if (i == 6 || i == 7) {
			menu_items[i].callback = info_callback;
		}
	}
}

void setup_menu_sections() {
	menu_sections[0] = (SimpleMenuSection ) { .items = menu_items, .num_items =
					sizeof(menu_items) / sizeof(menu_items[0]) };
}

void setup_menu_window() {
	menu_window = window_create();

	window_set_window_handlers(menu_window, (WindowHandlers ) { .load =
					settings_load, .unload = settings_unload, });
}

void stepGoal_load(Window *window) {
	stepGoalSetter = action_bar_layer_create();

	action_bar_layer_add_to_window(stepGoalSetter, set_stepGoal);
	action_bar_layer_set_click_config_provider(stepGoalSetter,
			goal_set_click_config);

	btn_dwn = gbitmap_create_with_resource(RESOURCE_ID_BTN_DOWN);
	btn_up = gbitmap_create_with_resource(RESOURCE_ID_BTN_UP);
	btn_sel = gbitmap_create_with_resource(RESOURCE_ID_BTN_SETUP);

	action_bar_layer_set_icon(stepGoalSetter, BUTTON_ID_UP, btn_up);
	action_bar_layer_set_icon(stepGoalSetter, BUTTON_ID_DOWN, btn_dwn);
	action_bar_layer_set_icon(stepGoalSetter, BUTTON_ID_SELECT, btn_sel);

	stepGoalText = text_layer_create(GRect(5, 10, 150, 150));
	stepGoalView = text_layer_create(GRect(10, 60, 150, 150));
	
	text_layer_set_background_color(stepGoalView, GColorClear);
	text_layer_set_background_color(stepGoalText, GColorClear);
	
	text_layer_set_font(stepGoalView, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BEBAS_35)));
	text_layer_set_font(stepGoalText,
			fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	
	layer_add_child(window_get_root_layer(set_stepGoal),
			(Layer*) stepGoalView);
	layer_add_child(window_get_root_layer(set_stepGoal), (Layer*) stepGoalText);

	static char buf[] = "123456";
	snprintf(buf, sizeof(buf), "%ld", stepGoal);
	text_layer_set_text(stepGoalView, buf);
	text_layer_set_text(stepGoalText, "Current Goal:");

	if (isDark) {
		window_set_background_color(set_stepGoal, GColorBlack);
		text_layer_set_text_color(stepGoalView, GColorWhite);
		text_layer_set_text_color(stepGoalText, GColorWhite);
		action_bar_layer_set_background_color(stepGoalSetter, GColorWhite);
	} else {
		window_set_background_color(set_stepGoal, GColorWhite);
		text_layer_set_text_color(stepGoalView, GColorBlack);
		text_layer_set_text_color(stepGoalText, GColorBlack);
		action_bar_layer_set_background_color(stepGoalSetter, GColorBlack);
	}
}

void stepGoal_unload(Window *window) {
	window_destroy(set_stepGoal);
	action_bar_layer_destroy(stepGoalSetter);
	gbitmap_destroy(btn_up);
	gbitmap_destroy(btn_dwn);
	gbitmap_destroy(btn_sel);
	text_layer_destroy(stepGoalView);
}

void settings_load(Window *window) {
	Layer *layer = window_get_root_layer(menu_window);
	statusBar = gbitmap_create_with_resource(RESOURCE_ID_STATUS_BAR);

	pedometer_settings = simple_menu_layer_create(layer_get_bounds(layer),
			menu_window, menu_sections, 1, NULL);
	simple_menu_layer_set_selected_index(pedometer_settings, 0, true);
	layer_add_child(layer, simple_menu_layer_get_layer(pedometer_settings));
	window_set_status_bar_icon(menu_window, statusBar);
}

void settings_unload(Window *window) {
	layer_destroy(window_get_root_layer(menu_window));
	simple_menu_layer_destroy(pedometer_settings);
	window_destroy(menu_window);
}

void ped_load(Window *window) {
	//steps = text_layer_create(GRect(0, 120, 150, 170));
	//pedCount = text_layer_create(GRect(0, 75, 150, 170));
	//calories = text_layer_create(GRect(0, 50, 150, 170));
      //  steps = text_layer_create(GRect(72, 94, 40, 18));
        calories = text_layer_create(GRect(5, 108, 45, 34));
        pedCount = text_layer_create(GRect(93, 108, 45, 34));
        //arrow = text_layer_create(GRect(0, 5, 50, 50));
        meters = text_layer_create(GRect(100, 5, 50, 50));
        meterskm = text_layer_create(GRect(100, 35, 60, 20));
      street = text_layer_create(GRect(40, 5, 70, 120));  
     text_layer = text_layer_create(GRect(40,95,50,30));         
  app_message_register_inbox_received(in_received_handler); 
  app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_failed(out_failed_handler);
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	dirindex = 0;
  indexload = 0;
  
  text_layer_set_text(text_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BEBAS_15))); 
   text_layer_set_text(text_layer, "Waiting for loading...");

    window_set_click_config_provider(window, (ClickConfigProvider) config_provider);  
  
  	big_time_layer = text_layer_create(GRect(10, 55, 76, 35));
    text_layer_set_background_color(big_time_layer, GColorWhite);
    text_layer_set_font(big_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BEBAS_30)));
    text_layer_set_text_color(big_time_layer, GColorBlack);
    text_layer_set_text(big_time_layer, "00:00");
    text_layer_set_text_alignment(big_time_layer, GTextAlignmentCenter);
    layer_add_child(window_get_root_layer(pedometer), (Layer*)big_time_layer);
  
    seconds_time_layer = text_layer_create(GRect(86, 55, 40, 35));
    text_layer_set_background_color(seconds_time_layer, GColorWhite);
    text_layer_set_font(seconds_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BEBAS_30)));
    text_layer_set_text_color(seconds_time_layer, GColorBlack);
    text_layer_set_text(seconds_time_layer, ".0");
    layer_add_child(window_get_root_layer(pedometer), (Layer*)seconds_time_layer);
  struct StopwatchState state;
		started = state.started;
		start_time = state.start_time;
		elapsed_time = state.elapsed_time;
		pause_time = state.pause_time;
		update_stopwatch();
		if(started) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Started!!");
			update_timer = app_timer_register(100, handle_timer, NULL);
			APP_LOG(APP_LOG_LEVEL_DEBUG, "Started timer to resume persisted state.");
		}
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Loaded persisted state.");
	
  
  
	if (isDark) {
		//window_set_background_color(pedometer, GColorBlack);
                window_set_background_color(pedometer, GColorWhite);
		//pedometerBack = gbitmap_create_with_resource(RESOURCE_ID_PED_WHITE);
    //flame = gbitmap_create_with_resource(RESOURCE_ID_FLAME_WHITE);
		//text_layer_set_background_color(steps, GColorClear);
		//text_layer_set_text_color(steps, GColorBlack);
		text_layer_set_background_color(pedCount, GColorClear);
		text_layer_set_text_color(pedCount, GColorBlack);
		text_layer_set_background_color(calories, GColorClear);
		text_layer_set_text_color(calories, GColorBlack);
        //text_layer_set_background_color(arrow, GColorClear);
        //text_layer_set_text_color(arrow, GColorBlack);
                text_layer_set_background_color(meters, GColorClear);
                text_layer_set_text_color(meters, GColorBlack);
    text_layer_set_background_color(meterskm, GColorClear);
                text_layer_set_text_color(meterskm, GColorBlack);
                text_layer_set_background_color(street, GColorClear);
                text_layer_set_text_color(street, GColorBlack);
	} else {
		window_set_background_color(pedometer, GColorBlack);
		//pedometerBack = gbitmap_create_with_resource(RESOURCE_ID_PED_BLK);
		//flame = gbitmap_create_with_resource(RESOURCE_ID_FLAME_BLK);
//		text_layer_set_background_color(steps, GColorClear);
	//	text_layer_set_text_color(steps, GColorBlack);
		text_layer_set_background_color(pedCount, GColorClear);
		text_layer_set_text_color(pedCount, GColorWhite);
		text_layer_set_background_color(calories, GColorClear);
		text_layer_set_text_color(calories, GColorWhite);
	    //text_layer_set_background_color(arrow, GColorClear);
	    //text_layer_set_text_color(arrow, GColorWhite);
		text_layer_set_background_color(meters, GColorClear);
		text_layer_set_text_color(meters, GColorWhite);
    text_layer_set_background_color(meterskm, GColorClear);
		text_layer_set_text_color(meterskm, GColorWhite);
		text_layer_set_background_color(street, GColorClear);
		text_layer_set_text_color(street, GColorWhite);
        }
  
  arrow_left = gbitmap_create_with_resource(RESOURCE_ID_ARROW_LEFT);
  arrow_right = gbitmap_create_with_resource(RESOURCE_ID_ARROW_RIGHT);

	//pedometerBack_layer = bitmap_layer_create(GRect(0, 0, 145, 215));
	//flame_layer = bitmap_layer_create(GRect(50, 0, 50, 50));
  arrow_layer = bitmap_layer_create(GRect(-5,0,50,50));
  bitmap_layer_set_bitmap(arrow_layer, arrow_left);
  layer_add_child(window_get_root_layer(pedometer), 
                  bitmap_layer_get_layer(arrow_layer)); 
  layer_add_child(window_get_root_layer(pedometer),
                 text_layer_get_layer(text_layer));
  //	bitmap_layer_set_bitmap(pedometerBack_layer, pedometerBack);
	//bitmap_layer_set_bitmap(flame_layer, flame);
	
	//layer_add_child(window_get_root_layer(pedometer),
		//	bitmap_layer_get_layer(pedometerBack_layer));
	
	//layer_add_child(window_get_root_layer(pedometer),
		//	bitmap_layer_get_layer(flame_layer));

//	layer_add_child(window_get_root_layer(pedometer), (Layer*) steps);
	layer_add_child(window_get_root_layer(pedometer), (Layer*) pedCount);
	layer_add_child(window_get_root_layer(pedometer), (Layer*) calories);
        layer_add_child(window_get_root_layer(pedometer), (Layer*) meters);
  layer_add_child(window_get_root_layer(pedometer), (Layer*) meterskm);
	layer_add_child(window_get_root_layer(pedometer), (Layer*) street);
	//layer_add_child(window_get_root_layer(pedometer), (Layer*) arrow);
        

	text_layer_set_font(pedCount, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BEBAS_15)));
	text_layer_set_font(calories, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BEBAS_15)));
	text_layer_set_font(street, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BEBAS_15)));
  text_layer_set_font(meters, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BEBAS_30)));
	text_layer_set_font(meterskm, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BEBAS_15)));
	
        //text_layer_set_text_alignment(steps, GTextAlignmentCenter);
	text_layer_set_text_alignment(pedCount, GTextAlignmentCenter);
	text_layer_set_text_alignment(calories, GTextAlignmentCenter);
	text_layer_set_text_alignment(meters, GTextAlignmentCenter);
  text_layer_set_text_alignment(meterskm, GTextAlignmentCenter);
	text_layer_set_text_alignment(street, GTextAlignmentCenter);
	//text_layer_set_text_alignment(arrow, GTextAlignmentCenter);

	//text_layer_set_text(steps, "s t e p s");

	static char buf[] = "1234567890";
	snprintf(buf, sizeof(buf), "%ld", pedometerCount);
	text_layer_set_text(pedCount, buf);

	static char buf2[] = "1234567890abcdefghijkl";
	snprintf(buf2, sizeof(buf2), "%ld", caloriesBurned);
	text_layer_set_text(calories, buf2);

  //      static char buf3[] = "1234567890";
	//snprintf(buf, sizeof(buf3), "arrow");
	//text_layer_set_text(arrow, buf3);

        static char buf4[] = "1234567890";
	snprintf(buf4, sizeof(buf4), "40");
	text_layer_set_text(meters, buf4);
	text_layer_set_text(meterskm, "km");

	static char buf5[] = "1234567890abcdefghijkl";
	snprintf(buf5, sizeof(buf5), "Hackathons Way");
	text_layer_set_text(street, buf5);
}

void ped_unload(Window *window) {
	struct StopwatchState state = (struct StopwatchState){
		.started = started,
		.start_time = start_time,
		.elapsed_time = elapsed_time,
		.pause_time = pause_time,
	};
  
  text_layer_destroy(text_layer);
  app_message_deregister_callbacks();
	text_layer_destroy(seconds_time_layer);
	text_layer_destroy(big_time_layer);
  
  app_timer_cancel(timer);
	window_destroy(pedometer);
	text_layer_destroy(pedCount);
	text_layer_destroy(calories);
  //text_layer_destroy(arrow);
  text_layer_destroy(street);
  text_layer_destroy(meters);
  text_layer_destroy(meterskm);
	//text_layer_destroy(steps);
	gbitmap_destroy(pedometerBack);
	accel_data_service_unsubscribe();
}
double float_time_ms() {
	time_t seconds;
	uint16_t milliseconds;
	time_ms(&seconds, &milliseconds);
	return (double)seconds + ((double)milliseconds / 1000.0);
}

void stop_stopwatch() {
    started = false;
	pause_time = float_time_ms();
    if(update_timer != NULL) {
        app_timer_cancel(update_timer);
        update_timer = NULL;
    }
}

void reset_stopwatch_handler(ClickRecognizerRef recognizer, Window *window) {
    if(busy_animating) return;
    bool is_running = started;
    stop_stopwatch();
    start_time = 0;
	elapsed_time = 0;
    if(is_running) start_stopwatch();
    update_stopwatch();
}

void toggle_stopwatch_handler(ClickRecognizerRef recognizer, Window *window) {
    if(started) {
        stop_stopwatch();
    } else {
        start_stopwatch();
    }
}

void update_stopwatch() {
    static char big_time[] = "00:00";
    static char deciseconds_time[] = ".0";
    static char seconds_time[] = ":00";

    // Now convert to hours/minutes/seconds.
    int tenths = (int)(elapsed_time * 10) % 10;
    int seconds = (int)elapsed_time % 60;
    int minutes = (int)elapsed_time / 60 % 60;
    int hours = (int)elapsed_time / 3600;

    // We can't fit three digit hours, so stop timing here.
    if(hours > 99) {
        stop_stopwatch();
        return;
    }
	
	if(hours < 1) {
		snprintf(big_time, 6, "%02d:%02d", minutes, seconds);
		snprintf(deciseconds_time, 3, ".%d", tenths);
	} else {
		snprintf(big_time, 6, "%02d:%02d", hours, minutes);
		snprintf(seconds_time, 4, ":%02d", seconds);
	}

    // Now draw the strings.
    text_layer_set_text(big_time_layer, big_time);
    text_layer_set_text(seconds_time_layer, hours < 1 ? deciseconds_time : seconds_time);
}



void start_stopwatch() {
    started = true;
	if(start_time == 0) {
		start_time = float_time_ms();
	} else if(pause_time != 0) {
		double interval = float_time_ms() - pause_time;
		start_time += interval;
	}
    update_timer = app_timer_register(100, handle_timer, NULL);
}

void handle_display_lap_times(ClickRecognizerRef recognizer, Window *window) {
  if (lors[dirindex].name)
    {
		text_layer_set_text(street, locs[dirindex].name);
    text_layer_set_text(meters, dists[dirindex].name);
    bitmap_layer_set_bitmap(arrow_layer, arrow_left);
  }
  else
    {
    text_layer_set_text(street, locs[dirindex].name);
    text_layer_set_text(meters, dists[dirindex].name);
    bitmap_layer_set_bitmap(arrow_layer, arrow_right);
  }
  dirindex=dirindex+1;
}

void config_provider(Window *window) {
	window_single_click_subscribe(BUTTON_RUN, (ClickHandler)toggle_stopwatch_handler);
	window_single_click_subscribe(BUTTON_RESET, (ClickHandler)reset_stopwatch_handler);
	window_single_click_subscribe(BUTTON_LAP, (ClickHandler)handle_display_lap_times);
}


void handle_timer(void* data) {
	if(started) {
		double now = float_time_ms();
		elapsed_time = now - start_time;
		update_timer = app_timer_register(100, handle_timer, NULL);
	}
	update_stopwatch();
}

void info_load(Window *window) {
	infor = text_layer_create(GRect(0, 0, 150, 150));

	if (isDark) {
		window_set_background_color(dev_info, GColorBlack);
		text_layer_set_background_color(infor, GColorClear);
		text_layer_set_text_color(infor, GColorWhite);
	} else {
		window_set_background_color(dev_info, GColorWhite);
		text_layer_set_background_color(infor, GColorClear);
		text_layer_set_text_color(infor, GColorBlack);
	}

	layer_add_child(window_get_root_layer(dev_info), (Layer*) infor);
	text_layer_set_text_alignment(infor, GTextAlignmentCenter);
	text_layer_set_text(infor,
			"\nDeveloped By: \nJathusan Thiruchelvanathan\n\nContact:\njathusan.t@gmail.com\n\n2014");
}

void info_unload(Window *window) {
	text_layer_destroy(infor);
	window_destroy(dev_info);
}

void window_load(Window *window) {

	splash = gbitmap_create_with_resource(RESOURCE_ID_SPLASH);
	window_set_background_color(window, GColorBlack);

	splash_layer = bitmap_layer_create(GRect(0, 0, 145, 185));
	bitmap_layer_set_bitmap(splash_layer, splash);
	layer_add_child(window_get_root_layer(window),
			bitmap_layer_get_layer(splash_layer));

	main_message = text_layer_create(GRect(0, 0, 150, 170));
	main_message2 = text_layer_create(GRect(3, 30, 150, 170));
	hitBack = text_layer_create(GRect(3, 40, 200, 170));

	text_layer_set_background_color(main_message, GColorClear);
	text_layer_set_text_color(main_message, GColorWhite);
	text_layer_set_font(main_message,
			fonts_load_custom_font(
					resource_get_handle(RESOURCE_ID_ROBOTO_LT_30)));
	layer_add_child(window_get_root_layer(window), (Layer*) main_message);

	
  text_layer_set_background_color(main_message2, GColorClear);
	text_layer_set_text_color(main_message2, GColorWhite);
	text_layer_set_font(main_message2,
			fonts_load_custom_font(
					resource_get_handle(RESOURCE_ID_ROBOTO_LT_15)));
	layer_add_child(window_get_root_layer(window), (Layer*) main_message2);

	text_layer_set_background_color(hitBack, GColorClear);
	text_layer_set_text_color(hitBack, GColorWhite);
	text_layer_set_font(hitBack,
			fonts_load_custom_font(
					resource_get_handle(RESOURCE_ID_ROBOTO_LT_15)));
	layer_add_child(window_get_root_layer(window), (Layer*) hitBack);

	text_layer_set_text(main_message, "      Goal");
	text_layer_set_text(main_message2, "          Reached!");
	text_layer_set_text(hitBack, "\n\n\n\n\n\n     << Press Back");
}

void window_unload(Window *window) {
	window_destroy(window);
	text_layer_destroy(main_message);
	text_layer_destroy(main_message2);
	text_layer_destroy(hitBack);
	bitmap_layer_destroy(splash_layer);
}

void autoCorrectZ(){
	if (Z_DELTA > YZ_DELTA_MAX){
		Z_DELTA = YZ_DELTA_MAX; 
	} else if (Z_DELTA < YZ_DELTA_MIN){
		Z_DELTA = YZ_DELTA_MIN;
	}
}

void autoCorrectY(){
	if (Y_DELTA > YZ_DELTA_MAX){
		Y_DELTA = YZ_DELTA_MAX; 
	} else if (Y_DELTA < YZ_DELTA_MIN){
		Y_DELTA = YZ_DELTA_MIN;
	}
}

void pedometer_update() {
	if (startedSession) {
		X_DELTA_TEMP = abs(abs(currX) - abs(lastX));
		if (X_DELTA_TEMP >= X_DELTA) {
			validX = true;
		}
		Y_DELTA_TEMP = abs(abs(currY) - abs(lastY));
		if (Y_DELTA_TEMP >= Y_DELTA) {
			validY = true;
			if (Y_DELTA_TEMP - Y_DELTA > 200){
				autoCorrectY();
				Y_DELTA = (Y_DELTA < YZ_DELTA_MAX) ? Y_DELTA + PED_ADJUST : Y_DELTA;
			} else if (Y_DELTA - Y_DELTA_TEMP > 175){
				autoCorrectY();
				Y_DELTA = (Y_DELTA > YZ_DELTA_MIN) ? Y_DELTA - PED_ADJUST : Y_DELTA;
			}
		}
		Z_DELTA_TEMP = abs(abs(currZ) - abs(lastZ));
		if (abs(abs(currZ) - abs(lastZ)) >= Z_DELTA) {
			validZ = true;
			if (Z_DELTA_TEMP - Z_DELTA > 200){
				autoCorrectZ();
				Z_DELTA = (Z_DELTA < YZ_DELTA_MAX) ? Z_DELTA + PED_ADJUST : Z_DELTA;
			} else if (Z_DELTA - Z_DELTA_TEMP > 175){
				autoCorrectZ();
				Z_DELTA = (Z_DELTA < YZ_DELTA_MAX) ? Z_DELTA + PED_ADJUST : Z_DELTA;
			}
		}
	} else {
		startedSession = true;
	}
}

void resetUpdate() {
	lastX = currX;
	lastY = currY;
	lastZ = currZ;
	validX = false;
	validY = false;
	validZ = false;
}

void update_ui_callback() {
	if ((validX && validY && !did_pebble_vibrate) || (validX && validZ && !did_pebble_vibrate)) {
		pedometerCount++;
		tempTotal++;

		caloriesBurned = (int) (pedometerCount / STEPS_PER_CALORIE);
		static char calBuf[] = "123456890abcdefghijkl";
		snprintf(calBuf, sizeof(calBuf), "Kcals\n%ld", caloriesBurned);
		text_layer_set_text(calories, calBuf);

    //		static char calBuf3[] = "123456890abcdefghijkl";
		//snprintf(calBuf3, sizeof(calBuf3), "Arrow \n%ld", caloriesBurned);
		//text_layer_set_text(arrow, calBuf3);
    
    
		static char buf[] = "123456890abcdefghijkl";
		snprintf(buf, sizeof(buf), "Steps\n%ld", pedometerCount);
		text_layer_set_text(pedCount, buf);

		static char buf2[] = "123456890abcdefghijkl";
		snprintf(buf2, sizeof(buf2), "%ld in Total", tempTotal);
		menu_items[2].subtitle = buf2;

		static char buf3[] = "1234567890abcdefg";
		snprintf(buf3, sizeof(buf3), "%ld Burned",
				(long) (tempTotal / STEPS_PER_CALORIE));
		menu_items[3].subtitle = buf3;

		layer_mark_dirty(window_get_root_layer(pedometer));
		layer_mark_dirty(window_get_root_layer(menu_window));

		if (stepGoal > 0 && pedometerCount == stepGoal) {
			vibes_long_pulse();
			window_set_window_handlers(window, (WindowHandlers ) { .load =
							window_load, .unload = window_unload, });
			window_stack_push(window, true);
		}
	}

	resetUpdate();
}

static void timer_callback(void *data) {
	AccelData accel = (AccelData ) { .x = 0, .y = 0, .z = 0 };
	accel_service_peek(&accel);

	if (!startedSession) {
		lastX = accel.x;
		lastY = accel.y;
		lastZ = accel.z;
	} else {
		currX = accel.x;
		currY = accel.y;
		currZ = accel.z;
	}
	
	did_pebble_vibrate = accel.did_vibrate;

	pedometer_update();
	update_ui_callback();

	layer_mark_dirty(window_get_root_layer(pedometer));
	timer = app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
}

void handle_init(void) {
	tempTotal = totalSteps = persist_exists(TS) ? persist_read_int(TS) : TSD;
	isDark = persist_exists(SID) ? persist_read_bool(SID) : true;
  
	if (!isDark) {
		theme = "Current: Light";
	} else {
		theme = "Current: Dark";
	}

	window = window_create();

	setup_menu_items();
	setup_menu_sections();
	setup_menu_window();

	window_stack_push(menu_window, true);
}

void handle_deinit(void) {
	totalSteps += pedometerCount;
	persist_write_int(TS, totalSteps);
	persist_write_bool(SID, isDark);
	accel_data_service_unsubscribe();
	window_destroy(menu_window);
}
