#include <SoftwareSerial.h>

// Define relay trigger pins
#define RLY1 2 // E-Match 1
#define RLY2 3 // E-Match 2
#define RLY3 4 // Safety Arm
#define RLY4 5 // Buzzer

// Define continutity check pins
#define CHK1 6 // E-Match 1
#define CHK2 7 // E-Match 2

// Define Sensor Pin
#define SNSR A0

// Define Radio Pins
#define RX 10 // Recieve
#define TX 11 // Transmit

// Setup Serial Radio
SoftwareSerial HC12(TX, RX); // HC-12 TX Pin, HC-12 RX Pin

// Define Variables
bool armed = false; // Armed state set to false by default
bool downlink = false; // Connection to Remote
int buzzer_delay = 500; // How long is the buzzer allowed to run (ms)
int fire_delay = 2000; // How long is the e-match fire allowed to run (ms)
int cont_delay = 50; // How long does the continuity check wait to allow current to flow (ms)
int comm_delay = 100; // How long does the HC-12 delay between read/write commands (ms)
int check_pin = 0; // Which pin is being used to check e-match
int fire_pin = 0; // Which pin is being used to fire e-match
int sensorValue = 0;
float voltage = 0.00;
const float threshold_volt = 0.75; // Threshold voltage for continutity check
int disconnect_counter = 0; // How many times has the controller not recieved data
int max_disconnect = 300; // What is the number of loops it can not recieve before trying to reconnect.
byte incomingByte; // HC-12 Incoming Byte
String command = ""; // Remote command
String reply = ""; // Response code for remote

void setup() {
  Serial.begin(9600); // Initialize Serial Monitor
  HC12.begin(9600); // Initialize Radio
  
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
  comm_controller();
  Serial.println(command);
  Serial.println(disconnect_counter);

  reply = ""; // Reset reply to empty

  command_actions(); // Check what the command action requires
  Serial.println(reply);
  
  command = ""; // Reset command to empty.
}

void get_voltage(){
  sensorValue = analogRead(SNSR); // Read Sensor Pin
  voltage = sensorValue * (5.0 / 1023.0);
}

void check_cont(String cmnd){
  if (cmnd == "CONT1"){ // Pick correct e-match test channel
    check_pin = CHK1;
  }
  else if (cmnd == "CONT2"){
    check_pin = CHK2;
  }

  // Turn off arming relay to run continuity check
  if (armed == true){
    digitalWrite(RLY3, LOW); // Turn off Arming Relay
  }

  // Check for continuity
  digitalWrite(check_pin, HIGH); // Run current through the e-match
  delay(cont_delay); // Wait for relays to fully open
  get_voltage(); // Read and record voltage
  digitalWrite(check_pin, LOW); // Turn off test-current

  // Send continuty check results
  if (voltage >= threshold_volt){ // Check for minimum voltage requirements
    digitalWrite(RLY4, HIGH); // Turn buzzer on for a short duration
    delay(buzzer_delay); // Time is arbitrary
    digitalWrite(RLY4, LOW);
    reply = "CONT"; // Set reply
  }
  else{ // Voltage minimum not met
    reply = "NCONT"; // Set reply
  }
  
  // Turn back on arming relay if armed
  if (armed == true){
    digitalWrite(RLY3, HIGH); // Turn off Arming Relay
  }
}

void fire_rocket(String cmnd){
  if (cmnd == "FIRE1"){ // Pick correct fire channel
    fire_pin = RLY1;
  }
  else if (cmnd == "FIRE2"){
    fire_pin = RLY2;
  }
  if (armed == true){ // Fire can only occur if the controller is armed
    digitalWrite(fire_pin, HIGH);
    delay(fire_delay);
    digitalWrite(fire_pin, LOW);
    reply = "FIRED"; // Set reply
  }
}

void command_actions(){
  if (command == "HAND" || disconnect_counter >= max_disconnect){
    downlink = false;
    reply = "SHAKE";
    if (armed == true){
      digitalWrite(RLY3, LOW); // Turn off Arming Relay
    }
  }
  else if (command == "CONF"){
    downlink = true;
    reply = "CONF";
    if (armed == true){
      digitalWrite(RLY3, HIGH); // Turn On Arming Relay
    }
    Serial.println("Connected");
  }
  else if (command == "CONT1" || command == "CONT2"){ // Continuity Check Command
    check_cont(command);
  }
  else if (command == "ARM"){ // Arming Command
    digitalWrite(RLY3, HIGH); // Engage arming relay
    armed = true; // Mark system as armed
    reply = "ARMD";
  }
  else if (command == "DARM"){ // Disarming Command
    digitalWrite(RLY3, LOW); // Turn off Arming Relay
    armed = false;
    reply = "DARMD";
  }
  else if (command == "FIRE1" || command == "FIRE2"){ // Fire Command
    fire_rocket(command);
  }
  else { // No Command
    reply = "";
  }
}

void recieve_data(){
  while (HC12.available()){ // If data available from Remote
    incomingByte = HC12.read(); // Read command
    command += char(incomingByte); // Update command with incoming byte
  }

  if (command == ""){ // If no reply recieved
    disconnect_counter++; // Increment reply counter
  }
  else{
    disconnect_counter = 0; // Reset counter
  }
}

void send_data(){
  if (reply != ""){ // If ready to send command
    HC12.print(reply);
  }

  while (Serial.available()) { // If Serial Line is being used
    HC12.write(Serial.read()); // Send commands from Serial Line
  }
}

void comm_controller(){
  recieve_data();
  delay(comm_delay); // Wait long enough for controller to recieve communciation

  send_data();
  delay(comm_delay); // Wait long enough for controller to recieve communciation
}
