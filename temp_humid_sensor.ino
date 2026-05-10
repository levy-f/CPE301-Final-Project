#include <DHT.h>

#define DHT_PIN  22
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

volatile bool buttonPressed = false;
unsigned long lastPress = 0;

void uart_init(unsigned long baud) {
  unsigned int ubrr = (F_CPU / 16 / baud) - 1;
  UBRR0H = (unsigned char)(ubrr >> 8);
  UBRR0L = (unsigned char)(ubrr);
  UCSR0B = (1 << TXEN0);
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_sendChar(char c) {
  while (!(UCSR0A & (1 << UDRE0)));
  UDR0 = c;
}

void uart_print(const char* str) {
  while (*str) uart_sendChar(*str++);
}

void uart_println(const char* str) {
  uart_print(str);
  uart_sendChar('\r');
  uart_sendChar('\n');
}

void uart_printFloat(float val, int decimals) {
  if (val < 0) { uart_sendChar('-'); val = -val; }
  int whole = (int)val;
  char buf[8];
  itoa(whole, buf, 10);
  uart_print(buf);
  uart_sendChar('.');
  float frac = val - whole;
  for (int i = 0; i < decimals; i++) {
    frac *= 10;
    int digit = (int)frac;
    uart_sendChar('0' + digit);
    frac -= digit;
  }
}

void ISR_button() {
  buttonPressed = true;
}

void setup() {
  uart_init(9600);
  DDRE  &= ~(1 << PE4);
  PORTE |=  (1 << PE4);
  attachInterrupt(digitalPinToInterrupt(2), ISR_button, FALLING);
  dht.begin();
  uart_println("Press button for reading");
}

void loop() {
  if (buttonPressed) {
    buttonPressed = false;
    unsigned long now = millis();
    if (now - lastPress >= 1000) {
      lastPress = now;
      float temp  = dht.readTemperature(true);
      float humid = dht.readHumidity();
      if (isnan(temp) || isnan(humid)) {
        uart_println("ERROR: bad sensor reading");
        return;
      }
      uart_print("Temp: "); uart_printFloat(temp, 1);
      uart_print(" F  |  Humid: "); uart_printFloat(humid, 1);
      uart_println(" %");
    }
  }
}