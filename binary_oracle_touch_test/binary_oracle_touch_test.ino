#include "FastLED.h"

// array for sensor values
int sensor_values[600];
int sensor_count = 0;

//int value_count

//
int waiting = 1;
int start_detected = 0;
int start_time_in_millis;
int signal_finished = 0;
int signal;

boolean signal_detected_first = false;
int millis_between_start_detections = 300;

int sensor_time_seconds = 1;
long sensor_time_millis = sensor_time_seconds * 1000;
long max_diff_millis;
long current_time_in_millis;

// --- PARAMETERS TO ADJUST ---------

// Set the method of analyzing analog bio-signal
// 0 = Counting Peaks / Troughs
// 1 = Average Amplitude of Peaks vs/ Troughs
// 2 = mock random values
#define BIO_SIGNAL_ANALYSIS_TYPE 2

// how often we poll the bio-signal
int sensing_period_in_millis = 40;

// --- END PARAMETERS TO ADJUST -----
int SENSING_PERIOD_IN_MILLIS = 40;

void setup() {

  Serial.begin(9600);
  delay(1000); // 2 second delay for recovery
  Serial.println("Starting 07: ");

}

// main loop
void loop()
{
  
  EVERY_N_MILLISECONDS(SENSING_PERIOD_IN_MILLIS) {
    check_binary_signal();
    if (signal_finished){
      process_signal();
    }
  }

}

void process_signal(){
  Serial.print("Signal: ");
  Serial.println(signal);
  signal_finished = 0;
}

int get_element_index_from_binary_values(int touch_1, int touch_2, int touch_3) {
  // get a number between 1 and 8 from 3 binary values
  return 1 + touch_1 + touch_2 * 2 + touch_3 * 4;
}

void reset_time_series(){
  sensor_count = 0;
}

void reset_signal_detection(){
  reset_time_series();
  current_time_in_millis = millis();
  start_time_in_millis = millis();
}

int check_start(){

  // if not done listening, record and keep going
  if(current_time_in_millis - start_time_in_millis < max_diff_millis){
    get_analog_value_and_add_to_time_series();
    current_time_in_millis = millis();
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
    signal_detected_first = 1;
    reset_signal_detection();
    return 0;
  }
}

void check_binary_signal(){

  if (waiting == 1){
    int started = check_start();
    if (started){

      Serial.println("Detected signal!");
      waiting = 0;

      // set up for signal recording
      // don't reset sensor data, keep the last period
      // Just increase the time to record
      // start_time_in_millis = millis();
      // current_time_in_millis = millis();
      max_diff_millis = sensor_time_millis;

    }
  }
  else{
  // waiting = 0  , recording
    if(current_time_in_millis - start_time_in_millis < max_diff_millis){
      get_analog_value_and_add_to_time_series();
      current_time_in_millis = millis();
    }
    else{
      // done listening, set up for start detection
      waiting = 1;
      max_diff_millis = millis_between_start_detections;
      start_time_in_millis = millis();
      current_time_in_millis = millis();

      // get binary signal value
      signal_finished = 1;
      signal = get_binary_from_time_series();
    }
  }
}

void get_analog_value_and_add_to_time_series(){
    sensor_values[sensor_count] = analogRead(A0);
    sensor_count += 1;
}

void get_average_and_count_of_time_series(){

}

int detect_signal_in_time_series(){
// TODO: implement, above threshold min/max
  return random(2);
}

int get_binary_from_time_series() {

   if (BIO_SIGNAL_ANALYSIS_TYPE == 2){
     int random_pick = random(2);

     if (random_pick){
       return 1;
     }
     else{
       return 0;
     }
   }
   else{
     // TODO: types 0 and 1
     return random(2);
   }
}
