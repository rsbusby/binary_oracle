
// ----------------------------------
// --- PARAMETERS TO ADJUST ---------

// output pin to action
const int transistorPin_test = 17;
const int transistorPin2_test = 19;
const int transistorPin3_test = 18;
// set the value to write, between 0 and 255
int analog_write_value = 255;

// --- END PARAMETERS TO ADJUST -----
// ----------------------------------

void setup() {
  Serial.begin(9600); // set the baud rate
  pinMode(transistorPin_test, OUTPUT);
  analogWrite(transistorPin_test, 0);
  pinMode(transistorPin2_test, OUTPUT);
  analogWrite(transistorPin2_test, 0);
  pinMode(transistorPin3_test, OUTPUT);
  analogWrite(transistorPin3_test, 0);
}


// main loop
void loop()
{
//  // cycle on and off
//  analogWrite(transistorPin_test, analog_write_value);
//  Serial.println("On");
//  delay(3000);
//  analogWrite(transistorPin_test, 0);
//  Serial.println("Off");
//  delay(1000);

  // cycle on and off
  analogWrite(transistorPin2_test, analog_write_value);
  Serial.println("On");
  delay(500);
  analogWrite(transistorPin2_test, 0);
  Serial.println("Off");
  delay(500);
//
//    // cycle on and off
//  analogWrite(transistorPin3_test, analog_write_value);
//  Serial.println("On");
//  delay(3000);
//  analogWrite(transistorPin3_test, 0);
//  Serial.println("Off");
//  delay(6000);
}
