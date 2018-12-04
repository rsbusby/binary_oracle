#include "FastLED.h"

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define THIS_VERSION "0.0.1"

// LED parameters
#define NUM_LEDS 150
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

// array for sensor values
int sensor_values[600];
int sensor_count = 0;

// initialize FastLED strips
CRGB leds[NUM_LEDS];
CRGB leds2[NUM_LEDS];

//
int waiting = 1;
int start_detected = 0;

// --- PARAMETERS TO ADJUST ---------

// Set the method of analyzing analog bio-signal
// 0 = Counting Peaks / Troughs
// 1 = Average Amplitude of Peaks vs/ Troughs
// 2 = mock random values
#define BIO_SIGNAL_ANALYSIS_TYPE 2

// how often we poll the bio-signal
int sensing_period_in_millis = 40;

// --- END PARAMETERS TO ADJUST -----

void setup() {

  Serial.begin(9600);
  Serial.print("version ");
  Serial.println(THIS_VERSION);
  Serial.println("");

  // set up digital input pins
  pinMode(AZOTEQ_PROX_PIN, INPUT);           // set pin to input
  digitalWrite(AZOTEQ_PROX_PIN, HIGH);       // turn on pullup resistors

  pinMode(AZOTEQ_TOUCH_PIN, INPUT);           // set pin to input
  digitalWrite(AZOTEQ_TOUCH_PIN, HIGH);       // turn on pullup resistors

  delay(2000); // 2 second delay for recovery

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, LED_DATA_PIN_1, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.addLeds<LED_TYPE, LED_DATA_PIN_2, COLOR_ORDER>(leds2, NUM_LEDS);


  // set master brightness control
  FastLED.setBrightness(100);

}

// main loop
void loop()
{

  if (waiting == 1){
    start_detected = detect_start();
  }

  if ( ! start_detected){
    continue;
  }
  else{ // start_detected
    waiting = 0;
    signal_start()

    int touch_1 = get_binary_signal();
    int touch_2 = get_binary_signal();
    int touch_3 = get_binary_signal();

    int element_1 = get_element_index_from_binary_values();
    show_element(element_1);
    trigger_element_action(element_1);

    int touch_1 = get_binary_signal();
    int touch_2 = get_binary_signal();
    int touch_3 = get_binary_signal();

    int element_2 = get_element_index_from_binary_values();
    show_element(element_2);
    trigger_element_action(element_2);

    send_element_data_over_serial(element_1, element_2);

    reset_time_series();
    waiting = 1;

  }

}

//char[ ] get_booleans_as_string(int[3] values){
//
//
//
//}

int get_element_index_from_binary_values(int touch_1, int touch_2, int touch_3) {
  // get a number between 1 and 8 from 3 binary values
  return 1 + touch_1 + touch_2 * 2 + touch_3 * 4;


void send_element_data_over_serial(int element_1, int element_2) {
  // Send two integers representing the elements, can be decoded by receiver
  Serial.print(0);
  Serial.print(element_1);
  Serial.print(element_2);
  Serial.println(9);
}

void detect_start(){
    // read from Teensy touch pin, 16 bit number from 0 to  65535 or 32766
    touchread = touchRead(TEENSY_TOUCH_PIN);
}

void reset_time_series(){
  sensor_count = 0;
}

int get_binary_signal(){
  get_time_series(6);

}

int get_time_series(int seconds){

  long max_diff_millis = seconds * 1000;
  long start_time = millis();
  long current_time = start_time;
  while(current_time - start_time < max_diff_millis){
    get_analog_value_and_add_to_time_series();
  }

}

void get_analog_value_and_add_to_time_series(){
    sensor_values[sensor_count] = analogRead(A0);
    sensor_count += 1;
}

boolean binary_from_time_series(int[] time_series) {

   //if (BIO_SIGNAL_ANALYSIS_TYPE == 2){}
     int random_pick = random(1);

     if (random_pick){
       return true;
     }
     else{
       return false;
     }
}
