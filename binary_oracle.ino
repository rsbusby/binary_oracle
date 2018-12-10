#include "FastLED.h"

#include "binary_oracle_sensing.h"

// ----------------------------------
// --- PARAMETERS TO ADJUST ---------
boolean debug = true;

// how often we poll the bio-signal
int sensing_period_in_millis = 100;

// milliseconds for sensing and recording
int sensor_time_millis = 2000;

// time to record sensor values when waiting for signal (takes 2 to start)
unsigned long millis_between_start_detections = 280;

// low and high threshold for a signal to be detected
unsigned int lo_signal_threshold = 180;
unsigned int hi_signal_threshold = 600;

int show_sensor_value = 1;

unsigned long time_between_trigrams_in_millis = 4000;

// Set the method of analyzing analog bio-signal
// 0 = Counting Peaks / Troughs
// 1 = Average Amplitude of Peaks vs/ Troughs (not implemented)
// 2 = mock random values
int bio_signal_analysis_type = 0;

unsigned long element_action_duration = 3000;

// 255 is fully on, not PWM
int element_action_write_value = 255;

// the gap in the zero LED pattern will be twice this many pixels
int led_half_gap = 5;
CRGB global_gap_color = CRGB::White; // Use Black for dark pixels

// --- END PARAMETERS TO ADJUST -----
// ----------------------------------

BinaryOracleSensor sensor = BinaryOracleSensor(sensor_time_millis, lo_signal_threshold, hi_signal_threshold, bio_signal_analysis_type);

// current state of system is denoted by 2 variables
// current trigram can be 1 or 2
int current_trigram = 1;
// current_touch_state can be 1, 2, 3, 4
int current_touch_state = 1;

boolean sensor_paused;
unsigned long start_pause_sensor_time_in_millis;
unsigned long sensor_pause_duration = 4000;

// LED parameters
#define NUM_LEDS 150
#define NUM_LEDS_IN_SECTION 50
#define NUM_STRIPS 2
#define LED_DATA_PIN_1 15
#define LED_DATA_PIN_2 16

// Output pins
#define FAN_OUT_1 11
#define FAN_OUT_2 10
#define TERRARIUM_OUT 9
#define PUMP_OUT 8
#define SERVO_OUT 7

// digital input pins for Azoteq sensors (note that analog input A0 is pin 14)
#define AZOTEQ_PROX_PIN 15
#define AZOTEQ_TOUCH_PIN 16

// LED strip properties
#define LED_TYPE WS2811
#define COLOR_ORDER GRB

// initialize FastLED strips
CRGB leds[NUM_STRIPS][NUM_LEDS];

unsigned long element_start_time_in_millis;
boolean element_action_is_on = 0;
int current_element_action_pin = TERRARIUM_OUT;

int touch_1;
int touch_2;
int touch_3;

// ---------- from TreeChi ----------------------------

// Gradient palette "Green_White_gp"
DEFINE_GRADIENT_PALETTE( Green_White_gp ) {
    0,   0,255,  0,
  127,  42,255, 45,
  255, 255,255,255};

// Gradient palette "Magenta_White_gp"
DEFINE_GRADIENT_PALETTE( Magenta_White_gp ) {
    0, 255,  0,255,
  127, 255, 55,255,
  255, 255,255,255};

CRGBPalette16 standby_palette(Green_White_gp);
CRGBPalette16 active_palette(Magenta_White_gp);

uint8_t MAX_PALETTE_CHANGES_PER_BLEND = 36;

// maximum analog signal value
#define MAX_ANALOG  1024

unsigned int new_brightness;
unsigned int exp_brightness;

// analog event threshold
volatile unsigned int adc_threshold = 800;

// time of increased brightness after signal, in milliseconds.
unsigned long TIMEOUT_MILLIS = sensor_time_millis;

// twinkle boost
uint8_t twinkle_boost;
uint8_t twinkle_boost_standby = 90;
uint8_t twinkle_boost_active = 200;

