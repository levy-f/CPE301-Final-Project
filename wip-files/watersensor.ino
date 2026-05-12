#define RDA 0x80
#define TBE 0x20  

volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *)0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;
 
volatile unsigned char* my_ADMUX  = (unsigned char*)0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*)0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*)0x7A;
volatile unsigned char* my_ADCL   = (unsigned char*)0x78;
volatile unsigned char* my_ADCH   = (unsigned char*)0x79;

const unsigned int THRESHOLD = 100;
volatile bool buttonPressed = false;
unsigned long lastPress = 0;

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

void U0putchar(unsigned char U0pdata)
{
  while((*myUCSR0A & TBE)==0);
  *myUDR0 = U0pdata;
}


void printString(const char *str)
{
  while (*str != '\0')
  {
    U0putchar(*str);
    str++;
  }
}

void newLine()
{
  U0putchar('\r');
  U0putchar('\n');
}

void printNumber(unsigned int num)
{
  if (num >= 1000)
  {
    U0putchar((num / 1000) + '0');
    U0putchar(((num / 100) % 10) + '0');
    U0putchar(((num / 10) % 10) + '0');
    U0putchar((num % 10) + '0');
  }
  else if (num >= 100)
  {
    U0putchar((num / 100) + '0');
    U0putchar(((num / 10) % 10) + '0');
    U0putchar((num % 10) + '0');
  }
  else if (num >= 10)
  {
    U0putchar((num / 10) + '0');
    U0putchar((num % 10) + '0');
  }
  else
  {
    U0putchar(num + '0');
  }
}

void adc_init()
{
  *my_ADMUX = 0x40;    
  *my_ADCSRB = 0x00;   
  *my_ADCSRA = 0x87;   
}

unsigned int adc_read(unsigned char adc_channel_num)
{
  *my_ADMUX &= 0xE0;                
  *my_ADCSRB &= 0xF7;                
  *my_ADMUX |= (adc_channel_num & 0x1F);

  *my_ADCSRA |= 0x40;                

  while((*my_ADCSRA & 0x40) != 0);  

  unsigned char low  = *my_ADCL;
  unsigned char high = *my_ADCH;

  unsigned int val = ((unsigned int)high << 8) | low;
  return val;
}

void ISR_button(){
  buttonPressed = true;
}

void setup(){
  U0init(9600);
  adc_init();

  DDRE &= ~(1 << PE4);
  PORTE |= (1 << PE4);
  attachInterrupt(digitalPinToInterrupt(2), ISR_button, FALLING);

  printString("Press button for water level");
  newLine();
}

void loop() 
{
  if(buttonPressed){
    buttonPressed = false;
    unsigned long now = millis();
    if(now-lastPress >= 1000){
      lastPress = now;

      unsigned int waterValue = adc_read(0);
      printString("Water Level: ");
      if(waterValue > THRESHOLD){
        printString("High");
      }
      else{
        printNumber(waterValue);
      }
      newLine();
    }
  }
}







