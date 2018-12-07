

void setup() {

  Serial.begin(9600);

  delay(2000); // 2 second delay for recovery

}

// main loop
void loop()
{

    // get random values 
    delay(2000);
    send_string_data_over_serial(1, random(2));
    send_string_data_over_serial(1, random(2));
    send_string_data_over_serial(3, random(2));
    delay(3000);
    send_string_data_over_serial(4, random(2));
    delay(3000);

    send_string_data_over_serial(5, random(2));
    send_string_data_over_serial(6, random(2));
    send_string_data_over_serial(7, random(2));
    delay(3000);
    send_string_data_over_serial(8, random(2));

}

void send_string_data_over_serial(int byte_value, int sequence_step) {
  // Send two integers representing the elements, can be decoded by receiver
  Serial.print(sequence_step);
  Serial.print(byte_value);
}
