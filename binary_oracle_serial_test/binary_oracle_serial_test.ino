

void setup() {

  Serial.begin(9600);

  delay(2000); // 2 second delay for recovery

}

// main loop
void loop()
{

    // get random values from 1 to 8
    int element_1 = random(1,9);
    int element_2 = random(1,9);

    send_element_data_over_serial(element_1, element_2);

    delay(4000);

}

void send_element_data_over_serial(int element_1, int element_2) {
  // Send two integers representing the elements, can be decoded by receiver
  Serial.print(0);
  Serial.print(element_1);
  Serial.print(element_2);
  Serial.println(9);
}
