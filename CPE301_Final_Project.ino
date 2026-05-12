#include <LiquidCrystal.h>
#include <Wire.h>
#include <RTClib.h>

RTC_DS1307 rtc;

DateTime now;

// LCD pins <--> Arduino pins
const int RS = 12, EN = 11, D4 = 46, D5 = 48, D6 = 50, D7 = 52;  //we can change these pins later, don't worry about needing to change the pins in your part of the circuit

LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

byte fahrenheit[8] = {
  0b11100,
  0b10000,
  0b11100,
  0b10000,
  0b10000,
  0b00000,
  0b00000,
  0b00000
};

/* ADC SETUP*/

#define RDA 0x80
#define TBE 0x20

unsigned char *ddr_h = (unsigned char *)0x101;
unsigned char *port_h = (unsigned char *)0x102;

volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int *myUBRR0 = (unsigned int *)0x00C4;
volatile unsigned char *myUDR0 = (unsigned char *)0x00C6;

volatile unsigned char *my_ADMUX = (unsigned char *)0x7C;
volatile unsigned char *my_ADCSRB = (unsigned char *)0x7B;
volatile unsigned char *my_ADCSRA = (unsigned char *)0x7A;
volatile unsigned int *my_ADC_DATA = (unsigned int *)0x78;

const int LED_GREEN = 4;   //pin 7, PH3
const int LED_YELLOW = 5;  //pin 8, PH4
const int LED_RED = 6;     //pin 9, PH5
const int LED_BLUE = 3;    //pin 6, PH6

volatile bool buttonONPressed = false;
volatile bool buttonOFFPressed = false;
volatile bool isError = false;
volatile int onState = 0;
bool systemOnState = false;
unsigned long previousMillis = 0;


/* DHT11 SETUP */

#include <DHT.h>

#define DHT_PIN 22
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

const unsigned int water_threshold = 200;
const unsigned int tempThresh = 50;

bool isWaterThreshMet = false;
unsigned int water_value = 0;
float temp_value = 0;
float humidity_value = 0;

void setup() {
  dht.begin();
  Wire.begin();
  U0init(9600);
  adc_init();

  if (!rtc.begin()) {
    printString("Couldn't find RTC. Check connections!\n");
    while (1)
      ;  // Halt execution if RTC is not found
  }

  if (!rtc.isrunning()) {
    printString("RTC is NOT running, setting the time...\n");
    // Set the RTC to the current date and time
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }


  pinSetup();
  updateLEDs();
}

void pinSetup() {
  //LED Setup
  *ddr_h |= (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6);
  *port_h &= ~((1 << 3) | (1 << 4) | (1 << 5) | (1 << 6));
  //BUTTON SETUP
  DDRE &= ~(1 << PE4);  //setup button pin2 PE4 OnBUtton
  PORTE |= (1 << PE4);  //
  DDRE &= ~(1 << PE5);  //setup button pin3 PE5 OFFBUtton
  PORTE |= (1 << PE5);  //
  //LCD Backlight Setup
  DDRA |= (1 << 3);
  PORTA &= ~(1 << 3);
  //Relay/Heatpad Setup
  DDRG |= (1 << 5);
  PORTG &= ~(1 << 5);

  attachInterrupt(digitalPinToInterrupt(2), ISR_OnButton, FALLING);
  attachInterrupt(digitalPinToInterrupt(3), ISR_OffButton, FALLING);

  printString("PIN SETUP WAS FINISHED \n");
}

void ISR_OnButton() {
  buttonONPressed = true;
  systemOnState = true;
  onState = 2;
  updateLEDs();
  printString("ON button pressed\n");
  setupLCD();
  PORTA |= (1 << 3);  //turn on LCD backlight
}

void ISR_OffButton() {
  buttonOFFPressed = true;
  systemOnState = false;
  onState = 0;
  updateLEDs();
  printString("OFF button pressed\n");
  lcd.noDisplay();
  PORTA &= ~(1 << 3);  //turn off LCD backlight
}

void ISR_Error() {
  printString("ERROR: Invalid Reading");
  onState = 3;
  updateLEDs();
  systemOnState = false;
}

void handleButtons() {
  if (buttonONPressed) {
    buttonONPressed = false;
  }
  if (buttonOFFPressed) {
    buttonOFFPressed = false;
  }
}


