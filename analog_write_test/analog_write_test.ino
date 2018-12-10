
// ----------------------------------
// --- PARAMETERS TO ADJUST ---------

// output pin to action
const int transistorPin_test = 13;

// set the value to write, between 0 and 255
int analog_write_value = 255;

// --- END PARAMETERS TO ADJUST -----
// ----------------------------------

void setup() {
  Serial.begin(9600); // set the baud rate
  pinMode(transistorPin_test, OUTPUT);
  analogWrite(transistorPin_test, 0);
}


// main loop
void loop()
{
  // cycle on and off
  analogWrite(transistorPin_test, analog_write_value);
  delay(2000);
  analogWrite(transistorPin_test, 0);
  delay(2000);

}
