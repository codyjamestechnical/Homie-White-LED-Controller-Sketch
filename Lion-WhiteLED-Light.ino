// Scektch developed by Cody James, Cody James Technical

#include <Homie.h>
#include <ArduinoJson.h>
#include "config.h"

HomieNode lightNode("light", "light");

// Set buffer for JSON processing
const int BUFFER_SIZE = JSON_OBJECT_SIZE(20);

//LED Transition Speed
unsigned long transitionSpeed = CONFIG_TRANSITION_SPEED;

// Milliseconds to wait between state announcements
// ie how long to wait to auto call the current state to the MQTT server
const long callStateDelay = CONFIG_CALL_STATE_DELAY;

unsigned long prevStateCallMillis = 0;
unsigned long prevTransitionMillis = 0;

// Define LED Pins
int ledPin = CONFIG_LED_PIN;

// Current Brightness
// This is initialized at 256 because the board wants to pull the pin HIGH on boot
// This will trigger the code to adjust the brightness if it is set below max
int currentBrightness = 256;

// Target Brightness
int targetBrightness = map(CONFIG_DEFAULT_BRIGHTNESS, 0, 255, CONFIG_MIN_BRIGHTNESS, CONFIG_MAX_BRIGHTNESS);

// Restore Brightness
int restoreBrightness;

// Power State
boolean state = true;

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

char* string2char(String command){
    if(command.length()!=0){
        char *p = const_cast<char*>(command.c_str());
        return p;
    }
}

String returnJSONstate() {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

  root["state"] = (state) ? CONFIG_JSON_PAYLOAD_ON : CONFIG_JSON_PAYLOAD_OFF;

  root["brightness"] = map(targetBrightness, CONFIG_MIN_BRIGHTNESS, CONFIG_MAX_BRIGHTNESS, 1, 255);

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));
  return buffer;
}

void setBrightnessValue(float newBrightness, int transitionTime = 0){
  if (newBrightness < 1 || newBrightness > 255) {
      Serial.println("ERROR: Brightness value is outside of range");
      Serial.println("Value received: " + String(newBrightness));
      return;
  }

  if (state == false) {
    state = true;
    targetBrightness = 1;
  }

  if ( CONFIG_MIN_BRIGHTNESS > 1 || CONFIG_MAX_BRIGHTNESS < 255 ) {
    newBrightness = map(newBrightness, 0, 255, CONFIG_MIN_BRIGHTNESS, CONFIG_MAX_BRIGHTNESS);
  }
  
  if ( transitionTime == 0 ) {
    Serial.println("No transition time set, setting default transition time");
    transitionSpeed = CONFIG_TRANSITION_SPEED;
  } else {
    Serial.println("Transition time received, setting custom transtion time");
    
    int ticks = abs( targetBrightness - newBrightness );
    Serial.println("number of steps needed to reach target brightness: " + String(ticks));
    
    if (ticks > 0) {
      transitionSpeed = (( transitionTime * 1000 ) / ticks );
      Serial.println("Calculated milliseconds between brightness updates: " + String(transitionSpeed));
    }
  }
  
  targetBrightness = newBrightness;
  Serial.println("Target brightness set to: " + String(newBrightness));
  
}

bool processJSON(char* message) {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(message);

  if (!root.success()) {
    Serial.println("ERROR: Failed to process JSON data");
    Serial.println("Make sure the JSON string is formatted properly, and try again");
    return false;
}
  
  if (root.containsKey("brightness")) {
   Serial.println("Brightness key found in JSON data");
   int bri = root["brightness"];
   
   if ( bri != targetBrightness ) {
      if ( root.containsKey("transition") ) {
        Serial.println("Transition key found in JSON data");
        Serial.println("Calling setBrightnessValue with transition time");
        
        setBrightnessValue(bri, root["transition"]);
      } else {
        Serial.println("No transition key found in JSON data");
        Serial.println("Calling setBrightnessValue() without transition time");
        
        setBrightnessValue(bri);
      }
    }
  }

  if ( root.containsKey("state") ) {
    Serial.println("state key found in JSON data");
    
    if (strcmp(root["state"], CONFIG_JSON_PAYLOAD_ON) == 0) {
      Serial.println("Calling setState(true)");
      setState(true);     
    } else if (strcmp(root["state"], CONFIG_JSON_PAYLOAD_OFF) == 0) {
       Serial.println("Calling setState(false)");
      setState(false);
    }
  }

  return true;
}

