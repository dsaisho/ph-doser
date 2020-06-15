
#define rx 2
#define tx 3

String inputstring = "";
String sensorstring = "";

boolean input_string_complete = false;  //have we received all the data from the PC
boolean sensor_string_complete = false; //have we received all the data from the Atlas Scientific product

float pH;
float phMax = 5.9;
float phMin = 5.4;
float phTarget = 5.6;

SoftwareSerial myserial(rx, tx);

void atlasPh_Init()
{
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
String atlasPh_phString()
{
    String phString = dtostrf(pH, 1, 2, buffer);
    return phString;
}
String atlasPh_statusString()
{

    switch (PH_STATE)
    {
    case PH_GOOD:
        return "good";
        break;
    case PH_HIGH:
        return "high";
        break;
    case PH_LOW:
        return "low";
        break;
    }
}
void atlasPh_loop()
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

        //uncomment this section to see how to convert the pH reading from a string to a float
        if (isdigit(sensorstring[0]))
        {                                //if the first character in the string is a digit
            pH = sensorstring.toFloat(); //convert the string to a floating point number so it can be evaluated by the Arduino
            if (pH >= phMax)
            {
                PH_STATE = PH_HIGH;
            }
            if (pH <= phMin)
            {
                PH_STATE = PH_LOW;      
            }
            if (pH <= phMax && pH >= phMin)
            {
                PH_STATE = PH_GOOD;
            }
        }
        else
        {
            Serial.println("FROM SENSOR");
            Serial.println(sensorstring); //send that string to the PC's serial monitor
        }
        sensorstring = "";              //clear the string
        sensor_string_complete = false; //reset the flag used to tell if we have received a completed string from the Atlas Scientific product
    }
}
