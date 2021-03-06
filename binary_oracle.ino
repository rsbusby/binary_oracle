#include "FastLED.h"

#include "binary_oracle_sensing.h"

// ----------------------------------
// --- PARAMETERS TO ADJUST ---------
boolean debug = 1;

// how often we poll the bio-signal
uint16_t sensing_period_in_millis = 100;

// milliseconds for sensing and recording
unsigned int sensor_time_millis = 2000;

// time to record sensor values when waiting for signal (takes 2 to start)
// unsigned long millis_between_start_detections = 280;

// low and high threshold for a signal to be detected
unsigned int lo_signal_threshold = 180;
// unsigned int hi_signal_threshold = 650;

// unsigned long time_between_trigrams_in_millis = 4000;

// Set the method of analyzing analog bio-signal
// 0 = Counting Peaks / Troughs
// 1 = Average Amplitude of Peaks vs/ Troughs (not implemented)
// 2 = mock random values
uint8_t bio_signal_analysis_type = 2;

unsigned long element_action_duration = 3000;

// 255 is fully on, not PWM
uint8_t element_action_write_value = 255;

// the gap in the zero LED pattern will be twice this many pixels
uint8_t led_half_gap = 2;
CRGB global_gap_color = CRGB::White; // Use Black for dark pixels

// --- END PARAMETERS TO ADJUST -----
// ----------------------------------

BinaryOracleSensor sensor = BinaryOracleSensor(2000, 180, 650, 0);

// current state of system is denoted by 2 variables
// current trigram can be 1 or 2
uint8_t current_trigram = 1;
// current_touch_state can be 1, 2, 3, 4
uint8_t current_touch_state = 1;

boolean sensor_paused;
unsigned long start_pause_sensor_time_in_millis;
unsigned long sensor_pause_duration = 4000;

// LED parameters
#define NUM_LEDS 120
#define NUM_LEDS_IN_SECTION 20
// #define NUM_STRIPS 1
#define LED_DATA_PIN_1 15
#define LED_DATA_PIN_2 16


#define NUM_LEDS_FIRE 7
#define LED_DATA_PIN_FIRE 16
#define LED_TYPE_FIRE WS2811
#define COLOR_ORDER_FIRE BRG
// int num_effective_pixels_in_trigram = NUM_LEDS / 2;

// where do sections begin
uint8_t start_section_1 = 0;
uint8_t start_section_2 = NUM_LEDS_IN_SECTION;
uint8_t start_section_3 = NUM_LEDS_IN_SECTION * 2;

// Output pins

#define FAN_OUT 17
#define PUMP_OUT 18
#define UV_LED_OUT 19

// LED strip properties
#define LED_TYPE WS2811
#define COLOR_ORDER BRG

// initialize FastLED strips
CRGB leds[1][NUM_LEDS];
CRGB leds_fire[NUM_LEDS_FIRE];

unsigned long element_start_time_in_millis;
boolean element_action_is_on = 0;
boolean fire_is_hot = 0;
uint8_t current_element_action_pin;

uint8_t touch_1;
uint8_t touch_2;
uint8_t touch_3;

boolean start = true;

void setup() {

  pinMode(FAN_OUT, OUTPUT);
  analogWrite(FAN_OUT, 0);
  pinMode(PUMP_OUT, OUTPUT);
  analogWrite(PUMP_OUT, 0);
  pinMode(UV_LED_OUT, OUTPUT);
  analogWrite(UV_LED_OUT, 0);
  // tell FastLED about the LED strip configuration

  Serial.begin(9600);
  delay(1000); // 2 second delay for recovery

  FastLED.addLeds<LED_TYPE, LED_DATA_PIN_1, COLOR_ORDER>(leds[0], NUM_LEDS);
  FastLED.addLeds<LED_TYPE_FIRE, LED_DATA_PIN_FIRE, COLOR_ORDER_FIRE>(leds_fire, NUM_LEDS_FIRE);
  // for(uint16_t i=0;i<NUM_LEDS;i++){
  //   leds[0][i] = CRGB::Black;
  // }

  // set master brightness control
  FastLED.setBrightness(100);
  // initialize_leds();


  // sensor parameters
  sensor.show_sensor_value = 1;
  sensor.lo_signal_threshold = 250;
  sensor.hi_signal_threshold = 650;
  sensor.millis_between_start_detections = 280;
  sensor.sensor_time_millis = 2000;
  sensor.bio_signal_analysis_type = 0;
  sensor.simulated_data = 0;
  sensor.debug = 1; //debug;

}

