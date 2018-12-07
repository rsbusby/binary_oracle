// use FastLED for timing
#include "FastLED.h"

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
int sensing_period_in_millis = 40;

// seconds for sensing
int sensor_time_seconds = 3;

// time to record sensor values when waiting for signal (takes 2 to start)
int millis_between_start_detections = 280;

// low and high threshold for a signal to be detected
int lo_signal_threshold = 150;
int hi_signal_threshold = 780;

// --- END PARAMETERS TO ADJUST -----
// ----------------------------------

// array for sensor values
int sensor_values[600];
int sensor_count = 0;

int waiting = 1;
int start_detected = 0;
int start_time_in_millis;
int signal_finished = 0;
int signal;

boolean signal_detected_first = false;

long sensor_time_millis = sensor_time_seconds * 1000;
long max_diff_millis;
long current_time_in_millis;


void setup() {
  Serial.begin(9600);
  delay(1000); // 2 second delay for recovery
  Serial.println("Starting 07: ");
}


// main loop
void loop()
{

  EVERY_N_MILLISECONDS(sensing_period_in_millis) {
    check_binary_signal();
    if (signal_finished){
      process_signal();
    }
  }

}


void process_signal(){
  // just output to serial for testing
  Serial.print("Signal: ");
  Serial.println(signal);
  signal_finished = 0;
}


void get_analog_value_and_add_to_time_series(){
    sensor_values[sensor_count] = analogRead(A0);
    sensor_count += 1;
}


void reset_time_series(){
  sensor_count = 0;
}


// ---------------------------
// main control function for sensing
// changes program state depending on recent sensor values and time elapsed
void check_binary_signal(){

  if (waiting == 1){
    int started = check_start();
    if (started){
      Serial.println("Detected signal!");
      waiting = 0;
      // set up for signal recording
      max_diff_millis = sensor_time_millis;
    }
  }
  else{
  // waiting = 0, recording
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

// -----------------------------
// functions to detect start of signal
int min_sensor_val(){
  int min_val = 0;
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
