#include "FastLED.h"

#define THIS_VERSION "0.2.0"

// ----------------------------------
// --- PARAMETERS TO ADJUST ---------
// 1 for simulated sensor data, 0 with real bio-signal
#define SIMULATED_DATA 0

// Set the method of analyzing analog bio-signal
// 0 = Counting Peaks / Troughs
// 1 = Average Amplitude of Peaks vs/ Troughs
// 2 = mock random values
#define BIO_SIGNAL_ANALYSIS_TYPE 0

// how often we poll the bio-signal
int sensing_period_in_millis = 100;

// milliseconds for sensing and recording
unsigned long sensor_time_millis = 2000;


// time to record sensor values when waiting for signal (takes 2 to start)
int millis_between_start_detections = 280;

// low and high threshold for a signal to be detected
int lo_signal_threshold = 180;
int hi_signal_threshold = 600;

#define show_sensor_value 0

unsigned long element_action_duration = 3000;

// 255 is fully on, not PWM
int element_action_write_value = 255;

// the gap in the zero LED pattern will be twice this many pixels
int led_half_gap = 5;

// --- END PARAMETERS TO ADJUST -----
// ----------------------------------

// current state of system is denoted by 2 variables
// current trigram can be 1 or 2
int current_trigram = 1;
// current_touch_state can be 1, 2, 3, 4
int current_touch_state = 1;

// LED parameters
#define NUM_LEDS 150
#define NUM_STRIPS 2
#define LED_DATA_PIN_1 16
#define LED_DATA_PIN_2 17

// Output pins
#define FAN_OUT_1 18
#define FAN_OUT_2 19
#define TERRARIUM_OUT 20
#define PUMP_OUT 21
#define SERVO_OUT 22

// digital input pins for Azoteq sensors (note that analog input A0 is pin 14)
#define AZOTEQ_PROX_PIN 17
#define AZOTEQ_TOUCH_PIN 16

// LED strip properties
#define LED_TYPE WS2811
#define COLOR_ORDER GRB

// initialize FastLED strips
//CRGB leds[NUM_LEDS];
//CRGB leds2[NUM_LEDS];
CRGB leds[NUM_STRIPS][NUM_LEDS];

// array for sensor values
int sensor_values[600];
int sensor_count = 0;

int waiting;
int start_detected = 0;
int start_time_in_millis;
int signal_finished = 0;
int signal;

boolean signal_detected_first = false;

unsigned long max_diff_millis;
unsigned long current_time_in_millis;

unsigned long element_start_time_in_millis;
boolean element_action_is_on = 0;
int current_element_action_pin = TERRARIUM_OUT;

int touch_1;
int touch_2;
int touch_3;

void setup() {
  Serial.begin(115200);
  delay(1000); // 2 second delay for recovery
  Serial.println("Starting version 12: ");

  current_time_in_millis = millis();
  start_time_in_millis = millis();
  waiting = 1;
  max_diff_millis = millis_between_start_detections;

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, LED_DATA_PIN_1, COLOR_ORDER>(leds[0], NUM_LEDS);
  FastLED.addLeds<LED_TYPE, LED_DATA_PIN_2, COLOR_ORDER>(leds[1], NUM_LEDS);

  // set master brightness control
  FastLED.setBrightness(100);

}

// main loop
void loop()
{

  EVERY_N_MILLISECONDS(sensing_period_in_millis) {
    check_action();
    check_binary_signal();
    if (signal_finished){
      process_signal();
    }
  }

}

// turn off action if it's time is up
void check_action(){
  if(element_action_is_on){
    current_time_in_millis = millis();
    if(current_time_in_millis - element_start_time_in_millis > element_action_duration){
      analogWrite(current_element_action_pin, 0);
      element_action_is_on = 0;
    }
  }
}

int get_element_index_from_binary_values(int touch_1, int touch_2, int touch_3) {
  // get a number between 1 and 8 from 3 binary values
  return touch_1 + touch_2 * 2 + touch_3 * 4;
}

