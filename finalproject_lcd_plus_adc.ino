#include <LiquidCrystal.h>

// LCD pins <--> Arduino pins
const int RS = 12, EN = 11, D4 = 5, D5 = 4, D6 = 3, D7 = 2; //we can change these pins later, don't worry about needing to change the pins in your part of the circuit

LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

/* ADC SETUP*/

#define RDA 0x80
#define TBE 0x20  

volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *)0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;
 
volatile unsigned char* my_ADMUX   = (unsigned char*)0x7C;
volatile unsigned char* my_ADCSRB  = (unsigned char*)0x7B;
volatile unsigned char* my_ADCSRA  = (unsigned char*)0x7A;
volatile unsigned int*  my_ADC_DATA = (unsigned int*)0x78;

bool onState = false;

const unsigned int water_threshold = 200;
bool isWaterThreshMet = false;
unsigned int water_value = 0;
unsigned int temp_value = 0;
unsigned int humidity_value = 0;

void setup() {
  setupLCD();
  Serial.begin(9600);
  adc_init();
}

void setupLCD(){
  
  lcd.begin(16, 2);
  lcd.print("Temp:");
  lcd.setCursor(0,1);
  lcd.print("Humidity:");
  lcd.setCursor(8,0);
  lcd.print("Water:");
}

void loop() {
  if (onState){ //The button/switch that turns everything on will update onState
    loop2();
  }
  delay(50);
}

void loop2() {
  
  water_value = runADC();
  
  temp_value = ___; 

  humid_value = ___;

  isWaterThreshMet = (water_value < water_threshold);
  updateLCD(temp_value, humid_value, water_value);
  updateSerial(temp_value, humid_value, water_value, isWaterThreshMet);
}

void updateLCD(int temp, int humid, bool water){ //putting values in the LCD
  
}


unsigned int runADC(){
  unsigned long sum = 0;
  unsigned int value;
  int i;

  for(i = 0; i < 10; i++)
  {
    sum += adc_read(0);
    delay(10);
  }

  value = sum / 10;

  delay(10);

  return value;
}

void adc_init() 
{
  *my_ADCSRA = 0x87;   
  *my_ADCSRB = 0x00;   
  *my_ADMUX  = 0x40;   
}

void updateSerial(int temp, int humid, int water, bool waterThresh)
{
  printString("Temperature: ");
  printNumber(temp);
  printString(" Humidity: ")
  printNumber(humid);
  printString(" Water Level: ")
  printNumber(water);
  printString(" (");
  printNumber(waterThresh);
  printString(")")
}

unsigned int adc_read(unsigned char adc_channel_num)
{
  *my_ADMUX &= 0xE0;                 
  *my_ADCSRB &= 0xF7;                
  *my_ADMUX |= (adc_channel_num & 0x1F);

  *my_ADCSRA |= 0x40;                
  while(*my_ADCSRA & 0x40);         

  return *my_ADC_DATA;
}

void U0init(int U0baud)
{
  unsigned long FCPU = 16000000;
  unsigned int tbaud;
  tbaud = (FCPU / 16 / U0baud - 1);

  *myUCSR0A = 0x20;
  *myUCSR0B = 0x18;
  *myUCSR0C = 0x06;
  *myUBRR0  = tbaud;
}

unsigned char U0kbhit()
{
  return *myUCSR0A & RDA;
}

unsigned char U0getchar()
{
  return *myUDR0;
}

void U0putchar(unsigned char U0pdata)
{
  while((*myUCSR0A & TBE) == 0);
  *myUDR0 = U0pdata;
}

void printString(const char *str) //any string printed will go here
{
  while(*str)
  {
    U0putchar(*str);
    str++;
  }
}

void printNumber(unsigned int num) //any int printed will go here
{
  if(num >= 1000) U0putchar((num / 1000) + '0');
  if(num >= 100)  U0putchar(((num / 100) % 10) + '0');
  if(num >= 10)   U0putchar(((num / 10) % 10) + '0');
  U0putchar((num % 10) + '0');
}