uint8_t hue_before_boost;
uint8_t hue_before_boost_active = 100;
uint8_t hue_before_boost_standby = 40;

#define CHANCE_OF_TWINKLE 96

// brightness range active, after signal
uint8_t min_brightness_active = 5;
uint8_t max_brightness_active = 30;

// brightness range inactive (if want dark after installation, set both to 0)
uint8_t min_brightness_standby = 3;
uint8_t max_brightness_standby = 8;

unsigned int LOGGING_PERIOD_IN_MILLIS = 700;  // not too small, around 500 is good
unsigned int SENSING_PERIOD_IN_MILLIS = 40;  // 40 works
  // 40 works
unsigned int LOOP_DELAY_IN_MILLIS = 0;  // delay for every loop 10 works, do we even need this when installed?


unsigned int num_blending_calls_to_active = 8;
unsigned int num_blending_calls_to_standby = 6;

unsigned int blending_period_to_active = 2;
unsigned int blending_period_to_standby = 2;


// --- END ADJUSTABLE PARAMETERS for TreeChi

unsigned int NUM_BLENDING_CALLS_PER_LOOP = 1;
unsigned int BLENDING_PERIOD_IN_MILLIS = 40;

// testing by simulating a changing analog signal
int fake_analog_increment = 2;

uint16_t active_pixel = 0;

// timing and sensor variables
unsigned int adc_val;
int touchread;
uint8_t current_brightness;
uint8_t current_hue;

unsigned int brightness_gain_from_adc = 1.0;
unsigned int min_brightness = 80;
unsigned int max_brightness = 120;

unsigned long time_of_last_event = millis();
unsigned long millis_since_last_event = 0;

volatile unsigned int prox = 0;
volatile unsigned int touch = 0;
boolean is_active;

uint16_t led_boost[NUM_LEDS_IN_SECTION];
uint8_t relative_brightnesses[NUM_LEDS_IN_SECTION];

CRGBPalette16 gCurrentPalette( CRGB::Black);
CRGBPalette16 gTargetPalette( CRGB::Blue);

// end TreeChi

void setup() {
  Serial.begin(115200);
  delay(1000); // 2 second delay for recovery

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, LED_DATA_PIN_1, COLOR_ORDER>(leds[0], NUM_LEDS);
  FastLED.addLeds<LED_TYPE, LED_DATA_PIN_2, COLOR_ORDER>(leds[1], NUM_LEDS);

  // set master brightness control
  FastLED.setBrightness(100);

  // sensor parameters
  sensor.show_sensor_value = 1;
  sensor.lo_signal_threshold = lo_signal_threshold;
  sensor.hi_signal_threshold = hi_signal_threshold;
  sensor.millis_between_start_detections = millis_between_start_detections;
  sensor.sensor_time_millis = sensor_time_millis;
  sensor.bio_signal_analysis_type = bio_signal_analysis_type;
  sensor.simulated_data = 0;
  sensor.debug = debug;

}

// main loop
void loop()
{

  // reporting to serial output

    EVERY_N_MILLISECONDS(LOGGING_PERIOD_IN_MILLIS) {
       if(debug && 0){
       //if(debug){
        log_treechi();
    }
  }

  EVERY_N_MILLISECONDS(sensing_period_in_millis) {
    check_action();
    if(is_sensor_enabled()){
      sensor.check_binary_signal();
      if (sensor.signal_finished){
        sensor.signal_finished = 0;
        process_signal(sensor.signal);
      }
    }

    // show_treechi_lights();
    FastLED.show();
  }

}

// turn off action if it's time is up
void check_action(){
  if(element_action_is_on){
    unsigned long current_time_in_millis = millis();
    if(current_time_in_millis - element_start_time_in_millis > element_action_duration){
      analogWrite(current_element_action_pin, 0);
      element_action_is_on = 0;
    }
  }
}

void pause_sensor(unsigned long pause_in_millis){
  sensor_paused = 1;
  sensor_pause_duration = pause_in_millis;
  start_pause_sensor_time_in_millis = millis();
}

