// test a passthru between USB and hardware serial
     
void setup() {
  while (!Serial);
  Serial.begin(9600);
  Serial1.begin(9600);
}

     
void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    Serial1.write(c);
  }
  if (Serial1.available()) {
    char c = Serial1.read();
    Serial.write(c);
  }
}
