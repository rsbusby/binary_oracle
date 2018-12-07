

void setup() {

  Serial.begin(9600);

  delay(2000); // 2 second delay for recovery

}

// main loop
void loop()
{

    // get random values from 1 to 8 for elements
    int element_1 = random(1,9);
    int reaction_1 = random(2);
    int element_2 = random(1,9);
    int reaction_2 = random(2);

    delay(2000);
    send_string_data_over_serial(element_1, 0);
    delay(3000);
    send_string_data_over_serial(reaction_1, 1);
    delay(3000);
    send_string_data_over_serial(element_2, 2);
    delay(3000);
    send_string_data_over_serial(reaction_2, 1);

}

void send_string_data_over_serial(int byte_value, int sequence_step) {
  // Send two integers representing the elements, can be decoded by receiver
  Serial.print("start");
  Serial.print(sequence_step);
  Serial.print(byte_value);
  Serial.println("end");
}
