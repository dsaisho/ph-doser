#include <MemoryFree.h>
#include <AnalogButtons.h>
#include <stdlib.h> //used to convert a float to char
#include <avr/eeprom.h>
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <elapsedMillis.h>
#include <SoftwareSerial.h>

#define ANALOG_PIN A1

///////////////////////// STATE ///////////////////////////////////
enum PhState
{
    PH_GOOD,
    PH_HIGH,
    PH_LOW
};
PhState PH_STATE;

enum LcdState
{
    LCD_STATE_CHECKING,
    LCD_STATE_START,
    LCD_STATE_DOSING_DOWN,
    LCD_STATE_MIXING
};

enum AppState
{
    APP_STATE_START,
    APP_STATE_CHECKING,
    APP_STATE_DOSING_START,
    APP_STATE_DOSING_ACTIVE,
    APP_STATE_MIXING
};
AppState APP_STATE = APP_STATE_START;

/////////////////////// VARS //////////////////////////////
boolean adjusting = false; //are we waiting for pump routine to finish(pump ph, wait moments for it ot mix in);
boolean doOnce = true;

String adjustingStatus = "No Adjustments"; // current status on whats going on with the adjustments - "no adjustments", "Motor Running", "Mixing"

int phUpRelayPin = 7;
int motorOnTime = 3000;
int mixWaitTime = 5000;
int updateTimerInterval = 500;
int phUpCount = 0;
int phDownCount = 0;

char buffer[25]; //used for converting float to string

AnalogButtons analogButtons(ANALOG_PIN, INPUT_PULLUP, 1, 30);
Button b1 = Button(239, &btn1Clicked);
Button b2 = Button(29, &btn2Clicked);

elapsedMillis adjustmentTimer;
elapsedMillis updateTimer;

elapsedMillis motorOnTimer;
elapsedMillis mixingTimer;
/////////////////////////// START FUNCTIONS /////////////////////////////////
void btn1Clicked()
{
    Serial.println("Btn1 Clicked");
    APP_STATE = APP_STATE_CHECKING;
}

void btn2Clicked()
{
    Serial.println("Btn2 Clicked");
}

void setup()
{
    Serial.begin(9600); //set baud rate for the hardware serial port_0 to 9600

    pinMode(phUpRelayPin, OUTPUT);
    phDownRelayOff();
    atlasPh_Init();

    lcd_init();
    lcd_update(LCD_STATE_START);

    analogButtons.add(b1);
    analogButtons.add(b2);

    updateTimer = 0;
}

void loop()
{
    analogButtons.check();
    atlasPh_loop();

    switch (APP_STATE)
    {
    case APP_STATE_CHECKING:
        checking();
        break;
    case APP_STATE_DOSING_START:
        dosingStart();
        break;
    case APP_STATE_DOSING_ACTIVE:
        dosingActive();
        break;
    case APP_STATE_MIXING:
        mixing();
        break;
    }
}
void dosingStart()
{
    motorOnTimer = 0;
    lcd_update(LCD_STATE_DOSING_DOWN);
    phDownRelayOn();
    APP_STATE = APP_STATE_DOSING_ACTIVE;
}
void dosingActive()
{
    if (motorOnTimer > motorOnTime)
    {
        phDownRelayOff();
        mixingTimer = 0;
        APP_STATE = APP_STATE_MIXING;
    }
}
void mixing()
{
    lcd_update(LCD_STATE_MIXING, atlasPh_phString(), atlasPh_statusString());
    if (mixingTimer > mixWaitTime)
    {
        APP_STATE = APP_STATE_CHECKING;
    }
}
void checking()
{ //display ph info and do logic to check if we need to add ph

    lcd_update(LCD_STATE_CHECKING, atlasPh_phString(), atlasPh_statusString());

    switch (PH_STATE)
    {
    case PH_GOOD:

        break;
    case PH_HIGH:
        APP_STATE = APP_STATE_DOSING_START;
        break;
    case PH_LOW:

        break;
    }
}

void phDownRelayOn()
{
    digitalWrite(phUpRelayPin, LOW);
}
void phDownRelayOff()
{
    digitalWrite(phUpRelayPin, HIGH);
}
void configureAnalogButtons()
{
    unsigned int value = analogRead(ANALOG_PIN);
    Serial.println(value);
    delay(250);
}