void updateLEDs() {
  *port_h &= ~((1 << LED_GREEN) | (1 << LED_YELLOW) | (1 << LED_RED) | (1 << LED_BLUE));
  switch (onState) {
    case 1:  // ACTIVE = GREEN
      *port_h |= (1 << LED_GREEN);
      break;
    case 2:  // IDLE = YELLOW
      *port_h |= (1 << LED_YELLOW);
      break;
    case 3:  // ERROR = RED
      *port_h |= (1 << LED_RED);
      break;
    case 0:  // OFF = BLUE
    default:
      *port_h |= (1 << LED_BLUE);
      break;
  }
}

void setupLCD() {
  lcd.createChar(1, fahrenheit);
  lcd.begin(16, 2);
  lcd.print("Temp:");
  lcd.setCursor(7, 0);
  lcd.write((byte)1);
  lcd.setCursor(0, 1);
  lcd.print("Humidity:  %");
  lcd.setCursor(8, 0);
  lcd.print("Water:");
}

void loop() {
  now = rtc.now();
  if (systemOnState) {
    loop2();
  }
}

void loop2() {
  handleButtons();
  water_value = runADC();

  temp_value = dht.readTemperature(true);

  humidity_value = dht.readHumidity();
  isError = (isnan(temp_value) || isnan(humidity_value));

  if (isError) {
    ISR_Error();
  } else {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= (60 * 1000))  //value represents delay in milliseconds
    {
      previousMillis = currentMillis;

      updateHeatPad(temp_value < tempThresh);
      isWaterThreshMet = (water_value > water_threshold);
      updateLCD((int)temp_value, (int)humidity_value, isWaterThreshMet);
      updateSerial((int)temp_value, (int)humidity_value, water_value, isWaterThreshMet);
    }
  }
}

void updateHeatPad(bool state) {
  if (state) {
    PORTG |= (1 << 5);
    onState = 1;  //green
    updateLEDs();
  } else {
    PORTG &= ~(1 << 5);
    onState = 2;  //yellow
    updateLEDs();
  }
}

void updateLCD(int temp, int humid, bool water) {  //putting values in the LCD
  lcd.setCursor(5, 0);
  lcd.print(temp);
  lcd.setCursor(9, 1);
  lcd.print(humid);
  lcd.setCursor(14, 0);
  if (water) {
    lcd.print("hi");
  } else {
    lcd.print("lo");
  }
}


unsigned int runADC() {
  unsigned long sum = 0;
  unsigned int value;
  int i;

  for (i = 0; i < 10; i++) {
    sum += adc_read(0);
  }

  value = sum / 10;

  return value;
}

void adc_init() {
  *my_ADCSRA = 0x87;
  *my_ADCSRB = 0x00;
  *my_ADMUX = 0x40;
}

void updateSerial(int temp, int humid, int water, bool waterThresh) {
  printNumber(now.year());
  printString("/");
  printNumber(now.month());
  printString("/");
  printNumber(now.day());
  printString(" ");
  printNumber(now.hour());
  printString(":");
  printNumber(now.minute());
  printString(":");
  printNumber(now.second());

  printString("\tTemperature: ");
  printNumber(temp);
  printString(" degrees\tHumidity: ");
  printNumber(humid);
  printString("%\tWater Level: ");
  printNumber(water);
  printString(" (");
  printString(waterThresh == 0 ? "Low" : "High");
  printString(")\n");
}

unsigned int adc_read(unsigned char adc_channel_num) {
  *my_ADMUX &= 0xE0;
  *my_ADCSRB &= 0xF7;
  *my_ADMUX |= (adc_channel_num & 0x1F);

  *my_ADCSRA |= 0x40;
  while (*my_ADCSRA & 0x40)
    ;

  return *my_ADC_DATA;
}

void U0init(int U0baud) {
  unsigned long FCPU = 16000000;
  unsigned int tbaud;
  tbaud = (FCPU / 16 / U0baud - 1);

  *myUCSR0A = 0x20;
  *myUCSR0B = 0x18;
  *myUCSR0C = 0x06;
  *myUBRR0 = tbaud;
}

unsigned char U0kbhit() {
  return *myUCSR0A & RDA;
}

unsigned char U0getchar() {
  return *myUDR0;
}

void U0putchar(unsigned char U0pdata) {
  while ((*myUCSR0A & TBE) == 0)
    ;
  *myUDR0 = U0pdata;
}

void printString(const char *str)  //any string printed will go here
{
  while (*str) {
    U0putchar(*str);
    str++;
  }
}

void printNumber(unsigned int num)  //any int printed will go here
{
  if (num >= 1000) U0putchar((num / 1000) + '0');
  if (num >= 100) U0putchar(((num / 100) % 10) + '0');
  if (num >= 10) U0putchar(((num / 10) % 10) + '0');
  U0putchar((num % 10) + '0');
}
