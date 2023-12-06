#include <IRremote.h>
#include <DHT.h>
#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>

//Control Device
const char THING_ID[]           = "8098e7da-a4fc-4498-98d1-6205109ac847";
const char DEVICE_LOGIN_NAME[]  = "e20ef8f3-de0d-4014-a652-14e185db083a";
const char DEVICE_KEY[]         = "I002MTZTPA1LISCVALFS";

const char SSID[]               = "TOTOLINK_N350RT";
const char PASS[]               = "09040704";
// const char SSID[]               = "nguyenhai";
// const char PASS[]               = "23456788";
// const char SSID[]               = "VNPT_YL2";
// const char PASS[]               = "86777414";
#define IR_ON_ALL     0xB847FF00
#define IR_OFF_ALL    0xBA45FF00
#define IR_Fan_Up     0xB54AFF00
#define IR_Fan_Down   0xBD42FF00
#define IR_Fan_On_Off 0xAD52FF00
#define IR_SW1        0xF20DFF00
#define IR_SW2        0xE718FF00
#define IR_SW3        0xA15EFF00
#define IR_SW4        0xF708FF00

#define DHT_pin       16  //RX2  pin connected with DHT
#define GAS_pin       34
#define IR_pin        35
#define wifiLed       2

#define Relay4_pin    19  //gpio for relay 
#define Relay2_pin    22
#define Relay3_pin    21
#define Relay1_pin    23
#define FanRelay1     32
#define FanRelay2     33
#define FanRelay3     25

#define Switch1_pin 13  //gpio for touch sensor
#define Switch2_pin 12
#define Switch3_pin 14
#define Switch4_pin 27

/*----------------------SETUP_IOT_CLOUND----------------------*/
void onSwitch1Change();
void onSwitch2Change();
void onSwitch3Change();
void onSwitch4Change();
void onFanChange();
void fanSpeedControl(int fan_speed);
void convert_fanBrightness_to_fanSpeed(CloudDimmedLight x, int y);

float gas_data = 0;
CloudDimmedLight fan;
CloudSwitch switch1;
CloudSwitch switch2;
CloudSwitch switch3;
CloudSwitch switch4;
CloudTemperatureSensor temperature;
CloudPercentage airQuality;
CloudPercentage humidity;

void initProperties() {
  ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
  ArduinoCloud.setThingId(THING_ID);
  ArduinoCloud.addProperty(fan, READWRITE, ON_CHANGE, onFanChange);
  ArduinoCloud.addProperty(switch1, READWRITE, ON_CHANGE, onSwitch1Change);
  ArduinoCloud.addProperty(switch2, READWRITE, ON_CHANGE, onSwitch2Change);
  ArduinoCloud.addProperty(switch3, READWRITE, ON_CHANGE, onSwitch3Change);
  ArduinoCloud.addProperty(switch4, READWRITE, ON_CHANGE, onSwitch4Change);
  //sensor
  ArduinoCloud.addProperty(temperature, READ, 1 * SECONDS, NULL); 
  ArduinoCloud.addProperty(airQuality, READ, 1 * SECONDS, NULL);
  ArduinoCloud.addProperty(humidity, READ, 1 * SECONDS, NULL);
}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);

//kHOI TAO DOI TUONG dht
DHT dht(DHT_pin, DHT11);
//Khoi tao doi tuong irrecv
IRrecv irrecv(IR_pin);
decode_results results;

//to remember the toggle state
int toggleState_1 = 0;
int toggleState_2 = 0;
int toggleState_3 = 0;
int toggleState_4 = 0;

int switchState_1 = 0;
int switchState_2 = 0;
int switchState_3 = 0;
int switchState_4 = 0;

/*-----------------------Check for connect wifi-------------------*/
void Connected() {
  Serial.println("Board successfully connected to Arduino IoT Cloud");
  digitalWrite(wifiLed, HIGH);
}
void doThisOnSync() {
  Serial.println("Thing Properties synchronised");
}
void Disconnected() {
  Serial.println("Board disconnected from Arduino IoT Cloud");
  digitalWrite(wifiLed, LOW);
}

