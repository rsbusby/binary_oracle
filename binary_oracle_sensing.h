#define NUM_SENSOR_VALUES 400

// sensing from the analogInput pin for the binary Oracle
class BinaryOracleSensor
{

      // to be private?
      // array for sensor values
      uint16_t sensor_values[NUM_SENSOR_VALUES];
      uint16_t sensor_count = 0;

      uint8_t start_detected = 0;
      unsigned long start_time_in_millis;

      boolean signal_detected_first = true;

      unsigned long current_time_in_millis;

public:

  uint8_t signal_finished = 0;
  uint8_t signal;

  uint16_t sensor_value;
  uint8_t waiting;

  unsigned long max_diff_millis;

  // how often we poll the bio-signal
  // int sensing_period_in_millis = 100;

  // milliseconds for sensing and recording
  unsigned long sensor_time_millis = 2000;

  // time to record sensor values when waiting for signal (takes 2 to start)
  int millis_between_start_detections = 280;

  // low and high threshold for a signal to be detected
  uint16_t lo_signal_threshold = 180;
  uint16_t hi_signal_threshold = 600;

  boolean show_sensor_value = 0;
  boolean simulated_data = 0;

  // Set the method of analyzing analog bio-signal
  // 0 = Counting Peaks / Troughs
  // 1 = Average Amplitude of Peaks vs/ Troughs
  // 2 = mock random values
  uint8_t bio_signal_analysis_type = 0;

  boolean debug = 1;

  // Constructor - creates a BinaryOracleSensor
 // and initializes the member variables and state
 BinaryOracleSensor(int sensor_time_millis, int hi_threshold, int lo_threshold, int signal_analysis_type){
   signal_finished = 0;
   current_time_in_millis = millis();
   start_time_in_millis = millis();
   waiting = 1;
   max_diff_millis = millis_between_start_detections;

   sensor_time_millis = sensor_time_millis;
   lo_signal_threshold = lo_threshold;
   hi_signal_threshold = hi_threshold;
   bio_signal_analysis_type = signal_analysis_type;
 }


 // ---------------------------
 // main control function for sensing
 // changes program state depending on recent sensor values and time elapsed
 void check_binary_signal(){

   current_time_in_millis = millis();

   if (waiting == 1){
     uint8_t started = check_start();
     if (started){
       if (debug){
         Serial.println("          ++++ Detected touch ++++");
       }
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

       // get binary signal value
       signal_finished = 1;

       if(debug && 0){
         Serial.print("sensor count: ");
         Serial.println(sensor_count);
       }
       signal = get_binary_from_time_series();

       reset_signal_detection();


     }
   }
 }

private:


// ---------------------------------------------
// signal detection and processing
void get_analog_value_and_add_to_time_series(){

    sensor_value = analogRead(A0);
    sensor_values[sensor_count] = sensor_value;
    sensor_count += 1;
    if (sensor_count == NUM_SENSOR_VALUES){
      if(debug && 0){
        Serial.println("OUT OF MEMORY");
      }
      sensor_count = 0;
    }

    if (show_sensor_value and debug){
      Serial.print("  -- sensor input: ");
      Serial.println(sensor_value);
    }

}

void reset_time_series(){
  sensor_count = 0;
}

// -----------------------------
// functions to detect start of signal
uint16_t min_sensor_val(){
  uint16_t min_val = 1023;
  for (uint16_t i=0; i < sensor_count;i++ ){
    if ( sensor_values[i] < min_val ){
      min_val = sensor_values[i];
    }
  }
  return min_val;
}


uint16_t max_sensor_val(){
  uint16_t max_val = 0;
  for (uint16_t i=0; i < sensor_count;i++ ){
    if ( sensor_values[i] > max_val ){
      max_val = sensor_values[i];
    }
  }
  return max_val;
}


uint8_t detect_signal_in_time_series(){
  // check values in sensor_values to see if any past threshold

  if(simulated_data){
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

  current_time_in_millis = millis();

  // if not done listening, record and keep going
  if(current_time_in_millis - start_time_in_millis < max_diff_millis){
    get_analog_value_and_add_to_time_series();
    return 0;
  }

  // done listening for start
  int signal_detected = detect_signal_in_time_series();

  if ( ! signal_detected ){
    signal_detected_first = 1;
    reset_signal_detection();
    return 0;
  }
  if (signal_detected_first){
      signal_detected_first = 1;
      // don't reset sensor data, keep the last period
      return 1;
  }

  return 0;

  // else{
  //   // detected a signal, but will wait one more cycle
  //   if (debug){
  //     Serial.println("          ++ Possible touch, debouncing ++");
  //   }
  //   signal_detected_first = 1;
  //   reset_signal_detection();
  //   return 0;
  // }
}


// --------------------------------------------
// functions to determine binary signal value
int get_mean(){
  int sum = 0;
  for (uint16_t i=0; i < sensor_count;i++ ){
    sum += sensor_values[i];
  }
  // TODO: would this be faster without float conversion?
  return int(sum / float(sensor_count));

}

int compare_troughs_and_peaks(int ref_val){
  // compare number of values above and below a reference value
  int lo_count = 0;
  int hi_count = 0;

  for (uint16_t i=0; i < sensor_count;i++ ){
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

   if (bio_signal_analysis_type == 2 || simulated_data){
     // fake it
     return random(2);
   }
   else if (bio_signal_analysis_type == 1){
     // TODO: type 1
     return random(2);
   }
   else{
     // BIO_SIGNAL_ANALYSIS_TYPE == 0
     return get_binary_signal_from_counts();
   }
}

// ------ end sensor detection and processing
};


//
// //
// //
// // // -----------------------------
// // // functions to detect start of signal
// // int min_sensor_val();
// // int max_sensor_val();
// // int detect_signal_in_time_series();
// // void reset_signal_detection();
// // int check_start();
// //
// // // --------------------------------------------
// // // functions to determine binary signal value
// // int get_mean();
// // int compare_troughs_and_peaks(int ref_val);
// // int get_binary_signal_from_counts();
// // int get_binary_from_time_series();
