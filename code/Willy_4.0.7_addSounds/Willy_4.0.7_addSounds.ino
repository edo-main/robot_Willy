#include <PS2X_lib.h>
#include <FastLED.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =============================================
// ================= НАСТРОЙКИ =================
// =============================================
#define PS2_DAT  30
#define PS2_CMD  11
#define PS2_SEL  10
#define PS2_CLK  31

#define BUZZER_PIN 32        // Зуммер

#define HEADLIGHT_RELAY_PIN 12

// Пины моторов
const int motorLeftA  = 9;
const int motorLeftB  = 8;
const int motorRightA = 5;
const int motorRightB = 3;

// Задние LED
#define REAR_LED_PIN   2
#define NUM_LEDS_REAR  8
CRGB rearLeds[NUM_LEDS_REAR];

// OLED
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// =============================================
// ================ СТРУКТУРЫ =================
// =============================================
struct LedState {
    bool enabled = true;
    int mode = 0;
    uint8_t hue = 0;
} leds;

struct Timers {
    unsigned long lastBlink = 0;
    unsigned long greenPress = 0;
    unsigned long redPress = 0;
    unsigned long selectPress = 0;
} timers;

struct RobotState {
    int speedMode = 1;
    float speedMultiplier = 0.60;
    bool enabled = false;
    bool tankMode = false;
    bool controlsInverted = false;
} robot;

struct ButtonStates {
    bool start = false;
    bool green = false;
    bool pink = false;
    bool padUp = false;
    bool padDown = false;
    bool select = false;
    bool red = false;
} lastButtons;

PS2X ps2x;

// Буфер джойстика
static int joystickBuffer[2][5] = {{128,128,128,128,128},{128,128,128,128,128}};
static int bufferIndex = 0;
static int lastValid[2] = {128, 128};
static unsigned long lastSpikeTime[2] = {0, 0};
static bool spikeActive[2] = {false, false};

bool headlightsOn = false;

// =============================================
// ================= КОНСТАНТЫ =================
// =============================================
const int DEADZONE_LOW  = 118;
const int DEADZONE_HIGH = 138;
const int MINSPEED      = 50;

// ====================== ЗВУКИ ======================
// Прототипы функций (чтобы можно было вызывать до их определения)
void beep(int frequency = 1200, int duration = 60);
void beepShort();
void beepMedium();
void beepLong();

// =============================================
// ================== SETUP ====================
// =============================================
void setup() {
    pinMode(motorLeftA,  OUTPUT);
    pinMode(motorLeftB,  OUTPUT);
    pinMode(motorRightA, OUTPUT);
    pinMode(motorRightB, OUTPUT);
    
    TCCR2B = (TCCR2B & 0xF8) | 0x02;
    TCCR3B = (TCCR3B & 0xF8) | 0x02;
    TCCR4B = (TCCR4B & 0xF8) | 0x02;

    FastLED.addLeds<WS2812B, REAR_LED_PIN, GRB>(rearLeds, NUM_LEDS_REAR);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        while(1);
    }
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT);
    randomSeed(analogRead(A1));

    pinMode(HEADLIGHT_RELAY_PIN, OUTPUT);
    digitalWrite(HEADLIGHT_RELAY_PIN, LOW);
    
    pinMode(BUZZER_PIN, OUTPUT);
}

// =============================================
// ================== LOOP =====================
// =============================================
void loop() {
    ps2x.read_gamepad();
   
    updateSystemState();
    if (!robot.enabled) {
        displayOFF();
        delay(5);
        return;
    }

    updateSpeedControls();
    updateLedControls();
    updateMotors();
    updateLeds();
    displayStatus();
    delay(5);
}