boolean is_sensor_enabled(){

  if(!sensor_paused){
    return 1;
  }

  unsigned long current_time_in_millis = millis();
  if(current_time_in_millis - start_pause_sensor_time_in_millis >= sensor_pause_duration){
    sensor_paused = 0;
    return 1;
  }
  // still paused
  return 0;
}

int get_element_index_from_binary_values(int touch_1, int touch_2, int touch_3) {
  // get a number between 1 and 8 from 3 binary values
  return touch_1 + touch_2 * 2 + touch_3 * 4;
}

void send_string_data_over_serial(int sequence_step, int byte_value) {
  // Send two integers representing the elements, can be decoded by receiver
  Serial.print(sequence_step);
  Serial.println(byte_value);
}

void log_treechi(){
  Serial.print("Time since last event: ");
  Serial.println(millis_since_last_event);
  Serial.print("touchRead: ");
  Serial.println(touchread);
  Serial.print("Analog: ");
  Serial.println(adc_val);
   //Serial.print("FastLED Brightness Level: ");
   //Serial.println(255); // current_brightness);
   //Serial.print("Hue: ");
   //Serial.println(current_hue);
  Serial.print("Current Max Brightness: ");
  Serial.println(max_brightness);
  Serial.print("is active: ");
  Serial.println(is_active);
  Serial.println();
}

void log_process_signal(int signal){
  if (current_touch_state < 4){
    Serial.print("** touch ");
  }
  else{
    Serial.print("** element reaction ");
  }
  Serial.print(current_touch_state);
  Serial.print(" recorded as ");
  Serial.println(signal);
}

void process_signal(int signal){

  if (debug){
    log_process_signal(signal);
  }

  // do something different depending on the current phase of the system:
  switch (current_touch_state) {
    case 1:    // was waiting for first signal
      touch_1 = signal;
      trigger_led_strip(signal);
      break;
    case 2:    //
      touch_2 = signal;
      trigger_led_strip(signal);
      break;
    case 3:
      touch_3 = signal;
      trigger_led_strip(signal);

      // Send trigram to Serial
      send_string_data_over_serial(1 + (current_trigram - 1) * 4, touch_1);
      send_string_data_over_serial(2 + (current_trigram - 1) * 4, touch_2);
      send_string_data_over_serial(3 + (current_trigram - 1) * 4, touch_3);

      trigger_element_action();

      // don't wait for a touch this time
      sensor.waiting = 0;

      break;
    case 4:
      // Send element reaction to serial
      send_string_data_over_serial(4 + (current_trigram - 1) * 4, signal);

      // pause sensor but keep background lighting going
      pause_sensor(6000);
      break;
  }

  // update state
  current_touch_state = current_touch_state + 1;
  if (current_touch_state > 4){
    current_touch_state = 1;

    // update trigram
    current_trigram = current_trigram + 1;
    if (current_trigram > 2){
      if(debug){
        Serial.println(" ******** Returning to initial state, trigram 1 ********");
        Serial.println();
      }
      current_trigram = 1;

      // pause everything for a bit
      delay(10000);
      reset_system();
    }
    else{
      if(debug){
        Serial.println(" ***** Moving on to trigram 2");
      }
    }

  }

}

void reset_system(){
  // reset both LED strips
  // anything else?
}

