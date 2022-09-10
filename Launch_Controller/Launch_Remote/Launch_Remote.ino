//Define button pins
#define CHNL 2
#define ARM 3
#define CONT 4
#define FIRE 5
#define LED_ARM 6
#define LED_CONT 7
#define LED_FIRE 8

//Define variables
bool armed = false; // Armed State
int channel = 0;

void setup() {
  Serial.begin(9600); // Initialize Serial Monitor
  
  // Setup Input Pins
  pinMode(CHNL, INPUT);
  pinMode(ARM, INPUT);
  pinMode(CONT, INPUT);
  pinMode(FIRE, INPUT);

  pinMode(LED_ARM, OUTPUT);
  pinMode(LED_CONT, OUTPUT);
  pinMode(LED_FIRE, OUTPUT);
}

void loop() {
  if (digitalRead(CHNL) == HIGH && channel != 1){
    Serial.println("Channel 1"); // Debug Line
    channel = 1;
  }
  else if (digitalRead(CHNL) == LOW && channel != 2){
    Serial.println("Channel 1"); // Debug Line
    channel = 2;
  }
  if (digitalRead(ARM) == HIGH && armed == false){
    Serial.println("ARM" + String(channel)); // Debug Line
    armed = true; // Marked arm
  }
  else if (digitalRead(ARM) == LOW && armed == true){
    Serial.println("DEARM" + String(channel)); // Debug Line
    armed = false; // Mark disarmed
  }
  if (digitalRead(CONT) == HIGH){
    Serial.println("CONT" + String(channel)); // Debug Line
    digitalWrite(LED_CONT,HIGH);
    delay(2000);
    digitalWrite(LED_CONT,LOW);
  }
  if (digitalRead(FIRE) == HIGH){
    Serial.println("FIRE" + String(channel)); // Debug Line
    digitalWrite(LED_FIRE,HIGH);
    delay(2000);
    digitalWrite(LED_FIRE,LOW);
  }
  delay(200);
}