void setup() {
  Serial.begin(115200);

  pinMode(DHT_pin, INPUT);
  
  dht.begin();

  pinMode(FanRelay1, OUTPUT);
  pinMode(FanRelay2, OUTPUT);
  pinMode(FanRelay3, OUTPUT);

  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::CONNECT, Connected);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::SYNC, doThisOnSync);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::DISCONNECT, Disconnected);

  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  irrecv.enableIRIn();

  //GPIO Init
  pinMode(Relay1_pin, OUTPUT);
  pinMode(Relay2_pin, OUTPUT);
  pinMode(Relay3_pin, OUTPUT);
  pinMode(Relay4_pin, OUTPUT);
  //Khoi tao gia tri ban dau cho relay la OFF
  digitalWrite(Relay1_pin, LOW);
  digitalWrite(Relay2_pin, LOW);
  digitalWrite(Relay3_pin, LOW);
  digitalWrite(Relay4_pin, LOW);

  pinMode(Switch1_pin, INPUT_PULLUP);
  pinMode(Switch2_pin, INPUT_PULLUP);
  pinMode(Switch3_pin, INPUT_PULLUP);
  pinMode(Switch4_pin, INPUT_PULLUP);

  pinMode(wifiLed, OUTPUT);
  pinMode(GAS_pin, INPUT);
}
/*----------------------------------------LOOP----------------------------------------*/
void loop() {
  ArduinoCloud.update();
  IR_remote_control();
  TouchSensor_control();
  sendSensor();
}
/*----------------------Read value dht11----------------------*/
void readSensor_dht11(){
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
}
void readSensor_gas(){
  int air_value = analogRead(GAS_pin); 
  Serial.println(air_value);
  airQuality = air_value;
}
void sendSensor()
{
  readSensor_dht11();
  readSensor_gas();
}
void IR_remote_control() {
  if (irrecv.decode()) {
    switch (irrecv.decodedIRData.decodedRawData) {
      case IR_OFF_ALL:
        Serial.println("Turn off all relay by Remote");
        relayTurnOffAll();
        fan.setSwitch(false);
        onFanChange();
        break;

      case IR_ON_ALL:
        Serial.println("turn on all relay by Remote");
        fan.setSwitch(true);
        onFanChange();
        relayTurnOnAll();
        break;

      case IR_SW1:
        relayOnOff(1);
        switch1 = toggleState_1;
        if(toggleState_1 == 1) Serial.println("relay 1 on by Remote");
        else Serial.println("relay 1 off by Remote");
      break;

      case IR_SW2:
        relayOnOff(2);
        switch2 = toggleState_2;
        if(toggleState_2 == 1) Serial.println("relay 2 on by Remote");
        else Serial.println("relay 2 off by Remote");
      break;

      case IR_SW3:
        relayOnOff(3);
        switch3 = toggleState_3;
        if(toggleState_3 == 1) Serial.println("relay 3 on by Remote");
        else Serial.println("relay 3 off by Remote");
      break;

      case IR_SW4:
        relayOnOff(4);
        switch4 = toggleState_4;
        if(toggleState_4 == 1) Serial.println("relay 4 on by Remote");
        else Serial.println("relay 4 off by Remote");
      break;

      case IR_Fan_Up:
        if (fan.getSwitch() && fan.getBrightness() < 100) {
          if(fan.getBrightness() + 25 > 100)
            fan.setBrightness(100);
          else
            fan.setBrightness(fan.getBrightness() + 25);
            Serial.print("Control Fan Up by Remote");
          onFanChange();
        }
        break;

      case IR_Fan_Down:
        if (fan.getSwitch() && fan.getBrightness() > 0) {
          if(fan.getBrightness() - 25 < 0)
            fan.setBrightness(0);
          else 
            fan.setBrightness(fan.getBrightness() - 25);

          Serial.print("Control Fan Down by Remote");
          onFanChange();
        }
        break;

      case IR_Fan_On_Off:
        fan.setSwitch(!fan.getSwitch());
        onFanChange();
        break;
      default:
        break;
    }

    irrecv.resume();
  }
}
void relayOnOff(int num){
  switch(num){
    case 1:
      if(toggleState_1 == 0){
        digitalWrite(Relay1_pin, HIGH);
        switchState_1 = HIGH;
        toggleState_1 = 1;
      }
      else{
        digitalWrite(Relay1_pin, LOW);
        switchState_1 = LOW;
        toggleState_1 = 0;
      }
    break;

    case 2:
      if(toggleState_2 == 0){
        digitalWrite(Relay2_pin, HIGH);
        switchState_2 = HIGH;
        toggleState_2 = 1;
      }
      else{
        digitalWrite(Relay2_pin, LOW);
        switchState_2 = LOW;
        toggleState_2 = 0;
      }
    break;

    case 3:
      if(toggleState_3 == 0){
        digitalWrite(Relay3_pin, HIGH);
        switchState_3 = HIGH;
        toggleState_3 = 1;
      }
      else{
        digitalWrite(Relay3_pin, LOW);
        switchState_3 = LOW;
        toggleState_3 = 0;
      }
    break;

    case 4:
      if(toggleState_4 == 0){
        digitalWrite(Relay4_pin, HIGH);
        switchState_4 = HIGH;
        toggleState_4 = 1;
      }
      else{
        digitalWrite(Relay4_pin, LOW);
        switchState_4 = LOW;
        toggleState_4 = 0;
      }
    break;
    default : break;
  }
}
void relayTurnOnAll(){
  digitalWrite(Relay1_pin, HIGH);
  digitalWrite(Relay2_pin, HIGH);
  digitalWrite(Relay3_pin, HIGH);
  digitalWrite(Relay4_pin, HIGH);
  switchState_1 = HIGH;
  switchState_2 = HIGH;
  switchState_3 = HIGH;
  switchState_4 = HIGH;
  toggleState_1 = 1;
  toggleState_2 = 1;
  toggleState_3 = 1;
  toggleState_4 = 1;
  switch1 = toggleState_1;
  switch2 = toggleState_2;
  switch3 = toggleState_3;
  switch4 = toggleState_4;
}
void relayTurnOffAll(){
  digitalWrite(Relay1_pin, LOW);
  digitalWrite(Relay2_pin, LOW);
  digitalWrite(Relay3_pin, LOW);
  digitalWrite(Relay4_pin, LOW);
  switchState_1 = LOW;
  switchState_2 = LOW;
  switchState_3 = LOW;
  switchState_4 = LOW;
  toggleState_1 = 0;
  toggleState_2 = 0;
  toggleState_3 = 0;
  toggleState_4 = 0;
  switch1 = toggleState_1;
  switch2 = toggleState_2;
  switch3 = toggleState_3;
  switch4 = toggleState_4;
}
/*----------------------Touch Control----------------------*/
void TouchSensor_control(){
  if(digitalRead(Switch1_pin) == HIGH && switchState_1 == LOW){
      digitalWrite(Relay1_pin, HIGH);
      toggleState_1 = 1;
      switchState_1 = HIGH;
      switch1 = toggleState_1;
      Serial.println("Relay 1 On by Touch");
      while(digitalRead(Switch1_pin)); delay(20);
  }
  if(digitalRead(Switch1_pin) == HIGH && switchState_1 == HIGH){
      digitalWrite(Relay1_pin, LOW);
      toggleState_1 = 0;
      switchState_1 = LOW;
      switch1 = toggleState_1;
      Serial.println("Relay 1 Off by Touch");
      while(digitalRead(Switch1_pin)); delay(20);
  }
  if(digitalRead(Switch2_pin) == HIGH && switchState_2 == LOW){
      digitalWrite(Relay2_pin, HIGH);
      toggleState_2 = 1;
      switchState_2 = HIGH;
      switch2 = toggleState_2;
      Serial.println("Relay 2 On by Touch");
      while(digitalRead(Switch2_pin)); delay(20);
  }
  if(digitalRead(Switch2_pin) == HIGH && switchState_2 == HIGH){
      digitalWrite(Relay2_pin, LOW);
      toggleState_2 = 0;
      switchState_2 = LOW;
      switch2 = toggleState_2;
      Serial.println("Relay 2 Off by Touch");
      while(digitalRead(Switch2_pin)); delay(20);
  }
  if(digitalRead(Switch3_pin) == HIGH && switchState_3 == LOW){
      digitalWrite(Relay3_pin, HIGH);
      toggleState_3 = 1;
      switchState_3 = HIGH;
      switch3 = toggleState_3;
      Serial.println("Relay 3 On by Touch");
      while(digitalRead(Switch3_pin)); delay(20);
  }
  if(digitalRead(Switch3_pin) == HIGH && switchState_3 == HIGH){
      digitalWrite(Relay3_pin, LOW);
      toggleState_3 = 0;
      switchState_3 = LOW;
      switch3 = toggleState_3;
      Serial.println("Relay 3 Off by Touch");
      while(digitalRead(Switch3_pin)); delay(20);
  }
  if(digitalRead(Switch4_pin) == HIGH && switchState_4 == LOW){
      digitalWrite(Relay4_pin, HIGH);
      toggleState_4 = 1;
      switchState_4 = HIGH;
      switch4 = toggleState_4;
      Serial.println("Relay 4 On by Touch");
      while(digitalRead(Switch4_pin)); delay(20);
  }
  if(digitalRead(Switch4_pin) == HIGH && switchState_4 == HIGH){
      digitalWrite(Relay4_pin, LOW);
      toggleState_4 = 0;
      switchState_4 = LOW;
      switch4 = toggleState_4;
      Serial.println("Relay 4 Off by Touch");
      while(digitalRead(Switch4_pin)); delay(20);
  }
}
void onSwitch1Change() {
  //Control the device
  if (switch1 == 1)
  {
    digitalWrite(Relay1_pin, HIGH);
    Serial.println("Device1 ON");
    toggleState_1 = 1;
  }
  else
  {
    digitalWrite(Relay1_pin, LOW);
    Serial.println("Device1 OFF");
    toggleState_1 = 0;
  }
}