void trigger_element_action(){

  int element = get_element_index_from_binary_values(touch_1, touch_2, touch_3);

  // do something different depending on the element value:
  switch (element) {
    case 0:    // Heaven, 000, ???
      if(debug){
        Serial.println("000 element -- Heaven");
      }
      current_element_action_pin = FAN_OUT_1;
      break;
    case 1:    // Thunder, 001, sound file
      if(debug){
       Serial.println("001 element -- Thunder");
      }
      current_element_action_pin = FAN_OUT_1;
      break;
    case 2:    // Water, 010, pump
      if(debug){
        Serial.println("010 element -- Water");
      }
      current_element_action_pin = FAN_OUT_1;
      break;
    case 3:    // Lake, 011, pump?
      if(debug){
        Serial.println("011 element -- Lake");
      }
      current_element_action_pin = FAN_OUT_1;
      break;
    case 4:    // Mountain, 100, ??
      if(debug){
        Serial.println("100 element -- Mountain");
      }
      current_element_action_pin = FAN_OUT_1;
      break;
    case 5:    // Fire , 101, LED ?
      if(debug){
        Serial.println("101 element -- Fire");
      }
      current_element_action_pin = FAN_OUT_1;
      break;
    case 6:    // Wind, 110, Fan
      if(debug){
        Serial.println("110 element -- Wind");
      }
      current_element_action_pin = FAN_OUT_1;
      break;
    case 7:    // Earth, 111, vibration
      if(debug){
        Serial.println("111 element -- Earth");
      }
      current_element_action_pin = FAN_OUT_1;
      break;
  }

  analogWrite(current_element_action_pin, element_action_write_value);
  element_action_is_on = 1;
  element_start_time_in_millis = millis();
}


// -------------------------------
// -- LED strip lighting routines

void trigger_led_strip(int signal){

  // initialize for current_touch_state == 1, to avoid compiler warnings
  int start_pixel = 0;
  int end_pixel = 50;
  CRGB color = CRGB::Yellow;
  CRGB gap_color = global_gap_color;

  // strip index is 0 or 1
  int current_strip_index = current_trigram - 1;

  // determine which pixels to change, and what color
  switch (current_touch_state) {
     case 1:    //
       start_pixel = 0;
       end_pixel = 50;
       color = CRGB::Red;
       break;
    case 2:    //
      start_pixel = 50;
      end_pixel = 100;
      color = CRGB::Blue;
      break;
    case 3:    //
      start_pixel = 100;
      end_pixel = 150;
      color = CRGB::Green;
      break;
  }

  // show line, with or without gap
  switch (signal) {
    case 0:    //
      // Serial.println("showing 0 in LED strip with gap");
      light_zero(current_strip_index, start_pixel, end_pixel, color, gap_color);
      break;
    case 1:    //
      // Serial.println("showing 1 in LED strip with no gap");
      light_one(current_strip_index , start_pixel, end_pixel, color);
      break;
  }
}

// show colored set of pixels, with a gap
void light_zero(int strip_index, int start_pixel, int end_pixel, CRGB color, CRGB gap_color){

    int half_point = (end_pixel - start_pixel) / 2;

    // before gap
    for(int dot = start_pixel; dot < half_point - led_half_gap; dot++) {
      leds[strip_index][dot] = color;
    }

    // gap
    for(int dot = half_point - led_half_gap; dot < half_point + led_half_gap; dot++) {
      leds[strip_index][dot] = gap_color;
    }

    // after gap
    for(int dot = half_point + led_half_gap; dot < end_pixel; dot++) {
      leds[strip_index][dot] = color;
    }
    // FastLED.show();
}

// show colored set of pixels, with no gap
void light_one(int strip_index, int start_pixel, int end_pixel, CRGB color){
  // light all the pixels
  for(int dot = start_pixel; dot < end_pixel; dot++) {
    leds[strip_index][dot] = color;
  }
  // FastLED.show();
}


// --- end lighting routines


// from TreeChi
void show_treechi_lights_for_section(int strip_index, int start_pixel){

    // DEBUG printing
    // Serial.print("Lighting TreeChi-style for strip ");
    // Serial.print(strip_index);
    // Serial.print(", start_pixel ");
    // Serial.println(start_pixel);

    int end_pixel = start_pixel + NUM_LEDS_IN_SECTION;
    for( int i = start_pixel; i < end_pixel; i++) {

       if (led_boost[i] > 0){
          led_boost[i] -= 1;
        }
       uint8_t hue = hue_before_boost + led_boost[i] / 2;
       leds[strip_index][i] = ColorFromPalette( gCurrentPalette, hue, relative_brightnesses[i], LINEARBLEND);
     }
}

