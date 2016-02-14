/*
 * Code for Arduino Mega
 * Digital Pins Usable For Interrupts - 2, 3, 18, 19, 20, 21
 * 
 */

#include <SPI.h>
#include <Ethernet.h>
#include <EventManager.h>
#include <OneWire.h>
#include <DallasTemperature.h>

static unsigned long lastMillis = 0; // holds the last read millis()
static int timer_1 = 0; // a repeating timer max time 32768 mS = 32sec use a long if you need a longer timer
// NOTE timer MUST be a signed number int or long as the code relies on timer_1 being able to be negative
// NOTE timer_1 is a signed int
#define TIMER_INTERVAL_1 10000
#define ONE_WIRE_BUS 2

//debouncing in interrupts
long debouncing_time = 250; //Debouncing Time in Milliseconds
volatile unsigned long last_micros;

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

const int temperaturePin = 0;

volatile int switchPinState1 = HIGH;
volatile int switchPinState2 = HIGH;
volatile bool processing = false;
int switchPinState3 = HIGH;
int switchPinState4 = HIGH;
int switchPinState5 = HIGH;
int switchPinState6 = HIGH;
int switchPinState7 = HIGH;
int switchPinState8 = HIGH;

float currentTemperature = 0;

//set up ethernet server stuff
byte ip[] = { 192, 168, 1, 177 };   // IP Address
byte subnet[] = { 255, 255, 255, 0 }; // Subnet Mask
byte gateway[] = { 192, 168, 1, 1 }; // Gateway
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // MAC Address

EthernetServer server = EthernetServer(80); // Port 80
EthernetClient reportClient;
String HTTPget = "";

char mainServer[] = "192.168.1.12"; 

EventManager gMyEventManager;

//one wire temp sensor DS18B20+
// Setup a oneWire instance to communicate with any OneWire devices 
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
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
  attachInterrupt(digitalPinToInterrupt(switchPin1), handleSwitch1, RISING);
  attachInterrupt(digitalPinToInterrupt(switchPin2), handleSwitch2, RISING);
  //set the relays off
  digitalWrite(relayPin1, HIGH);
  digitalWrite(relayPin2, HIGH);
  digitalWrite(relayPin3, HIGH);
  digitalWrite(relayPin4, HIGH);
  digitalWrite(relayPin5, HIGH);
  digitalWrite(relayPin6, HIGH);
  digitalWrite(relayPin7, HIGH);
  digitalWrite(relayPin8, HIGH);

  lastMillis = millis();

  gMyEventManager.addListener( EventManager::kEventUser0, switchEventsListener );
  
  Ethernet.begin(mac, ip, gateway, subnet); // setup ethernet with params from above
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  //start up temp sensor
  sensors.begin();
}

void loop() {
  //handle web requests
  handlWebRequests();

  //keep the relay state
  digitalWrite(relayPin1, switchPinState1);
  digitalWrite(relayPin2, switchPinState2);
  digitalWrite(relayPin3, switchPinState3);
  digitalWrite(relayPin4, switchPinState4);
  digitalWrite(relayPin5, switchPinState5);
  digitalWrite(relayPin6, switchPinState6);
  digitalWrite(relayPin7, switchPinState7);
  digitalWrite(relayPin8, switchPinState8);

  //process events in the queue
  gMyEventManager.processEvent();
  
  //handle timer events
  handleTimer();
}

float readTemperature(){
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.print("Temperature for Device 1 is: ");
  Serial.println(sensors.getTempCByIndex(0));
  return sensors.getTempCByIndex(0);
}

void handlWebRequests(){
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) 
  {
    // send http reponse header
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println();
    // process request.
    processClient(client);
  }
}
void processClient(EthernetClient client)
{
  // http request will end with a blank line
  boolean lineIsBlank = true;
  while (client.connected())
  {
    if (client.available())
    {
      char c = client.read();
      HTTPget += c;
      if (c == '\n' && lineIsBlank)  break;
      if (c == '\n')
      {
        lineIsBlank = true;
      }
      else if (c != '\r')
      {
        lineIsBlank = false;
      }
    }
  }
  client.print(processRequest(HTTPget));
  delay(1); // give the web browser a moment to recieve
  client.stop(); // close connection
  HTTPget = ""; // clear out the get param we saved
}

String processRequest(String req){
  int reqLength = req.length();
  int urlStart = req.indexOf('/');
  String str = req.substring(urlStart, reqLength);
  int spacePos = str.indexOf(' ');
  str = str.substring(0, spacePos);
  int analogPos = StringContains(str, "analog");
  int digitalPos = StringContains(str, "digital");
  int getPos = StringContains(str, "get");
  int setPos = StringContains(str, "set");
  String pin = "";
  if(analogPos > 0){
    if(getPos > 0){
      Serial.print("analog get command in for pin ");
      pin = str.substring(getPos+4, str.length());
      Serial.println(pin);
      return String(analogRead(0));
    }else if(setPos > 0){
      Serial.println("analog set command in for pin ");
      pin = str.substring(setPos+4, str.length());
      Serial.println(pin);
    }
  }else if(digitalPos > 0){
    if(getPos > 0){
      Serial.println("digital get command in for pin ");
      pin = str.substring(getPos+4, str.length());
      Serial.println(pin);
      return String(getDigitalPinValue(pin.toInt()));
    }else if(setPos > 0){
      Serial.println("digital set command in for pin ");
      pin = str.substring(setPos+4, str.length());
      Serial.println(pin);
      return String(flipDigitalPin(pin.toInt()));
    }
  }
  return str;
}