// =============================================
// ============= СИСТЕМНОЕ СОСТОЯНИЕ ===========
// =============================================
void updateSystemState() {
    bool currentStart = ps2x.Button(PSB_START);
    bool green = ps2x.Button(PSB_GREEN);
    bool red = ps2x.Button(PSB_RED);
    unsigned long now = millis();

    if (currentStart && !lastButtons.start) {
        robot.enabled = !robot.enabled;
        if (robot.enabled) {
            beepMedium();          // Включение
            delay(250);
            beep(2200, 100);
        } else {
            beepLong();        
            delay(250);
            beep(400, 100);         // Выключение
            resetMotorsAndLeds();
        }
    }

    if (green && !lastButtons.green) {
        timers.greenPress = now;
        lastButtons.green = true;
    }
    else if (!green && lastButtons.green) {
        if (now - timers.greenPress < 1000) {
            robot.tankMode = !robot.tankMode;
            resetMotors();
            if (robot.tankMode)
                beep(1800, 180);     // Tank
            else
                beep(900, 180);      // Normal
        }
        lastButtons.green = false;
    }

    if (red && !lastButtons.red) {
        timers.redPress = now;
        lastButtons.red = true;
    }
    else if (!red && lastButtons.red) {
        if (now - timers.redPress >= 700) {
            robot.controlsInverted = !robot.controlsInverted;
            beep(1200, 100);
        }
        lastButtons.red = false;
    }

    lastButtons.start = currentStart;
}

// =============================================
// ================ СКОРОСТЬ ===================
// =============================================
void updateSpeedControls() {
    bool padUp = ps2x.Button(PSB_PAD_UP);
    bool padDown = ps2x.Button(PSB_PAD_DOWN);

    if (!ps2x.Button(PSB_SELECT)) {
        if (padUp && !lastButtons.padUp && robot.speedMode < 3) {
            robot.speedMode++;
            updateSpeedMultiplier();
            beep(1200, 50);
            
        }
        if (padDown && !lastButtons.padDown && robot.speedMode > 1) {
            robot.speedMode--;
            updateSpeedMultiplier();
            beep(600, 50);
        }
    }

    lastButtons.padUp = padUp;
    lastButtons.padDown = padDown;
}

void updateSpeedMultiplier() {
    const float multipliers[] = {0.60f, 0.80f, 1.00f};
    robot.speedMultiplier = multipliers[robot.speedMode - 1];
}

// =============================================
// ================ LED УПРАВЛЕНИЕ ==============
// =============================================
void updateLedControls() {
    bool pink = ps2x.Button(PSB_PINK);
    bool select = ps2x.Button(PSB_SELECT);
    unsigned long now = millis();

    if (pink && !lastButtons.pink) {
        headlightsOn = !headlightsOn;
        leds.enabled = headlightsOn;
        digitalWrite(HEADLIGHT_RELAY_PIN, headlightsOn ? HIGH : LOW);
        
        if (headlightsOn)
            beep(1500, 60);
        else
            beep(800, 100);
    }
    lastButtons.pink = pink;

    if (select && !lastButtons.select) {
        timers.selectPress = now;
        lastButtons.select = true;
    }
    else if (!select && lastButtons.select) {
        lastButtons.select = false;
    }

    if (select && (now - timers.selectPress >= 300)) {
        if (ps2x.ButtonPressed(PSB_PAD_LEFT)) {
            leds.mode = 0;
            leds.enabled = true;
            updateBasicLedColors();
            beepShort();
        }
        else if (ps2x.ButtonPressed(PSB_PAD_DOWN)) {
            leds.mode = 1;
            leds.enabled = true;
            beepShort();
        }
        else if (ps2x.Button(PSB_R2)) {
            leds.hue = (leds.hue + 10) % 256; //  
            updateBasicLedColors();
            beep(600, 50);
        }
        else if (ps2x.Button(PSB_L2)) {
            leds.hue = (leds.hue + 246) % 256;
            updateBasicLedColors();
            beep(1200, 50);
        }
    }
}

// =============================================
// =============== МОТОРЫ ======================
// =============================================
void updateMotors() {
    resetMotors();
    if (robot.tankMode) {
        updateTankModeMotors();
    } else {
        updateNormalModeMotors();
    }
}

void updateTankModeMotors() {
    if (ps2x.Button(PSB_SELECT)) return;
    int speed = 255 * robot.speedMultiplier;
    if (robot.controlsInverted) {
        if (ps2x.Button(PSB_L1)) analogWrite(motorRightA, speed);
        if (ps2x.Button(PSB_L2)) analogWrite(motorRightB, speed);
        if (ps2x.Button(PSB_R1)) analogWrite(motorLeftA,  speed);
        if (ps2x.Button(PSB_R2)) analogWrite(motorLeftB,  speed);
    } else {
        if (ps2x.Button(PSB_L1)) analogWrite(motorLeftB,  speed);
        if (ps2x.Button(PSB_L2)) analogWrite(motorLeftA,  speed);
        if (ps2x.Button(PSB_R1)) analogWrite(motorRightB, speed);
        if (ps2x.Button(PSB_R2)) analogWrite(motorRightA, speed);
    }
}

