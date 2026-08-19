// ---------------------------- Pin Definitions ------------------------------
const uint8_t PIN_LED_RED   = 21;
const uint8_t PIN_LED_BLUE  = 19;
const uint8_t PIN_BUTTON    = 23;   // INPUT_PULLUP, LOW = pressed
const uint8_t PIN_LDR       = 34;   // ADC1_CH6, input-only pin

// ---------------------------- Tunable Constants -----------------------------
const int      DARK_THRESHOLD    = 1250;  // Calibrated: dark ~800, light ~1700
const uint32_t ALARM_DURATION_MS = 10000; // 10 seconds LEDs stay on after motion
const uint32_t DEBOUNCE_MS       = 50;    // Button debounce window
const uint32_t TELEMETRY_PERIOD_MS = 1000; // How often to print status
const uint32_t BLINK_INTERVAL_MS  = 300;  // How fast Red/Blue alternate in BLINK mode

// ---------------------------- Light Modes ------------------------------------
enum LightMode {
  MODE_BLINK = 0,   // Red and Blue alternate
  MODE_SOLID = 1    // Red and Blue both on steadily
};

LightMode currentMode = MODE_BLINK; // Starting mode — button toggles from here

// ---------------------------- System State ----------------------------------
bool alarmActive        = false; // True while the LEDs should be displaying
uint32_t alarmStartTime = 0;     // millis() timestamp of last motion trigger

// Non-blocking blink state (only used while alarmActive && MODE_BLINK)
uint32_t lastBlinkToggle = 0;
bool     blinkShowRed    = true; // true = Red on/Blue off, false = Blue on/Red off

// Button debouncing state
bool     lastRawButtonState   = HIGH; // raw pin reading from previous loop
bool     stableButtonState    = HIGH; // debounced/confirmed state
uint32_t lastButtonChangeTime = 0;

// Telemetry timing
uint32_t lastTelemetryTime = 0;

// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.setTimeout(10);

  pinMode(PIN_LED_RED,  OUTPUT);
  pinMode(PIN_LED_BLUE, OUTPUT);
  pinMode(PIN_BUTTON,   INPUT_PULLUP);

  setLEDs(false, false); // Ensure both LEDs start OFF

  Serial.println("=== ESP32 Motion Security System v2 — Booted ===");
  Serial.println(">> Starting mode: BLINK");
}

// -----------------------------------------------------------------------------
void loop() {
  uint32_t now = millis();

  handleButton(now);      // Non-blocking debounce + mode toggle
  handleSerialInput(now);  // Read '1'/'0' from laptop, arm alarm if needed
  updateAlarmTimer(now);   // Turn alarm off once 30s has elapsed
  updateLEDs(now);         // Drive the LEDs according to alarmActive + currentMode
  printTelemetry(now);     // Periodic status print
}

// -----------------------------------------------------------------------------
// Reads the button with software debouncing and toggles currentMode on a
// confirmed HIGH -> LOW transition (press). Fully non-blocking.
void handleButton(uint32_t now) {
  bool rawState = digitalRead(PIN_BUTTON);

  // If the raw reading changed, restart the debounce timer.
  if (rawState != lastRawButtonState) {
    lastButtonChangeTime = now;
    lastRawButtonState = rawState;
  }

  // If the reading has been stable for DEBOUNCE_MS, accept it as the true state.
  if ((now - lastButtonChangeTime) > DEBOUNCE_MS) {
    if (rawState != stableButtonState) {
      stableButtonState = rawState;

      // A press is a transition to LOW (button connects pin to GND).
      if (stableButtonState == LOW) {
        // Cycle between the two modes.
        currentMode = (currentMode == MODE_BLINK) ? MODE_SOLID : MODE_BLINK;

        Serial.print(">> BUTTON PRESSED — Mode now: ");
        Serial.println(currentMode == MODE_BLINK ? "BLINK" : "SOLID");

        // Reset blink phase so the new mode starts cleanly.
        lastBlinkToggle = now;
        blinkShowRed = true;
      }
    }
  }
}

