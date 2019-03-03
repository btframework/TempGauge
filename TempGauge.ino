#include "U8glib.h"

#define TEMP_PIN        A0

const float R1 = 1973.00; // ohm
const float A = 13.5282;
const float B = 2.9905;
const float C = -0.0079;

const int xmax = 128;
const int ymax = 62;
const int xcenter = xmax / 2;
const int ycenter = ymax /2 + 10;
const int arc = ymax / 2;

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE | U8G_I2C_OPT_DEV_0);

uint32_t readVcc()
{
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif
  
  delay(2); // Wait for Vref to settle
  
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)) ; // measuring
  
  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both
  
  uint32_t result = (high << 8) | low;
  
  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}

void gauge(uint16_t temp)
{
  uint8_t angle;
  if (temp < 100)
    angle = 0;
  else
    if (temp >= 100 && temp <= 210)
      angle = map(temp, 100, 210, 0, 45);
    else
      if (temp > 210 and temp <= 260)
        angle = map(temp, 210, 260, 45, 90);
      else
        angle = 90;

  if (angle < 45)
    angle += 135;
  else
    angle -= 45;
        
  // draw border of the gauge
  u8g.drawCircle(xcenter, ycenter, arc + 6, U8G_DRAW_UPPER_RIGHT);
  u8g.drawCircle(xcenter, ycenter, arc + 4, U8G_DRAW_UPPER_RIGHT);
  u8g.drawCircle(xcenter, ycenter, arc + 6, U8G_DRAW_UPPER_LEFT);
  u8g.drawCircle(xcenter, ycenter, arc + 4, U8G_DRAW_UPPER_LEFT);
  
  // draw the needle
  float x1 = sin(2 * angle * 2 * 3.14 / 360);
  float y1 = cos(2 * angle * 2 * 3.14 / 360);
  u8g.drawLine(xcenter, ycenter, xcenter + arc * x1, ycenter - arc * y1);
  u8g.drawDisc(xcenter, ycenter, 5, U8G_DRAW_UPPER_LEFT);
  u8g.drawDisc(xcenter, ycenter, 5, U8G_DRAW_UPPER_RIGHT);
  u8g.setFont(u8g_font_chikita); 
  
  // show scale labels
  u8g.drawStr(12, 42, "100");
  u8g.drawStr(20, 18, "155");
  u8g.drawStr(60, 14, "210");
  u8g.drawStr(95, 18, "235");
  u8g.drawStr(105, 42, "260");
  
  // show digital value and align its position
  u8g.setFont(u8g_font_profont22);
  u8g.setPrintPos(54, 60);
  if (temp < 10)
    u8g.print("0");
  if (temp > 99)
    u8g.setPrintPos(47,60);
  u8g.print(temp);
}

void setup()
{
  pinMode(TEMP_PIN, INPUT);

  u8g.setRot180();
  u8g.setFont(u8g_font_chikita);
  u8g.setColorIndex(1);
}

void loop()
{
  static bool doReadVcc = true;
  static uint16_t loopCnt = 0;
  static uint32_t vcc = 0;
  
  if (doReadVcc)
  {
    vcc = readVcc();
    doReadVcc = false;
  }
  
  uint32_t raw = 0;
  for (uint8_t i = 0; i < 100; i++)
    raw += analogRead(TEMP_PIN); // Raw value from ADC.
  raw = raw / 100;
  
  float v = (float)raw * ((float)vcc / 1023.00); // Measured voltage (Volt).

  float r = 0;
  if (vcc - v != 0)
    r = (v * R1) / (vcc - v);
  r = (float)(uint32_t)r;
  if (r <= 0)
    r = 1;

  float lnR = log(r);
  float tC = 1 / ((A + B * lnR + C * lnR * lnR * lnR) / 10000.00) - 273.15;
  if (tC < 0)
    tC = 0;
  
  float tF = 9.00 / 5.00 * tC + 32.00;
  if (tF < 0)
    tF = 0;

  u8g.firstPage();
  do
  {
    gauge((uint16_t)tF);
  } while (u8g.nextPage());

  loopCnt++;
  if (loopCnt > 1000)
  {
    doReadVcc = true;
    loopCnt = 0;
  }
}
