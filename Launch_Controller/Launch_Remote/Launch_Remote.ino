#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>

//Define button pins
#define CHNL1 2
#define CHNL2 3
#define CONT 4
#define ARM 5
#define FIRE 6
#define LED_CONT 7
#define LED_FIRE 8

// Define Radio Pins
#define RX 10 // Receiver
#define TX 11 // Transmit

// Setup Serial Radio
SoftwareSerial HC12(TX, RX); // HC-12 TX Pin, HC-12 RX Pin

//Define variables
bool armed = false; // Armed State
bool downlink = false; // Connection State
int channel = 0; // Which channel is currently being used?
int led_delay = 2000; // How long do the LEDs blink? (ms)
int comm_delay = 100; // How long does the HC-12 delay between read/write commands (ms)
int LCD_columns = 16;
int LCD_rows = 2;
int disconnect_counter = 0; // How many times has the controller not recieved data
int max_disconnect = 200; // What is the number of loops it can not recieve before trying to reconnect.
byte incomingByte; // HC-12 Incoming Byte
byte outgoingByte; // HC-12 Outgoing Byte
String command = ""; // Command to send to controller
String reply = ""; // Reply from controller
String disp = ""; // Display line to LCD

// Define LCD Setup
LiquidCrystal_I2C lcd(0x27,LCD_columns,LCD_rows);  // set the LCD address to 0x27 for a 16 chars and 2 line display

byte Connection[] = { // Custom connection icon
  B01110,
  B10001,
  B00100,
  B01010,
  B00000,
  B00100,
  B00000,
  B11111
};

void setup() {
  Serial.begin(9600); // Initialize Serial Monitor
  HC12.begin(9600); // Initialize Radio

  lcd.init();         // Initialize LCD 
  lcd.backlight();

  disp = "Power On";
  write_LCD();
  
  // Setup Input Pins
  pinMode(CHNL1, INPUT);
  pinMode(CHNL2, INPUT);
  pinMode(ARM, INPUT);
  pinMode(CONT, INPUT);
  pinMode(FIRE, INPUT);

  // Setup Output Pins
  pinMode(LED_CONT, OUTPUT);
  pinMode(LED_FIRE, OUTPUT);

  lcd.createChar(0, Connection);

  get_downlink();
}

void loop() {
  // Check if channel has been updated
  if (digitalRead(CHNL1) == HIGH && channel != 1){ // Switch is on Channel 1
    channel = 1;
  }
  else if (digitalRead(CHNL2) == HIGH && channel != 2){ // Switch is on Channel 2
    channel = 2;
  }
  else if (digitalRead(CHNL1) == LOW && digitalRead(CHNL2) == LOW  && channel != 0){ // Switch is set to 0 or an invalid channel
    channel = 0;
  }

  // Check for commands to be sent to the controller
  if (digitalRead(ARM) == LOW && armed == true){ // Arm Switch Disengaged, currently armed
    command = "DARM"; // Set Command
    armed = false; // Mark disarmed
  }
  else if (channel != 0){
    if (digitalRead(ARM) == HIGH && armed == false){ // Arm Switch Engaged, currently disarmed
      command = "ARM"; // Set Command
      armed = true; // Marked arm
    }
    else if (digitalRead(CONT) == HIGH){ // Continuity Check Button Pressed
      command = "CONT" + String(channel); // Set Command
    }
    else if (digitalRead(FIRE) == HIGH && armed == true){ // Fire Button Pressed, and armed
      // Flash LED to Confirm Fire
      command = "FIRE" + String(channel);
    }
  }
  else{ // No Input
    command = "";
  }
  
  Serial.println(command);
  comm_remote(); // Send commands to and read replies from controller

  Serial.println(disconnect_counter);

  if (disconnect_counter >= max_disconnect){ // If the controller has not recieved data from remote in a while, attempt to reconnect.
    downlink = false;
    get_downlink();
  }

  Serial.println(reply);
  reply_actions(); // Check what the reply action requires

  command = ""; // Reset command to empty
  reply = ""; // Reset reply to empty.

  write_LCD(); // Set LCD display
}

void reply_actions(){
  if (reply == "NCONT"){
    disp = "Continutity not detected"; // Set display
  }
  else if (reply == "CONT"){
    // Flash LED to Confirm Continuity
    digitalWrite(LED_CONT,HIGH);
    delay(led_delay);
    digitalWrite(LED_CONT,LOW);
    
    disp = "Continutity detected"; // Set display
  }
  else if (reply == "ARMD"){
    digitalWrite(LED_FIRE,HIGH);
    disp = "Armed"; // Set display
  }
  else if (reply == "DARMD"){
    digitalWrite(LED_FIRE,LOW); // Turn off fire button LED
    disp = "Disarmed"; // Set display
  }
  else if (reply == "FIRED"){
    // Flash LED to Confirm Fire
    digitalWrite(LED_FIRE,LOW);
    delay(led_delay);
    digitalWrite(LED_FIRE,HIGH);

    disp = "Motor Fired"; // Set display
  }
  else{
    disp = ""; // Set display
  }
}

void recieve_data(){
  while (HC12.available()){ // If data available from Remote
    incomingByte = HC12.read(); // Read reply
    reply += char(incomingByte); // Update reply with incoming byte
  }

  if (reply == ""){ // If no reply recieved
    disconnect_counter++; // Increment reply counter
  }
  else{
    disconnect_counter = 0; // Reset counter
  }
}

void send_data(){
  if (command != ""){ // If ready to send command
    HC12.print(command);
  }

  while (Serial.available()) { // If Serial Line is being used
    HC12.write(Serial.read()); // Send commands from Serial Line
  }
}

void comm_remote(){
  recieve_data();
  delay(comm_delay); // Wait long enough for controller to recieve communciation

  send_data();
  delay(comm_delay); // Wait long enough for controller to recieve communciation
}

void get_downlink(){
  if (armed == true){
    digitalWrite(LED_FIRE,LOW);
  }

  disp = "Connecting";
  write_LCD();
  while (downlink == false){
    command = "HAND";
    comm_remote();
    command = ""; // Reset command to empty.
    
    if (reply == "SHAKE"){
      downlink = true;
      command = "CONF";
      comm_remote();
    }

    reply = ""; // Reset reply to empty
  }

  if (armed == true){
    digitalWrite(LED_FIRE,HIGH);
  }

  disconnect_counter = 0;
  Serial.println("Connected");
  disp = "Connected";
  write_LCD();
}

void write_LCD(){
  if (disp != ""){ // If ready to display text
    if (disp.length() <= LCD_columns){ // If the length of the display text is less than the size of the LCD display
      lcd.clear(); // Clear screen
      lcd.setCursor(0,0); // Set cursor to the end
      lcd.print(disp); // Print the display text to the LCD
      status_disp(); // Display the status of the remote
    }
    else{
      disp = disp + " ";
      for (int pos = 0; pos < (disp.length()-LCD_columns); pos++) {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(disp.substring(pos, pos + LCD_columns));
        status_disp();
        delay(300);
      }
    }
  }
  else{
    status_disp();
  }

  disp = "";
}
  
void status_disp(){
  lcd.setCursor(0,1);
  if (channel != 0){
    lcd.print("CH 0" + String(channel));
  }
  else{
    lcd.print("NO CH");
  }

  if (downlink == true){
    lcd.setCursor(LCD_columns-1,1);
    lcd.write(0);
  }
}
