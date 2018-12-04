
// array for sensor values
int sensor_values[600];
int sensor_count = 0;

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
  delay(1000); // 2 second delay for recovery

}

// main loop
void loop()
{

    Serial.println("Getting touch signal over a few seconds");

    int touch_1 = get_binary_signal(6);
    Serial.print("Signal: ");
    Serial.println(touch_1);


}

int get_element_index_from_binary_values(int touch_1, int touch_2, int touch_3) {
  // get a number between 1 and 8 from 3 binary values
  return 1 + touch_1 + touch_2 * 2 + touch_3 * 4;
}

void send_element_data_over_serial(int element_1, int element_2) {
  // Send two integers representing the elements, can be decoded by receiver
  Serial.print(0);
  Serial.print(element_1);
  Serial.print(element_2);
  Serial.println(9);
}
//
// void detect_start(){
//     // read from Teensy touch pin, 16 bit number from 0 to  65535 or 32766
//     touchread = touchRead(TEENSY_TOUCH_PIN);
// }

void reset_time_series(){
  sensor_count = 0;
}

boolean get_binary_signal(int seconds){

  long max_diff_millis = seconds * 1000;
  long start_time = millis();
  long current_time = start_time;
  while(current_time - start_time < max_diff_millis){
    get_analog_value_and_add_to_time_series();
  }
  return get_binary_from_time_series();
}

void get_analog_value_and_add_to_time_series(){
    sensor_values[sensor_count] = analogRead(A0);
    sensor_count += 1;
}

int get_binary_from_time_series() {

   //if (BIO_SIGNAL_ANALYSIS_TYPE == 2){}
     int random_pick = random(1);

     if (random_pick){
       return 1;
     }
     else{
       return 0;
     }
}
