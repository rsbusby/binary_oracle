#include "FastLED.h"

// #include "binary_oracle_utils.h"

#include "binary_oracle_sensing.h"


// ----------------------------------
// --- PARAMETERS TO ADJUST ---------

// Set the method of analyzing analog bio-signal
// 0 = Counting Peaks / Troughs
// 1 = Average Amplitude of Peaks vs/ Troughs
// 2 = mock random values
int bio_signal_analysis_type = 0;
//
// // // how often we poll the bio-signal
int sensing_period_in_millis = 100;

// milliseconds for sensing and recording
int sensor_time_millis = 2000;

// time to record sensor values when waiting for signal (takes 2 to start)
unsigned long millis_between_start_detections = 280;

// low and high threshold for a signal to be detected
int lo_signal_threshold = 180;
int hi_signal_threshold = 600;

int show_sensor_value = 0;

unsigned long element_action_duration = 3000;

// 255 is fully on, not PWM
int element_action_write_value = 255;

// the gap in the zero LED pattern will be twice this many pixels
int led_half_gap = 5;

// --- END PARAMETERS TO ADJUST -----
// ----------------------------------

BinaryOracleSensor sensor = BinaryOracleSensor(sensor_time_millis, lo_signal_threshold, hi_signal_threshold, bio_signal_analysis_type);


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

// // array for sensor values
// int sensor_values[600];
// int sensor_count = 0;
//
// int waiting;
// int start_detected = 0;
// int start_time_in_millis;
// int signal_finished = 0;
// int signal;
//
// boolean signal_detected_first = false;
//
// unsigned long max_diff_millis;
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
    sensor.check_binary_signal();
    if (sensor.signal_finished){
      sensor.signal_finished = 0;
      process_signal(sensor.signal);
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

void process_signal(int signal){

  // do something different depending on the current phase of the system:
  switch (current_touch_state) {
    case 1:    // was waiting for first signal
      Serial.print("** touch 1 recorded as ");
      Serial.println(signal);
      touch_1 = signal;
      trigger_led_strip(signal);
      break;
    case 2:    //
      Serial.print("** touch 2 recorded as ");
      Serial.println(signal);
      touch_2 = signal;
      trigger_led_strip(signal);
      break;
    case 3:    //
      Serial.print("** touch 3 recorded as ");
      Serial.println(signal);
      touch_3 = signal;
      trigger_led_strip(signal);
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

void trigger_led_strip(int signal){

  // initialize for current_touch_state == 1, to avoid compiler warnings
  int start_pixel = 0;
  int end_pixel = 50;
  CRGB color = CRGB::Yellow;

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
      light_zero(current_strip_index, start_pixel, end_pixel, color);
      break;
    case 1:    //
      // Serial.println("showing 1 in LED strip with no gap");
      light_one(current_strip_index , start_pixel, end_pixel, color);
      break;
  }
}

// show colored set of pixels, with a gap
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

// show colored set of pixels, with no gap
void light_one(int strip_index, int start_pixel, int end_pixel, CRGB color){
  // light all the pixels
  for(int dot = start_pixel; dot < end_pixel; dot++) {
    leds[strip_index][dot] = color;
  }
  FastLED.show();
}

// --- end lighting routines