int getDigitalPinValue(int pin){
  if(pin == 1){
     return digitalRead(relayPin1);
  }else if(pin == 2){
     return digitalRead(relayPin2);
  }else if(pin == 3){
     return digitalRead(relayPin3);
  }else if(pin == 4){
     return digitalRead(relayPin4);
  }else if(pin == 5){
     return digitalRead(relayPin5);
  }else if(pin == 6){
     return digitalRead(relayPin6);
  }else if(pin == 7){
     return digitalRead(relayPin7);
  }else if(pin == 8){
     return digitalRead(relayPin8);
  } 
}

int flipDigitalPin(int pin){
  Serial.print("switching relay ");
  Serial.println(pin);
  if(pin == 1){
     switchPinState1 = !switchPinState1; 
     reportData();
     return switchPinState1;
  }else if(pin == 2){
     switchPinState2 = !switchPinState2; 
     reportData();
     return switchPinState2;
  }else if(pin == 3){
     switchPinState3 = !switchPinState3; 
     reportData();
     return switchPinState3;
  }else if(pin == 4){
     switchPinState4 = !switchPinState4; 
     reportData();
     return switchPinState4;
  }else if(pin == 5){
     switchPinState5 = !switchPinState5; 
     reportData();
     return switchPinState5;
  }else if(pin == 6){
     switchPinState6 = !switchPinState6; 
     reportData();
     return switchPinState6;
  }else if(pin == 7){
     switchPinState7 = !switchPinState7; 
     reportData();
     return switchPinState7;
  }else if(pin == 8){
     switchPinState8 = !switchPinState8; 
     reportData();
     return switchPinState8;
  }
}

void reportData(){
    reportClient = EthernetClient();
    String data = buildReportData();
    Serial.println(data);
    bool lineIsBlank = false;
    if (reportClient.connect(mainServer, 9000)) {
        Serial.println("connected");
        String request = "GET /arduino/" + data + " HTTP/1.1";
        // Make a HTTP request:
        reportClient.println(request);
        reportClient.println("Host: 192.168.1.5");
        reportClient.println("Connection: close");
        reportClient.println();
      } else {
        // if you didn't get a connection to the server:
        Serial.println("connection failed");
      }
//      while (reportClient.connected())
//      {
//        if (reportClient.available())
//        {
//          char c = reportClient.read();
//          data += c;
//          if (c == '\n' && lineIsBlank)  break;
//          if (c == '\n')
//          {
//            lineIsBlank = true;
//          }
//          else if (c != '\r')
//          {
//           lineIsBlank = false;
//          }
//        }
//      }
  //    Serial.print(data);
      delay(10); // give the web browser a moment to recieve
      reportClient.stop(); // close connection
}
void handleTimer(){
  // set millisTick at the top of each loop if and only if millis() has changed
  unsigned long deltaMillis = 0; // clear last result
  unsigned long thisMillis = millis();
  if (thisMillis != lastMillis) {
    // we have ticked over
    // calculate how many millis have gone past
    deltaMillis = thisMillis-lastMillis; // note this works even if millis() has rolled over back to 0
    lastMillis = thisMillis;
  }
  timer_1 -= deltaMillis;
  if (timer_1 <= 0) {
    // reset timer since this is a repeating timer
    timer_1 += TIMER_INTERVAL_1; // note this prevents the delay accumulating if we miss a mS or two 
    // if we want exactly 1000 delay to next time even if this one was late then just use timeOut = 1000;
    // do time out stuff here
    Serial.println("Repeating Timer 1 timed out");
    reportData();
  }
}

void handleSwitch1() {
  if((long)(micros() - last_micros) >= debouncing_time * 1000) {
    //switchPinState1 = !switchPinState1; 
    //flipDigitalPin(1);
    gMyEventManager.queueEvent( EventManager::kEventUser0, 1 );
    last_micros = micros();
  }
}

void handleSwitch2() {
  if((long)(micros() - last_micros) >= debouncing_time * 1000) {
    //switchPinState2 = !switchPinState2; 
    //flipDigitalPin(2);
    gMyEventManager.queueEvent( EventManager::kEventUser0, 2 );
    last_micros = micros();
  }
}

void switchEventsListener( int eventCode, int eventParam )
{
    Serial.println("Processing interrupt event param: " + eventParam);
    flipDigitalPin(eventParam);
    Serial.println("Event processed.");
}

String buildReportData(){
  float temperature = readTemperature();
  String identifier = "0001-";
  return identifier + switchPinState1 + "-" + switchPinState2 + "-" + switchPinState3 +
  "-" + switchPinState4 + "-" + switchPinState5 + "-" + switchPinState6 + "-" + switchPinState7 + "-" + switchPinState8 +
  "-" + temperature;
}

int StringContains(String s, String search) {
    int max = s.length() - search.length();
    int lgsearch = search.length();
    for (int i = 0; i <= max; i++) {
        if (s.substring(i, i + lgsearch) == search) return i;
    }
 return -1;
}

