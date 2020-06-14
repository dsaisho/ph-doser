#include <MemoryFree.h>
#include <AnalogButtons.h>
#include <stdlib.h> //used to convert a float to char
#include <avr/eeprom.h>
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <elapsedMillis.h>
#include <SoftwareSerial.h>

#define rx 2
#define tx 3

#define ANALOG_PIN A1

#define I2C_ADDR 0x3F // <<----- Add your address here.  Find it from I2C Scanner
#define BACKLIGHT_PIN 3
#define En_pin 2
#define Rw_pin 1
#define Rs_pin 0
#define D4_pin 4
#define D5_pin 5
#define D6_pin 6
#define D7_pin 7
char buffer[25]; //used for converting float to string

boolean adjusting = false; //are we waiting for pump routine to finish(pump ph, wait moments for it ot mix in);
boolean doOnce = true;
boolean input_string_complete = false;  //have we received all the data from the PC
boolean sensor_string_complete = false; //have we received all the data from the Atlas Scientific product

String adjustingStatus = "No Adjustments"; // current status on whats going on with the adjustments - "no adjustments", "Motor Running", "Mixing"
String inputstring = "";                   //a string to hold incoming data from the PC
String sensorstring = "";

float phMax = 5.9;
float phMin = 0;
float phTarget = 5.6;
float pH;

int phStatus = 1; //Current status of the actual ph of the water - 1 good, 2 to low, 3 to high
int phUpRelayPin = A5;
int motorOnTime = 3000;
int mixWaitTime = 5000;
int updateTimerInterval = 500;
int phUpCount = 0;
int phDownCount = 0;
int lcdDataSwap = 0; //toggle the bottom row of the lcd to show min/max, then the count of how many times ph up or ph down has been used.

LiquidCrystal_I2C lcd(I2C_ADDR, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin);
SoftwareSerial myserial(rx, tx);
AnalogButtons analogButtons(ANALOG_PIN, INPUT_PULLUP, 1);

elapsedMillis adjustmentTimer;
elapsedMillis updateTimer;
Button b1 = Button(223, &startBtnClicked);

void startBtnClicked()
{
    Serial.println("BUTTON CLICKED");
}

void setup()
{
    analogButtons.add(b1);
    pinMode(phUpRelayPin, OUTPUT);
    phUpRelayOff();

    //INIT LCD
    lcd.begin(16, 2);
    lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
    lcd.setBacklight(HIGH);
    lcd.home();
    lcd.print("Starting Leo Phi");

    updateTimer = 0;

    Serial.begin(9600);      //set baud rate for the hardware serial port_0 to 9600
    myserial.begin(9600);    //set baud rate for the software serial port to 9600
    inputstring.reserve(10); //set aside some bytes for receiving data from the PC
    sensorstring.reserve(30);
}

void serialEvent()
{
    Serial.println("SERIAL EVENT");           //if the hardware serial port_0 receives a char
    inputstring = Serial.readStringUntil(13); //read the string until we see a <CR>
    input_string_complete = true;             //set the flag used to tell if we have received a completed string from the PC
    Serial.println(input_string_complete);
}

void loop()
{
    analogButtons.check();
    if (updateTimer > updateTimerInterval)
    {
        updateTimer = 0;
        updateLcd();
        if (!adjusting)
            doPhChecks(); //dont do checks while adjusting to prevent adjusting functions from being called again
    }
    atlas_ph_loop();
    //handleAdjustments();
}

void handleAdjustments()
{
    //if we needed to add more ph, set boolean adjusting to true, until the process is over.
    if (adjusting && adjustingStatus != "Mixing")
    {
        phUpRelayOn();                //start motor
        adjustingStatus = "Motor On"; //set new status
        Serial.println("turning ON motor");
        //wait for motor timer to complete, then turn off motor
        if (adjustmentTimer > motorOnTime)
        {
            phUpRelayOff();
            adjustingStatus = "Mixing";
            adjustmentTimer = 0;
            Serial.println("turning off motor");
        }
    }

    //when the motor stops, set timer to wait for solution to mix
    if (adjustingStatus == "Mixing")
    {
        if (adjustmentTimer > mixWaitTime)
        {
            adjusting = false;
            adjustingStatus = "No Adjustments";
        }
    }
}