// -----------------------------------------------------------------------------
// Reads a single character from Serial (sent by the laptop app each frame).
// '1' = motion detected this frame, '0' = no motion. Any other byte ignored.
void handleSerialInput(uint32_t now) {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();

    if (c == '1') {
      bool roomIsDark = (analogRead(PIN_LDR) < DARK_THRESHOLD);

      if (roomIsDark) {
        if (!alarmActive) {
          // Fresh trigger — reset blink phase so it starts clean.
          lastBlinkToggle = now;
          blinkShowRed = true;
        }
        alarmActive    = true;
        alarmStartTime = now;   // (re)start the 30s countdown
      }
      // If the room is bright, a '1' is simply ignored — no alarm.
    }
    // '0' bytes need no action: the timer in updateAlarmTimer() naturally
    // turns things off after 30s of no fresh '1's.
  }
}

// -----------------------------------------------------------------------------
// Non-blocking timeout: once ALARM_DURATION_MS has passed since the last
// motion trigger, switch the alarm off.
void updateAlarmTimer(uint32_t now) {
  if (alarmActive && (now - alarmStartTime >= ALARM_DURATION_MS)) {
    alarmActive = false;
    Serial.println(">> 30s elapsed with no new motion — Alarm OFF");
  }
}

// -----------------------------------------------------------------------------
// Drives the Red/Blue LEDs based on whether the alarm is active and which
// display mode is currently selected. Fully non-blocking (uses millis()
// for the blink timing instead of delay()).
void updateLEDs(uint32_t now) {
  if (!alarmActive) {
    setLEDs(false, false);
    return;
  }

  if (currentMode == MODE_SOLID) {
    setLEDs(true, true); // Both LEDs on steadily
  } else {
    // MODE_BLINK: alternate which LED is lit every BLINK_INTERVAL_MS.
    if (now - lastBlinkToggle >= BLINK_INTERVAL_MS) {
      lastBlinkToggle = now;
      blinkShowRed = !blinkShowRed;
    }
    setLEDs(blinkShowRed, !blinkShowRed); // exactly one of the two is on
  }
}

// -----------------------------------------------------------------------------
// Convenience helper to set both LEDs at once.
void setLEDs(bool redOn, bool blueOn) {
  digitalWrite(PIN_LED_RED,  redOn  ? HIGH : LOW);
  digitalWrite(PIN_LED_BLUE, blueOn ? HIGH : LOW);
}

// -----------------------------------------------------------------------------
// Prints LDR reading, dark/light status, alarm state, current mode, and
// remaining alarm time, once per second — useful for debugging without
// flooding the Serial Monitor.
void printTelemetry(uint32_t now) {
  if (now - lastTelemetryTime < TELEMETRY_PERIOD_MS) return;
  lastTelemetryTime = now;

  int ldrValue = analogRead(PIN_LDR);
  bool roomIsDark = (ldrValue < DARK_THRESHOLD);

  uint32_t remainingMs = 0;
  if (alarmActive) {
    uint32_t elapsed = now - alarmStartTime;
    remainingMs = (elapsed < ALARM_DURATION_MS) ? (ALARM_DURATION_MS - elapsed) : 0;
  }

  Serial.print("[STATUS] LDR=");
  Serial.print(ldrValue);
  Serial.print(" (");
  Serial.print(roomIsDark ? "DARK" : "LIGHT");
  Serial.print(") | Mode=");
  Serial.print(currentMode == MODE_BLINK ? "BLINK" : "SOLID");
  Serial.print(" | Alarm=");
  Serial.print(alarmActive ? "ON" : "OFF");
  Serial.print(" | TimeLeft=");
  Serial.print(remainingMs / 1000);
  Serial.println("s");
}
