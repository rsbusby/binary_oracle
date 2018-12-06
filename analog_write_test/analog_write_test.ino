
// ----------------------------------
// --- PARAMETERS TO ADJUST ---------

const int transistorPin_test = 9;

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
  analogWrite(transistorPin_test, 255);
  delay(2000);
  analogWrite(transistorPin_test, 0);
  delay(2000);

}