void process_signal(){
  signal_finished = 0;

  // do something different depending on the current phase of the system:
  switch (current_touch_state) {
    case 1:    // was waiting for first signal
      Serial.print("** touch 1 recorded as ");
      Serial.println(signal);
      touch_1 = signal;
      trigger_led_strip();
      break;
    case 2:    //
      Serial.print("** touch 2 recorded as ");
      Serial.println(signal);
      touch_2 = signal;
      trigger_led_strip();
      break;
    case 3:    //
      Serial.print("** touch 3 recorded as ");
      Serial.println(signal);
      touch_3 = signal;
      trigger_led_strip();
      // Send trigram to Serial
      trigger_element_action();
      break;
    case 4:    //
      Serial.print("** Element action finished, reaction recorded as ");
      Serial.println(signal);
      // Send element reaction to serial
      break;
  }

  // update state
  current_touch_state = current_touch_state + 1;
  if (current_touch_state > 4){
    current_touch_state = 1;

    // update trigram
    current_trigram = current_trigram + 1;
    if (current_trigram > 2){
      Serial.println(" ******** Returning to initial state, trigram 1 ********");
      Serial.println();
      current_trigram = 1;
      reset_system();
    }
    else{
      Serial.println(" ***** Moving on to trigram 2");
    }

  }

// spacing for clarity
Serial.println();
}

void reset_system(){
  // reset both LED strips
  // anything else?
}

void trigger_element_action(){


  int element = get_element_index_from_binary_values(touch_1, touch_2, touch_3);

  // Serial.print(" .... triggering element ... ");
  // Serial.println(element);

  // do something different depending on the element value:
  switch (element) {
    case 0:    // Heaven, 000, ???
      Serial.println("000 element -- Heaven");
      current_element_action_pin = FAN_OUT_1;
      break;
    case 1:    // Thunder, 001, sound file
      Serial.println("001 element -- Thunder");
      current_element_action_pin = FAN_OUT_1;
      break;
    case 2:    // Water, 010, pump
      Serial.println("010 element -- Water");
      current_element_action_pin = FAN_OUT_1;
      break;
    case 3:    // Lake, 011, pump?
    Serial.println("011 element -- Lake");
      current_element_action_pin = FAN_OUT_1;
      break;
    case 4:    // Mountain, 100, ??
      Serial.println("100 element -- Mountain");
      current_element_action_pin = FAN_OUT_1;
      break;
    case 5:    // Fire , 101, LED ?
    Serial.println("101 element -- Fire");
      current_element_action_pin = FAN_OUT_1;
      break;
    case 6:    // Wind, 110, Fan
      Serial.println("110 element -- Wind");
      current_element_action_pin = FAN_OUT_1;
      break;
    case 7:    // Earth, 111, vibration
      Serial.println("111 element -- Earth");
      current_element_action_pin = FAN_OUT_1;
      break;
  }

  analogWrite(current_element_action_pin, element_action_write_value);
  element_action_is_on = 1;
  element_start_time_in_millis = millis();
}


// -------------------------------
// -- LED strip lighting routines

void trigger_led_strip(){

  // for current_touch_state == 1
  int start_pixel = 0;
  int end_pixel = 50;
  CRGB color = CRGB::Yellow;

  // strip index is 0 or 1
  int current_strip_index = current_trigram - 1;


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

  switch (signal) {
    case 0:    //
      // Serial.println("showing 0 in LED strip with gap");
      light_zero(current_strip_index, start_pixel, end_pixel, color);
      break;
    case 1:    // Thunder, 001, sound file
      // Serial.println("showing 1 in LED strip with no gap");
      light_one(current_strip_index , start_pixel, end_pixel, color);
      break;
  }
}

void light_zero(int strip_index, int start_pixel, int end_pixel, CRGB color){

    int half_point = (end_pixel - start_pixel) / 2;
    for(int dot = start_pixel; dot < half_point - led_half_gap; dot++) {
      leds[strip_index][dot] = color;
    }
    for(int dot = half_point + led_half_gap; dot < end_pixel; dot++) {
      leds[strip_index][dot] = color;
    }
    FastLED.show();
}

void light_one(int strip_index, int start_pixel, int end_pixel, CRGB color){
  // light all the pixels
  for(int dot = start_pixel; dot < end_pixel; dot++) {
    leds[strip_index][dot] = color;
  }
  FastLED.show();
}

// --- end lighting routines

// ---------------------------------------------
// signal detection and processing
void get_analog_value_and_add_to_time_series(){

    int sensor_value = analogRead(A0);
    sensor_values[sensor_count] = sensor_value;
    sensor_count += 1;

    if (show_sensor_value){
      Serial.print("  -- sensor input: ");
      Serial.println(sensor_value);
    }

}


