/*
 * Code for Arduino Mega
 * Digital Pins Usable For Interrupts - 2, 3, 18, 19, 20, 21
 * 
 */

const int  switchPin1 = 20;
const int  switchPin2 = 21;
const int relayPin1 = 30;
const int relayPin2 = 31;
const int relayPin3 = 32;
const int relayPin4 = 33;
const int relayPin5 = 34;
const int relayPin6 = 35;
const int relayPin7 = 36;
const int relayPin8 = 37;

volatile int switchPinState1 = HIGH;
volatile int switchPinState2 = HIGH;

void setup() {
  pinMode(switchPin1, INPUT);
  pinMode(switchPin2, INPUT);
  pinMode(relayPin1, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  pinMode(relayPin3, OUTPUT);
  pinMode(relayPin4, OUTPUT);
  pinMode(relayPin5, OUTPUT);
  pinMode(relayPin6, OUTPUT);
  pinMode(relayPin7, OUTPUT);
  pinMode(relayPin8, OUTPUT);
  //attach interrupts
  attachInterrupt(digitalPinToInterrupt(switchPin1), handleSwitch1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(switchPin2), handleSwitch2, CHANGE);
  //set the relays off
  digitalWrite(relayPin1, HIGH);
  digitalWrite(relayPin2, HIGH);
  digitalWrite(relayPin3, HIGH);
  digitalWrite(relayPin4, HIGH);
  digitalWrite(relayPin5, HIGH);
  digitalWrite(relayPin6, HIGH);
  digitalWrite(relayPin7, HIGH);
  digitalWrite(relayPin8, HIGH);
  // initialize serial communication:
  Serial.begin(9600);
}

void loop() {
  digitalWrite(relayPin1, switchPinState1);
  digitalWrite(relayPin2, switchPinState2);
}

void handleSwitch1() {
  Serial.println("switching relay 1");
  switchPinState1 = !switchPinState1;
}

void handleSwitch2() {
  Serial.println("switching relay 2");
  switchPinState2 = !switchPinState2;
}