void onSwitch2Change() {
  if (switch2 == 1)
  {
    digitalWrite(Relay2_pin, HIGH);
    Serial.println("Device2 ON");
    toggleState_2 = 1;
  }
  else
  {
    digitalWrite(Relay2_pin, LOW);
    Serial.println("Device2 OFF");
    toggleState_2 = 0;
  }
}

void onSwitch3Change() {
  if (switch3 == 1)
  {
    digitalWrite(Relay3_pin, HIGH);
    Serial.println("Device3 ON");
    toggleState_3 = 1;
  }
  else
  {
    digitalWrite(Relay3_pin, LOW);
    Serial.println("Device3 OFF");
    toggleState_3 = 0;
  }
}

void onSwitch4Change() {
  if (switch4 == 1)
  {
    digitalWrite(Relay4_pin, HIGH);
    Serial.println("Device4 ON");
    toggleState_4 = 1;
  }
  else
  {
    digitalWrite(Relay4_pin, LOW);
    Serial.println("Device4 OFF");
    toggleState_4 = 0;
  }
}
void onGasDataChange()  {
  // Add your code here to act upon GasData change
}
void onFanChange() {
  if (fan.getSwitch()) {
    int current_fan_speed = 0;
    if (fan.getBrightness() >= 1 && fan.getBrightness() <= 25) current_fan_speed = 1;
    else if (fan.getBrightness() >= 26 && fan.getBrightness() <= 50) current_fan_speed = 2;
    else if (fan.getBrightness() >= 51 && fan.getBrightness() <= 75) current_fan_speed = 3;
    else if (fan.getBrightness() >= 76 && fan.getBrightness() <= 100) current_fan_speed = 4;
    Serial.print("Fan Speed By App: ");
    Serial.println(current_fan_speed);
    fanSpeedControl(current_fan_speed);
  } else {
    fanSpeedControl(0);
  }
}