void show_treechi_lights(){

  // set parameters
  get_brightness();

  if(current_trigram == 1){
    // if on first trigram, show TreeChi for entire 2nd trigram
    show_treechi_lights_for_section(1, 0);
    show_treechi_lights_for_section(1, 50);
    show_treechi_lights_for_section(1, 100);
  }

  // now set up for partial strip
  int strip_index = 0;
  if(current_trigram == 2){
    strip_index = 1;
  }

  if(current_touch_state < 2){
    show_treechi_lights_for_section(strip_index, 0);
  }
  if (current_touch_state < 3){
    show_treechi_lights_for_section(strip_index, 50);
  }
  if (current_touch_state < 4){
    show_treechi_lights_for_section(strip_index, 100);
  }
}

void get_brightness(){
    // adc_val = analogRead(A0);
    adc_val = sensor.sensor_value;

    // reset timeout if either signal exists
    if (adc_val > adc_threshold || adc_val < lo_signal_threshold){
      time_of_last_event = millis();
    }

    // check if current state is active
    unsigned long current_time = millis();
    millis_since_last_event = current_time - time_of_last_event;

    // set the brightness levels and palette depending on whether we are active
    if (millis_since_last_event < TIMEOUT_MILLIS){
      min_brightness = min_brightness_active;
      max_brightness = max_brightness_active;
      twinkle_boost = twinkle_boost_active;
      hue_before_boost = hue_before_boost_active;
      gTargetPalette = active_palette;
      BLENDING_PERIOD_IN_MILLIS = blending_period_to_active;
      NUM_BLENDING_CALLS_PER_LOOP = num_blending_calls_to_active;
      MAX_PALETTE_CHANGES_PER_BLEND = 36;
      is_active = true;
    }
    else{
      min_brightness = min_brightness_standby;
      max_brightness = max_brightness_standby;
      twinkle_boost = twinkle_boost_standby;
      hue_before_boost = hue_before_boost_standby;
      gTargetPalette = standby_palette;
      BLENDING_PERIOD_IN_MILLIS = blending_period_to_standby;
      NUM_BLENDING_CALLS_PER_LOOP = num_blending_calls_to_standby;
      MAX_PALETTE_CHANGES_PER_BLEND = 24;
      is_active = false;
    }

    // calculate brightness
    float normalized_signal = float(adc_val) / MAX_ANALOG;
    new_brightness = int(normalized_signal * (max_brightness - min_brightness) + min_brightness);

//    current_hue = adc_val / 4;

    if (new_brightness < current_brightness){
       current_brightness -= 1;
    }
    else if ( new_brightness > current_brightness){
      current_brightness += 1;
    }

//    if (is_active){
      active_pixel = int(normalized_signal * NUM_LEDS_IN_SECTION);
      led_boost[active_pixel] = twinkle_boost;

//    }
//    else{
//      if (random8() < CHANCE_OF_TWINKLE){
//        active_pixel = (random8() * NUM_LEDS) / 255;
//        led_boost[active_pixel] = twinkle_boost_standby;
//        Serial.print("random pixel  ");
//        Serial.print(active_pixel);
//        Serial.print("  ");
//        Serial.println(max_brightness + led_boost[active_pixel]);
//      }
//    }

    for( int i = 0; i < NUM_LEDS_IN_SECTION; i++) {
      relative_brightnesses[i] = current_brightness;
      relative_brightnesses[i] += led_boost[i];
    }

}

void limit_brightness(){
   // limit brightness just in case
    if (current_brightness < 0){
      current_brightness = 0;
    }
    if (current_brightness > 255){
      current_brightness = 255;
    }
}

uint8_t limit_brightness_val(uint16_t given_brightness){
   // limit brightness just in case
    if (given_brightness < 0){
      given_brightness = 0;
    }
    if (given_brightness > 255){
      given_brightness = 255;
    }
    return given_brightness;
}