void doPhChecks()
{

    int rph = round(pH * 100);

    if (rph > (phMax * 100))
        phStatus = 3; // is it to high
    else if (rph < (phMin * 100))
    { // is it to low
        phStatus = 2;
    }
    else
    { //it was good
        phStatus = 1;
    }

    //Serial.println(phStatus);

    switch (phStatus)
    {
    case 1:
        phCheckGood();
        break;
    case 2:
        doAddPhUp();
        break;
    case 3:
        doAddPhDown();
        break;
    default:
        return;
        break;
    }
}

void updateLcd()
{
    String phString = dtostrf(pH, 1, 2, buffer);

    lcd.clear();

    lcd.print("PH: ");
    lcd.print(phString);
    lcd.setCursor(9, 0);
    lcd.print("-");
    lcd.setCursor(11, 0); //put cursor on bottom
    lcd.print(getStatusChar());

    if (!adjusting)
    { //print default info if were not adjusting anything

        if (lcdDataSwap < 10)
        {
            lcd.setCursor(0, 1);
            lcd.print("Min:");
            lcd.print(phMin);
            lcd.print(" Max:");
            lcd.print(phMax);
            lcd.setCursor(7, 1);
            lcd.print(' ');
        }
        if (lcdDataSwap >= 10)
        {
            lcd.setCursor(0, 1);
            lcd.print("Up: ");
            lcd.print(phUpCount);
            lcd.print(" Down: ");
            lcd.print(phDownCount);
        }

        lcdDataSwap++;

        if (lcdDataSwap > 20)
            lcdDataSwap = 0;
    }
    else
    { //display current status of current adjustment on the bottom row only for how long adjustment is going on for.
        lcd.setCursor(0, 1);
        lcd.print("Status: ");
        lcd.print(adjustingStatus);

        lcdDataSwap = 10; //make it so when we go back to default view, it shows the count of pump first(vs info on min and max)
    }
}

String getStatusChar()
{
    switch (phStatus)
    {
    case 1:
        return "GOOD";
    case 2:
        return "LOW";
    case 3:
        return "HIGH";
    }
}

void phCheckGood()
{
    //Serial.println("ph good");
}

void doAddPhUp()
{
    //turn on motor for few moments.
    //wait for few moments to set adjusting volume back, allow time for new ph to stir before we start re checking.
    Serial.println("do Ph Up");

    adjusting = true;
    adjustmentTimer = 0;
    phUpCount++;
}

void doAddPhDown()
{

    adjusting = true;
    adjustmentTimer = 0;
    Serial.println("do Ph DOWN");
    phDownCount++;
}

void phUpRelayOn()
{
    digitalWrite(phUpRelayPin, LOW);
}
void phUpRelayOff()
{
    digitalWrite(phUpRelayPin, HIGH);
}

void atlas_ph_loop()
{ //here we go...
    if (input_string_complete == true)
    {                                  //if a string from the PC has been received in its entirety
        myserial.print(inputstring);   //send that string to the Atlas Scientific product
        myserial.print('\r');          //add a <CR> to the end of the string
        inputstring = "";              //clear the string
        input_string_complete = false; //reset the flag used to tell if we have received a completed string from the PC
    }

    if (myserial.available() > 0)
    {                                        //if we see that the Atlas Scientific product has sent a character
        char inchar = (char)myserial.read(); //get the char we just received
        sensorstring += inchar;              //add the char to the var called sensorstring
        if (inchar == '\r')
        {                                  //if the incoming character is a <CR>
            sensor_string_complete = true; //set the flag
        }
    }

    if (sensor_string_complete == true)
    { //if a string from the Atlas Scientific product has been received in its entirety
        Serial.println("FROM SENSOR");
        Serial.println(sensorstring); //send that string to the PC's serial monitor
                                      //uncomment this section to see how to convert the pH reading from a string to a float
        if (isdigit(sensorstring[0]))
        {                                //if the first character in the string is a digit
            pH = sensorstring.toFloat(); //convert the string to a floating point number so it can be evaluated by the Arduino
            if (pH >= 7.0)
            {                           //if the pH is greater than or equal to 7.0
                Serial.println("high"); //print "high" this is demonstrating that the Arduino is evaluating the pH as a number and not as a string
            }
            if (pH <= 6.999)
            {                          //if the pH is less than or equal to 6.999
                Serial.println("low"); //print "low" this is demonstrating that the Arduino is evaluating the pH as a number and not as a string
            }
        }

        sensorstring = "";              //clear the string
        sensor_string_complete = false; //reset the flag used to tell if we have received a completed string from the Atlas Scientific product
    }
}