void initialize_leds(){
  for(int i=0;i<NUM_LEDS;i++){
    leds[0][i] = CRGB::Wheat;
  }
}

// main loop
void loop()
{

  if(start){
    initialize_leds();
    start = false;
  }


  EVERY_N_MILLISECONDS(sensing_period_in_millis) {
    check_action();
    update_fire();
    if(is_sensor_enabled()){
      sensor.check_binary_signal();
      if (sensor.signal_finished){
        sensor.signal_finished = 0;
        process_signal(sensor.signal);
      }
    }

    FastLED.show();
  }

}

// turn of fire LEDs
void quench_fire(){
  fire_is_hot = 0;
  for(uint8_t i=0;i<NUM_LEDS_FIRE;i++){
    leds_fire[i] = CRGB::Black;
  }
}

// random flickering
void fire_colors_fastled(){
  for(int i=0;i<NUM_LEDS_FIRE;i++){
     uint8_t red = random(0, 255);
     uint8_t green = random(0, red/4);
     leds_fire[i] = CRGB(red,green, 0);
  }
}

void update_fire(){
  if(fire_is_hot){
    fire_colors_fastled();
  }
}

// turn off action if it's time is up
void check_action(){
  if(element_action_is_on || fire_is_hot){
    unsigned long current_time_in_millis = millis();
    if(current_time_in_millis - element_start_time_in_millis > element_action_duration){

      if (fire_is_hot){
        if(debug){
          Serial.print("Turning off fire action");
        }
        quench_fire();
        fire_is_hot = 0;
      }
      else{
        if(debug){
          Serial.print("Turning off analog output pin ");
          Serial.println(current_element_action_pin);
        }
        analogWrite(current_element_action_pin, 0);
      }
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

uint8_t get_element_index_from_binary_values(uint8_t touch_1, uint8_t touch_2, uint8_t touch_3) {
  // get a number between 1 and 8 from 3 binary values
  return touch_1 + touch_2 * 2 + touch_3 * 4;
}

void send_string_data_over_serial(uint8_t sequence_step, uint8_t byte_value) {
  // Send two integers representing the elements, can be decoded by receiver
  Serial.print(sequence_step);
  Serial.println(byte_value);
}

void log_process_signal(uint8_t signal){
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

void process_signal(uint8_t signal){

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
      sensor.max_diff_millis = sensor.sensor_time_millis;

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
  initialize_leds();

}


void trigger_element_action(){

  uint8_t element = get_element_index_from_binary_values(touch_1, touch_2, touch_3);
  element_action_duration = 4000;

  // test single element
  element = 6;

  // do something different depending on the element value:
  switch (element) {
    case 0:    // Heaven, 000, UV LED
      if(debug){
        Serial.println("000 element -- Heaven");
      }
      current_element_action_pin = UV_LED_OUT;
      element_action_duration = 4000;
      break;
    case 1:    // Thunder, 001, Sound
      if(debug){
       Serial.println("001 element -- Thunder");
      }
      element_action_is_on = 0;
      return;
    case 2:    // Water, 010, Pump
      if(debug){
        Serial.println("010 element -- Water");
      }
      current_element_action_pin = PUMP_OUT;
      element_action_duration = 2500;
      break;
    case 3:    // Lake, 011, Pump
      if(debug){
        Serial.println("011 element -- Lake");
      }
      current_element_action_pin = PUMP_OUT;
      break;
    case 4:    // Mountain, 100, Sound
      if(debug){
        Serial.println("100 element -- Mountain");
      }
      element_action_is_on = 0;
      return;
    case 5:    // Fire , 101, Fire LED
      if(debug){
        Serial.println("101 element -- Fire");
      }
      // TODO: implement fire
      fire_is_hot = true;
      element_action_duration = 4000;
      start_element_timer();
      return;
    case 6:    // Wind, 110, Fan
      if(debug){
        Serial.println("110 element -- Wind");
      }
      current_element_action_pin = FAN_OUT;
      element_action_duration = 5000;
      break;
    case 7:    // Earth, 111, Sound
      if(debug){
        Serial.println("111 element -- Earth");
      }
 //     current_element_action_pin = FAN_OUT;
      element_action_is_on = 0;
      return;
  }

  if(debug){
    Serial.print("Turning on output pin ");
    Serial.println(current_element_action_pin);
  }
  analogWrite(current_element_action_pin, element_action_write_value);
  start_element_timer();
}

void start_element_timer(){
  element_action_is_on = 1;
  element_start_time_in_millis = millis();
}

// -------------------------------
// -- LED strip lighting routines

void trigger_led_strip(uint8_t signal){

  // initialize for current_touch_state == 1, to avoid compiler warnings
  uint16_t start_pixel = 0;
  uint16_t end_pixel = NUM_LEDS_IN_SECTION;
  CRGB color = CRGB::Yellow;
  CRGB gap_color = global_gap_color;

  uint16_t single_strip_shift = (current_trigram - 1) * (NUM_LEDS / 2);

  // strip index is 0 or 1
  uint8_t current_strip_index = 0; // 0 means single strip //current_trigram - 1;

  // determine which pixels to change, and what color
  switch (current_touch_state) {
     case 1:    //
       start_pixel = start_section_1 + single_strip_shift;
       end_pixel = start_pixel + NUM_LEDS_IN_SECTION;
       color = CRGB::DarkMagenta;
       break;
    case 2:    //
      start_pixel = start_section_2 + single_strip_shift;
      end_pixel = start_pixel + NUM_LEDS_IN_SECTION;
      color = CRGB::SlateBlue;
      break;
    case 3:    //
      start_pixel = start_section_3 + single_strip_shift;
      end_pixel = start_pixel + NUM_LEDS_IN_SECTION;
      color = CRGB::Lime;
      break;
  }

  // show line, with or without gap
  switch (signal) {
    case 0:    //
      //Serial.println("showing 0 in LED strip with gap");
      //Serial.print("show ")
      light_zero(current_strip_index, start_pixel, end_pixel, color, gap_color);
      break;
    case 1:    //
      // Serial.println("showing 1 in LED strip with no gap");
      light_one(current_strip_index , start_pixel, end_pixel, color);
      break;
  }
}

// show colored set of pixels, with a gap
void light_zero(uint8_t strip_index, uint16_t start_pixel, uint16_t end_pixel, CRGB color, CRGB gap_color){

    uint16_t half_point = start_pixel + (end_pixel - start_pixel) / 2;

    // before gap
    for(uint16_t dot = start_pixel; dot < half_point - led_half_gap; dot++) {
      leds[0][dot] = color;
    }

    // gap
    for(uint16_t dot = half_point - led_half_gap; dot < half_point + led_half_gap; dot++) {
      leds[0][dot] = gap_color;
    }

    // after gap
    for(uint16_t dot = half_point + led_half_gap; dot < end_pixel; dot++) {
      leds[0][dot] = color;
    }
    // FastLED.show();

    if(debug){
      Serial.print("light zero: ");
      Serial.print(start_pixel);
      Serial.print("  ");
      Serial.print(half_point - led_half_gap);
      Serial.print("  ");
      Serial.print(half_point + led_half_gap);
      Serial.print("  ");
      Serial.println(end_pixel);
    }
}

// show colored set of pixels, with no gap
void light_one(uint8_t strip_index, uint16_t start_pixel, uint16_t end_pixel, CRGB color){
  // light all the pixels
  if(debug){
    Serial.print("light one:  ");
    Serial.print(start_pixel);
    Serial.print("  ");
    Serial.println(end_pixel);
  }
  for(uint16_t dot = start_pixel; dot < end_pixel; dot++) {
    leds[0][dot] = color;
  }
  // FastLED.show();
}


// --- end lighting routines
