// how long between color steps in milliseconds
// default value, a different value will be 
// automatically calculated if a transition time
// is supplied over MQTT JSON
#define CONFIG_TRANSITION_SPEED 3

// time between calling current state to server
#define CONFIG_CALL_STATE_DELAY 10000

// LED pin assignments
#define CONFIG_LED_PIN 12

// set on/off values for MQTT JSON commands
#define CONFIG_JSON_PAYLOAD_ON "ON"
#define CONFIG_JSON_PAYLOAD_OFF "OFF"

// Reverse the LED logic
// false: 0 (off) - 255 (bright)
// true: 255 (off) - 0 (bright)
#define CONFIG_INVERT_LOGIC false

// Enables Serial and Print Statements
#define CONFIG_DEBUG false

// this is required, and is the minimum/maximum brightness allowed
// 0 - 255 are acceptable values
// this is useful if you have LEDs that don't look like they're on below a certain value
// or are too bright above certain values
#define CONFIG_MIN_BRIGHTNESS 1
#define CONFIG_MAX_BRIGHTNESS 128

#define CONFIG_DEFAULT_BRIGHTNESS 255
#define CONFIG_DEFAULT_STATE false

// NOTES
// beer sign:
// PIN: 12; Max Brightness: 128

// Under Cabinet: 
// PIN: 14