void updateNormalModeMotors() {
    int rawY = ps2x.Analog(PSS_LY);
    int rawX = ps2x.Analog(PSS_LX);
    rawY = filterSpike(rawY, 0);
    rawX = filterSpike(rawX, 1);
    joystickBuffer[0][bufferIndex] = rawY;
    joystickBuffer[1][bufferIndex] = rawX;
    bufferIndex = (bufferIndex + 1) % 5;
    int joyY = averageBuffer(joystickBuffer[0]);
    int joyX = averageBuffer(joystickBuffer[1]);
    driveMotors(joyY, joyX);
}

// =============================================
// ============ ВСПОМОГАТЕЛЬНЫЕ ================
// =============================================
int filterSpike(int raw, int axis) {
    if (raw == 255) {
        if (!spikeActive[axis]) {
            lastSpikeTime[axis] = millis();
            spikeActive[axis] = true;
            return lastValid[axis];
        } else if (millis() - lastSpikeTime[axis] >= 30) {
            lastValid[axis] = 255;
            return 255;
        }
        return lastValid[axis];
    }
    spikeActive[axis] = false;
    lastValid[axis] = raw;
    return raw;
}

int averageBuffer(int* buf) {
    long sum = 0;
    for (int i = 0; i < 5; i++) sum += buf[i];
    return sum / 5;
}

void driveMotors(int joyY, int joyX) {
    if (joyY < DEADZONE_LOW) {
        int speed = map(joyY, DEADZONE_LOW, 0, MINSPEED, 255) * robot.speedMultiplier;
        handleForward(speed, joyX);
    }
    else if (joyY > DEADZONE_HIGH) {
        int speed = map(joyY, DEADZONE_HIGH, 255, MINSPEED, 255) * robot.speedMultiplier;
        handleBackward(speed, joyX);
    }
    else if (joyX < DEADZONE_LOW || joyX > DEADZONE_HIGH) {
        int turnSpeed = map(joyX < DEADZONE_LOW ? joyX : joyX,
                            joyX < DEADZONE_LOW ? DEADZONE_LOW : DEADZONE_HIGH,
                            joyX < DEADZONE_LOW ? 0 : 255,
                            MINSPEED, 255) * robot.speedMultiplier;
        if (robot.controlsInverted) {
            if (joyX < DEADZONE_LOW) {
                analogWrite(motorLeftB,  turnSpeed);
                analogWrite(motorRightA, turnSpeed);
            } else {
                analogWrite(motorLeftA,  turnSpeed);
                analogWrite(motorRightB, turnSpeed);
            }
        } else {
            if (joyX < DEADZONE_LOW) {
                analogWrite(motorLeftB,  turnSpeed);
                analogWrite(motorRightA, turnSpeed);
            } else {
                analogWrite(motorLeftA,  turnSpeed);
                analogWrite(motorRightB, turnSpeed);
            }
        }
    }
}

void handleForward(int speed, int joyX) {
    if (robot.controlsInverted) {
        if (joyX < DEADZONE_LOW) {
            int t = map(joyX, DEADZONE_LOW, 0, 0, speed);
            analogWrite(motorRightB, constrain(speed - t, 0, 255));
            analogWrite(motorLeftB,  speed);
        } else if (joyX > DEADZONE_HIGH) {
            int t = map(joyX, DEADZONE_HIGH, 255, 0, speed);
            analogWrite(motorRightB, speed);
            analogWrite(motorLeftB,  constrain(speed - t, 0, 255));
        } else {
            analogWrite(motorRightB, speed);
            analogWrite(motorLeftB,  speed);
        }
    } else {
        if (joyX < DEADZONE_LOW) {
            int t = map(joyX, DEADZONE_LOW, 0, 0, speed);
            analogWrite(motorLeftA,  constrain(speed - t, 0, 255));
            analogWrite(motorRightA, speed);
        } else if (joyX > DEADZONE_HIGH) {
            int t = map(joyX, DEADZONE_HIGH, 255, 0, speed);
            analogWrite(motorLeftA,  speed);
            analogWrite(motorRightA, constrain(speed - t, 0, 255));
        } else {
            analogWrite(motorLeftA,  speed);
            analogWrite(motorRightA, speed);
        }
    }
}