void fanSpeedControl(int fan_speed) {
  switch (fan_speed) {
    case 0:
      digitalWrite(FanRelay1, LOW);
      digitalWrite(FanRelay2, LOW);
      digitalWrite(FanRelay3, LOW);
      break;
    case 1:
      digitalWrite(FanRelay1, LOW);
      digitalWrite(FanRelay2, LOW);
      digitalWrite(FanRelay3, LOW);
      delay(500);
      digitalWrite(32, HIGH);
      break;
    case 2:
      digitalWrite(FanRelay1, LOW);
      digitalWrite(FanRelay2, LOW);
      digitalWrite(FanRelay3, LOW);
      delay(500);
      digitalWrite(FanRelay2, HIGH);
      break;
    case 3:
      digitalWrite(FanRelay1, LOW);
      digitalWrite(FanRelay2, LOW);
      digitalWrite(FanRelay3, LOW);
      delay(500);
      digitalWrite(FanRelay2, HIGH);
      digitalWrite(FanRelay1, HIGH);
      break;
    case 4:
      digitalWrite(FanRelay1, LOW);
      digitalWrite(FanRelay2, LOW);
      digitalWrite(FanRelay3, LOW);
      delay(500);
      digitalWrite(FanRelay3, HIGH);
      break;
    default:
      break;
  }
}

void convert_fanBrightness_to_fanSpeed(CloudDimmedLight x, int y) {
  if (x.getBrightness() == 0) y = 0;
  else if (x.getBrightness() >= 1 && x.getBrightness() <= 25) y = 1;
  else if (x.getBrightness() >= 26 && x.getBrightness() <= 50) y = 2;
  else if (x.getBrightness() >= 51 && x.getBrightness() <= 75) y = 3;
  else if (x.getBrightness() >= 76 && x.getBrightness() <= 100) y = 4;
}
