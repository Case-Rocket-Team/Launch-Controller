// Define relay trigger pins
#define RLY1 2 // Buzzer
#define RLY2 5 // E-Match 2
#define RLY3 6 // E-Match 1
#define RLY4 7 // Safety Arm

// Define continutity check pins
#define CHK1 4 // E-Match 1
#define CHK2 3 // E-Match 2

// Define Sensor Pin
#define SNSR A0

// Define Variables
int sensorValue = 0;
float voltage = 0.00;
const float threshold_volt = 0.75; // Threshold voltage for continutity check
const int max_mess_length = 6; // Set maximum command message length
bool armed = false; // Armed state set to false by default
int buzzer_delay = 500; // How long is the buzzer allowed to run (ms)
int fire_delay = 2000; // How long is the e-match fire allowed to run (ms)
int cont_delay = 50; // How long does the continuity check wait to allow current to flow (ms)
int check_pin = 0; // Which pin is being used to check e-match
int fire_pin = 0; // Which pin is being used to fire e-match
String reply = ""; // Response code for remote

void setup() {
  Serial.begin(9600); // Initialize Serial Monitor
  
  // Setup Output Pins
  pinMode(RLY1, OUTPUT);
  pinMode(RLY2, OUTPUT);
  pinMode(RLY3, OUTPUT);
  pinMode(RLY4, OUTPUT);
  pinMode(CHK1, OUTPUT);
  pinMode(CHK2, OUTPUT);

  // Hard-set all outputs to LOW
  digitalWrite(RLY1, LOW);
  digitalWrite(RLY2, LOW);
  digitalWrite(RLY3, LOW);
  digitalWrite(RLY4, LOW);
  digitalWrite(CHK1, LOW);
  digitalWrite(CHK2, LOW);
}

void loop() {
  while (Serial.available() == 0){} // Wait for incoming commands
  String command = Serial.readString(); // Read incoming command
  command.trim(); // Format incoming command

  // Serial.println(command); // Debug line

  if (command == "CONT1" || command == "CONT2"){ // Continuity Check Command
    check_cont(command);
  }
  else if (command == "ARM"){ // Arming Command
    // Serial.println("Arming fire"); // Debug line
    digitalWrite(RLY4, HIGH); // Engage arming relay
    armed = true; // Mark system as armed
    reply = "ARMD";
  }
  else if (command == "DARM"){ // Disarming Command
    digitalWrite(RLY4, LOW); // Turn off Arming Relay
    armed = false;
    reply = "DARMD";
  }
  else if (command == "FIRE1" || command == "FIRE2"){ // Fire Command
    fire_rocket(command);
  }
  else { // No Command
    reply = "";
  }

  if (reply != ""){
    Serial.println(reply);
  }
}

void get_voltage(){
  sensorValue = analogRead(SNSR);
  // Serial.println(sensorValue); // Debug line
  voltage = sensorValue * (5.0 / 1023.0);
}

void check_cont(String cmnd){
  if (cmnd == "CONT1"){ // Pick correct e-match test channel
    check_pin = CHK1;
  }
  else if (cmnd == "CONT2"){
    check_pin = CHK2;
  }
  Serial.println("Continutity Check");
  digitalWrite(check_pin, HIGH); // Run current through the e-match
  delay(cont_delay);
  get_voltage(); // Read and record voltage
  Serial.print("Voltage: "); // Print voltage to Serial monitor
  Serial.println(voltage);
  digitalWrite(check_pin, LOW); // Turn off test-current

  if (voltage >= threshold_volt){ // Check for minimum voltage requirements
    // Serial.println("Buzzer"); // Debug line
    digitalWrite(RLY1, HIGH); // Turn buzzer on for a short duration
    delay(buzzer_delay); // Time is arbitrary
    digitalWrite(RLY1, LOW);
    reply = "CONT";
  }
  else{ // Voltage minimum not met
    // Serial.println("Continutity not detected"); // Debug line
    reply = "NCONT";
  }
}

void fire_rocket(String cmnd){
  if (cmnd == "FIRE1"){ // Pick correct fire channel
    fire_pin = RLY3;
  }
  else if (cmnd == "FIRE2"){
    fire_pin = RLY2;
  }
  if (armed == true){ // Fire can only occur if the controller is armed
    // Serial.println("Firing Channel 1"); // Debug line
    digitalWrite(fire_pin, HIGH);
    delay(fire_delay);
    digitalWrite(fire_pin, LOW);
    reply = "FIRED";
    // Serial.println("End Fire"); // Debug line
  }
  else{
    // Serial.println("Controller is not armed"); // Debug line
    reply = "NARMD";
  }
}