void handleBackward(int speed, int joyX) {
    if (robot.controlsInverted) {
        if (joyX < DEADZONE_LOW) {
            int t = map(joyX, DEADZONE_LOW, 0, 0, speed);
            analogWrite(motorRightA, constrain(speed - t, 0, 255));
            analogWrite(motorLeftA,  speed);
        } else if (joyX > DEADZONE_HIGH) {
            int t = map(joyX, DEADZONE_HIGH, 255, 0, speed);
            analogWrite(motorRightA, speed);
            analogWrite(motorLeftA,  constrain(speed - t, 0, 255));
        } else {
            analogWrite(motorRightA, speed);
            analogWrite(motorLeftA,  speed);
        }
    } else {
        if (joyX < DEADZONE_LOW) {
            int t = map(joyX, DEADZONE_LOW, 0, 0, speed);
            analogWrite(motorLeftB,  constrain(speed - t, 0, 255));
            analogWrite(motorRightB, speed);
        } else if (joyX > DEADZONE_HIGH) {
            int t = map(joyX, DEADZONE_HIGH, 255, 0, speed);
            analogWrite(motorLeftB,  speed);
            analogWrite(motorRightB, constrain(speed - t, 0, 255));
        } else {
            analogWrite(motorLeftB,  speed);
            analogWrite(motorRightB, speed);
        }
    }
}

// =============================================
// ================== LED ======================
// =============================================
void updateLeds() {
    if (!leds.enabled && leds.mode != 1) {
        fill_solid(rearLeds, NUM_LEDS_REAR, CRGB::Black);
        FastLED.show();
        return;
    }
    if (leds.mode == 0) updateBasicLedColors();
    else if (leds.mode == 1) updateBlinkingLeds(500, CRGB::Red);
}

void updateBasicLedColors() {
    if (leds.enabled) {
        fill_solid(rearLeds, NUM_LEDS_REAR, CHSV(leds.hue, 255, 255));
    } else {
        fill_solid(rearLeds, NUM_LEDS_REAR, CRGB::Black);
    }
    FastLED.show();
}

void updateBlinkingLeds(int interval, CRGB color) {
    static bool state = false;
    if (millis() - timers.lastBlink >= interval) {
        state = !state;
        fill_solid(rearLeds, NUM_LEDS_REAR, state ? color : CRGB::Black);
        FastLED.show();
        timers.lastBlink = millis();
        beep(300, 15);
        delay(30);
        beep(120, 30);
    }
}

// =============================================
// ================= ДИСПЛЕЙ ===================
// =============================================
void displayStatus() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
   
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.print(robot.tankMode ? "TANK" : "NORMAL");
    display.setTextSize(3);
    display.setCursor(0, 30);
    display.print("SPD:");
    display.print(robot.speedMode);
    if (robot.controlsInverted) display.print(" R");
    display.display();
}

void displayOFF() {
    display.clearDisplay();
    display.setTextSize(4);
    display.setCursor(20, 16);
    display.print("OFF");
    display.display();
}

// =============================================
// ================== СБРОС ====================
// =============================================
void resetMotorsAndLeds() {
    resetMotors();
    fill_solid(rearLeds, NUM_LEDS_REAR, CRGB::Black);
    FastLED.show();
}

void resetMotors() {
    analogWrite(motorLeftA,  0);
    analogWrite(motorLeftB,  0);
    analogWrite(motorRightA, 0);
    analogWrite(motorRightB, 0);
}

// ====================== ЗВУКИ ======================
void beep(int frequency, int duration) {
    tone(BUZZER_PIN, frequency, duration);
}

void beepShort()  { beep(1400, 40); }
void beepMedium() { beep(1000, 80); }
void beepLong()   { beep(800,  150); }