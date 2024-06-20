// ESP32 combat robot (robowar) controller
// Drive: RC receiver -> differential drive via PWM+DIR motor driver
// Weapon: horizontal spinner/bar driven by a brushless motor + ESC
// Same control logic across 8kg / 15kg / 30kg classes -- only the
// motors, ESC current rating and battery change with weight class.

#include <ESP32Servo.h>

// ---------- RC input pins (input-only capable ESP32 pins) ----------
const int PIN_CH_STEER  = 34; // steering channel
const int PIN_CH_THROT  = 35; // throttle channel
const int PIN_CH_WEAPON = 32; // weapon arm channel

// ---------- Drive motor driver pins (PWM + DIR per side) ----------
const int PIN_LEFT_PWM  = 25;
const int PIN_LEFT_DIR  = 26;
const int PIN_RIGHT_PWM = 27;
const int PIN_RIGHT_DIR = 14;

// ---------- Weapon ESC signal pin ----------
const int PIN_WEAPON_ESC = 33;

// ---------- Status LED ----------
const int PIN_STATUS_LED = 2;

// ---------- RC pulse width range (microseconds) ----------
const int RC_MID = 1500;
const int RC_DEADZONE = 30;

// Weapon arms when channel is above this pulse width
const int WEAPON_ARM_THRESHOLD = 1700;
const int WEAPON_RUN_US = 2000; // full speed to ESC when armed
const int WEAPON_OFF_US = 1000; // ESC stop signal

// Failsafe: cut everything if no valid RC pulse for this long
const unsigned long FAILSAFE_TIMEOUT_MS = 300;

// LEDC PWM config for drive motors
const int PWM_FREQ = 20000;
const int PWM_RES_BITS = 8; // 0-255
const int PWM_CH_LEFT = 0;
const int PWM_CH_RIGHT = 1;

Servo weaponESC;

volatile uint32_t steerRiseUs = 0, steerPulseUs = RC_MID;
volatile uint32_t throtRiseUs = 0, throtPulseUs = RC_MID;
volatile uint32_t weaponRiseUs = 0, weaponPulseUs = 1000;

volatile unsigned long lastSteerMs = 0;
volatile unsigned long lastThrotMs = 0;
volatile unsigned long lastWeaponMs = 0;

void IRAM_ATTR isrSteer() {
  if (digitalRead(PIN_CH_STEER) == HIGH) {
    steerRiseUs = micros();
  } else {
    steerPulseUs = micros() - steerRiseUs;
    lastSteerMs = millis();
  }
}

void IRAM_ATTR isrThrot() {
  if (digitalRead(PIN_CH_THROT) == HIGH) {
    throtRiseUs = micros();
  } else {
    throtPulseUs = micros() - throtRiseUs;
    lastThrotMs = millis();
  }
}

void IRAM_ATTR isrWeapon() {
  if (digitalRead(PIN_CH_WEAPON) == HIGH) {
    weaponRiseUs = micros();
  } else {
    weaponPulseUs = micros() - weaponRiseUs;
    lastWeaponMs = millis();
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_CH_STEER, INPUT);
  pinMode(PIN_CH_THROT, INPUT);
  pinMode(PIN_CH_WEAPON, INPUT);
  pinMode(PIN_STATUS_LED, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(PIN_CH_STEER), isrSteer, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_CH_THROT), isrThrot, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_CH_WEAPON), isrWeapon, CHANGE);

  pinMode(PIN_LEFT_DIR, OUTPUT);
  pinMode(PIN_RIGHT_DIR, OUTPUT);
  ledcSetup(PWM_CH_LEFT, PWM_FREQ, PWM_RES_BITS);
  ledcSetup(PWM_CH_RIGHT, PWM_FREQ, PWM_RES_BITS);
  ledcAttachPin(PIN_LEFT_PWM, PWM_CH_LEFT);
  ledcAttachPin(PIN_RIGHT_PWM, PWM_CH_RIGHT);

  weaponESC.setPeriodHertz(50);
  weaponESC.attach(PIN_WEAPON_ESC, 1000, 2000);
  weaponESC.writeMicroseconds(WEAPON_OFF_US);

  unsigned long now = millis();
  lastSteerMs = now;
  lastThrotMs = now;
  lastWeaponMs = now;
}

void setDriveMotor(int pwmChannel, int dirPin, int speedSigned) {
  speedSigned = constrain(speedSigned, -255, 255);
  digitalWrite(dirPin, speedSigned >= 0 ? HIGH : LOW);
  ledcWrite(pwmChannel, abs(speedSigned));
}

bool rcSignalValid(unsigned long lastMs) {
  return (millis() - lastMs) < FAILSAFE_TIMEOUT_MS;
}

void loop() {
  unsigned long now = millis();

  bool steerOk  = rcSignalValid(lastSteerMs);
  bool throtOk  = rcSignalValid(lastThrotMs);
  bool weaponOk = rcSignalValid(lastWeaponMs);
  bool linkOk = steerOk && throtOk;

  if (!linkOk) {
    setDriveMotor(PWM_CH_LEFT, PIN_LEFT_DIR, 0);
    setDriveMotor(PWM_CH_RIGHT, PIN_RIGHT_DIR, 0);
    weaponESC.writeMicroseconds(WEAPON_OFF_US);
    digitalWrite(PIN_STATUS_LED, (now / 200) % 2); // fast blink = no signal
    delay(10);
    return;
  }

  digitalWrite(PIN_STATUS_LED, HIGH); // solid = signal ok

  int steer = (int)steerPulseUs - RC_MID;
  int throt = (int)throtPulseUs - RC_MID;
  if (abs(steer) < RC_DEADZONE) steer = 0;
  if (abs(throt) < RC_DEADZONE) throt = 0;

  // RC half-range is ~500us -> scale to +-255
  int steerScaled = constrain((steer * 255) / 500, -255, 255);
  int throtScaled = constrain((throt * 255) / 500, -255, 255);

  int leftSpeed  = constrain(throtScaled + steerScaled, -255, 255);
  int rightSpeed = constrain(throtScaled - steerScaled, -255, 255);

  setDriveMotor(PWM_CH_LEFT, PIN_LEFT_DIR, leftSpeed);
  setDriveMotor(PWM_CH_RIGHT, PIN_RIGHT_DIR, rightSpeed);

  if (weaponOk && weaponPulseUs > WEAPON_ARM_THRESHOLD) {
    weaponESC.writeMicroseconds(WEAPON_RUN_US);
  } else {
    weaponESC.writeMicroseconds(WEAPON_OFF_US);
  }

  delay(5);
}