void reset_time_series(){
  sensor_count = 0;
}


// ---------------------------
// main control function for sensing
// changes program state depending on recent sensor values and time elapsed
void check_binary_signal(){

  current_time_in_millis = millis();

  if (waiting == 1){
    int started = check_start();
    if (started){
      Serial.println("          ++++ Detected touch ++++");
      waiting = 0;
      // set up for signal recording
      max_diff_millis = sensor_time_millis;
    }
  }
  else{
  // waiting = 0, recording
    if(current_time_in_millis - start_time_in_millis < max_diff_millis){
      get_analog_value_and_add_to_time_series();
      // current_time_in_millis = millis();
    }
    else{
      // done listening, set up for start detection
      waiting = 1;
      max_diff_millis = millis_between_start_detections;
      start_time_in_millis = millis();
      // current_time_in_millis = millis();

      // get binary signal value
      signal_finished = 1;
      signal = get_binary_from_time_series();
    }
  }
}

// -----------------------------
// functions to detect start of signal
int min_sensor_val(){
  int min_val = 1023;
  for (int i=0; i < sensor_count;i++ ){
    if ( sensor_values[i] < min_val ){
      min_val = sensor_values[i];
    }
  }
  return min_val;
}


int max_sensor_val(){
  int max_val = 0;
  for (int i=0; i < sensor_count;i++ ){
    if ( sensor_values[i] > max_val ){
      max_val = sensor_values[i];
    }
  }
  return max_val;
}


int detect_signal_in_time_series(){
  // check values in sensor_values to see if any past threshold

  if(SIMULATED_DATA){
    return random(2);
  }

  if ( max_sensor_val() > hi_signal_threshold || min_sensor_val() < lo_signal_threshold){
    return 1;
  }
  else{
    return 0;
  }
}


void reset_signal_detection(){
  reset_time_series();
  current_time_in_millis = millis();
  start_time_in_millis = millis();
}


int check_start(){

  // Serial.print("check_start:  ");

  current_time_in_millis = millis();

  // Serial.println(current_time_in_millis - start_time_in_millis);

  // if not done listening, record and keep going
  if(current_time_in_millis - start_time_in_millis < max_diff_millis){
    // Serial.println("readin");
    get_analog_value_and_add_to_time_series();
    return 0;
  }

  // done listening for start
  int signal_detected = detect_signal_in_time_series();

  if ( ! signal_detected ){
    signal_detected_first = 0;
    reset_signal_detection();
    return 0;
  }
  if (signal_detected_first){
      signal_detected_first = 0;
      // don't reset sensor data, keep the last period
      return 1;
  }
  else{
    // detected a signal, but will wait one more cycle
    Serial.println("          ++ Possible touch, debouncing ++");
    signal_detected_first = 1;
    reset_signal_detection();
    return 0;
  }
}


// --------------------------------------------
// functions to determine binary signal value
int get_mean(){
  int sum = 0;
  for (int i=0; i < sensor_count;i++ ){
    sum += sensor_values[i];
  }
  // TODO: would this be faster without float conversion?
  return int(sum / float(sensor_count));

}

int compare_troughs_and_peaks(int ref_val){
  // compare number of values above and below a reference value
  int lo_count = 0;
  int hi_count = 0;

  for (int i=0; i < sensor_count;i++ ){
    int val = sensor_values[i];
    if ( val < ref_val ){
      lo_count += 1;
    }
    else if (val > ref_val){
      hi_count += 1;
    }
  }

  if (hi_count > lo_count){
    return 1;
  }
  return 0;
}

int get_binary_signal_from_counts(){
  // method 0 to get a roughly 50:50 value from the bio-signal
  int mean = get_mean();
  return compare_troughs_and_peaks(mean);
}

int get_binary_from_time_series() {

   if (BIO_SIGNAL_ANALYSIS_TYPE == 2 || SIMULATED_DATA){
     // fake it
     return random(2);
   }
   else if (BIO_SIGNAL_ANALYSIS_TYPE == 1){
     // TODO: type 1
     return random(2);
   }
   else{
     // BIO_SIGNAL_ANALYSIS_TYPE == 0
     return get_binary_signal_from_counts();
   }
}

// ------ end sensor detection and processing
