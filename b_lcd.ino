#define I2C_ADDR 0x3F // <<----- Add your address here.  Find it from I2C Scanner
#define BACKLIGHT_PIN 3
#define En_pin 2
#define Rw_pin 1
#define Rs_pin 0
#define D4_pin 4
#define D5_pin 5
#define D6_pin 6
#define D7_pin 7
int lcdDataSwap = 0; //toggle the bottom row of the lcd to show min/max, then the count of how many times ph up or ph down has been used.
String lastUpdate;

LiquidCrystal_I2C lcd(I2C_ADDR, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin);

void lcd_init()
{
    //INIT LCD
    lcd.begin(16, 2);
    lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
    lcd.setBacklight(HIGH);
    lcd.home();
}

void lcd_update(LcdState LCD_STATE, String value1 = "val1", String value2 = "val2")
{
    String currentUpdate = String(LCD_STATE) + value1 + value2;

    if(currentUpdate == lastUpdate) return;

    lastUpdate = currentUpdate;
    switch (LCD_STATE)
    {
    case LCD_STATE_WELCOME:
        lcd.setCursor(0, 0);
        lcd.print("Saisho Ph Doser v1");
        displayPh(value1, value2,1);
        break;
    case LCD_STATE_START:
        lcd.clear();
        lcd.print("Saisho Ph Doser v1");
        lcd.setCursor(0, 1);
        lcd.print("Hit btn to start.");
        break;
    case LCD_STATE_CHECKING:
        displayPh(value1, value2);
        lcd.setCursor(0, 1);
        lcd.print("                ");
        lcd.setCursor(0, 1);
        lcd.print("Status: Checking");
        break;
    case LCD_STATE_DOSING_DOWN:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Status: Dosing");
        lcd.setCursor(0, 1);
        lcd.print("PH Down");
        break;
    case LCD_STATE_MIXING:
        displayPh(value1, value2);
        lcd.setCursor(0, 1);
        lcd.print("                ");
        lcd.setCursor(0, 1);
        lcd.print("Status: Mixing");
        break;
    }
}

void displayPh(String value1, String value2, int level = 0)
{
    lcd.setCursor(0, level);
    lcd.print("                ");
    lcd.setCursor(0, level);
    lcd.print("PH: ");
    lcd.print(value1);
    lcd.setCursor(9, level);
    lcd.print("-");
    lcd.setCursor(11, level); //put cursor on bottom
    lcd.print(value2);
}