void setState( bool newState ) { 
  if ( newState == false && state != false ) {
    
    // copy the targetRGB to restoreRGB
    // so we can restore settings when we turn back on
    // there must be a more efficient way to do this

    Serial.println("Setting default transition time, before turning off the light");
    transitionSpeed = CONFIG_TRANSITION_SPEED;

    Serial.println("Saving brightness restore point");
    restoreBrightness = targetBrightness;
    
    Serial.println("Setting target brightness to 0");
    targetBrightness = 0;

    Serial.println("Setting state variable to false");
    state = false;

    Serial.println("The light is off");
  } else if ( newState == true && state != true ) { 

    Serial.println("Setting brightness to last value from restore point");
    targetBrightness = restoreBrightness;

    Serial.println("Setting state variable to true");
    state = true;
    
    Serial.println("The light is on");
  } else {
    Serial.println("New state == old state; state not updated");
  }
}

void announceStats() {
  Serial.println("Announcing stats to MQTT server");
  
  int bri = round(map(targetBrightness, CONFIG_MIN_BRIGHTNESS, CONFIG_MAX_BRIGHTNESS, 1, 255));
  int briPercent = round(map(targetBrightness, CONFIG_MIN_BRIGHTNESS, CONFIG_MAX_BRIGHTNESS, 1, 100));
  
  lightNode.setProperty("on").send((state ? "true" : "false"));
  
  lightNode.setProperty("brightness").send(String(bri));
  
  lightNode.setProperty("brightnesspct").send(String(briPercent));
  
  lightNode.setProperty("json").send(String(returnJSONstate()));
}

bool jsonHandler(HomieRange range, String value) {
  if (!processJSON(string2char(value))) {
    Serial.println("Error processing JSON data!");
    announceStats();
    return false;
  }
  Serial.println("Successfully processed JSON data!");
  announceStats();
  return true;
}

bool stateHandler(HomieRange range, String value) {
  if (value != "true" && value != "false") {
    Serial.println("Invalid state data received; unable to parse data");
    Serial.println("Received value: " + value);
    return false;
  }

  bool on = (value =="true");

  Serial.println("Calling setState()");
  setState(on);
  
  announceStats();
  
  return true;
}

bool brightnessHandler(HomieRange range, String value) {
  
  int bri = value.toInt();

  setBrightnessValue(bri);

  announceStats();

  Serial.println("Changed brightness to " + String(bri));
  
  return true;
}

bool brightnessPercentHandler(HomieRange range, String value) {
  int pct = value.toInt();

  if (pct < 1) {
    pct = 1;
  } else if ( pct > 100 ) {
    pct = 100;
  }

  int bri = map(pct, 0, 100, 0, 255);

  setBrightnessValue(bri);

  announceStats();

  Serial.println("Changed brightness to " + String(bri));
  return true;
}

void homieSetup() {
  Serial.println("WiFi Connected; Startup Complete!");
  
  if ( CONFIG_DEFAULT_STATE == false ) {
    setState(false);
  }
  
  announceStats();
}

void homieLoop() {
  unsigned int currentMillis = millis();

  if ( currentMillis - prevStateCallMillis >= callStateDelay ) {
    announceStats();
    prevStateCallMillis = currentMillis;
  }
}

void setup() { 
  if ( CONFIG_DEBUG == true ) {
    Serial.begin(115200);
    Serial.println("Starting...");
  }

  Homie_setBrand("lhl");
  Homie_setFirmware("lion-home-light", "1.0.0");

  lightNode.advertise("on").settable(stateHandler);
  lightNode.advertise("brightness").settable(brightnessHandler);
  lightNode.advertise("brightnesspct").settable(brightnessPercentHandler);
  lightNode.advertise("json").settable(jsonHandler);
  
  analogWriteFreq(400);
  analogWriteRange(255);

  Homie.setSetupFunction(homieSetup);
  Homie.setLoopFunction(homieLoop);
  Homie.setup();
  
}
 
void loop()  { 
  unsigned long currentMillis = millis();
  
  if (currentMillis - prevTransitionMillis >= transitionSpeed) {
    
      if ( currentBrightness!= targetBrightness ) {
          if ( currentBrightness > targetBrightness ) {
            currentBrightness--;
          } else {
            currentBrightness++;
          }
          
          if ( CONFIG_INVERT_LOGIC == true ) {
            int inverted = (255 - currentBrightness);
            analogWrite(ledPin, inverted);
          }else {
            analogWrite(ledPin, currentBrightness);
          }      
      
    }
  
    prevTransitionMillis = currentMillis;
 }
 Homie.loop();
}
