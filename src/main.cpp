#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include "GxEPD2_display_selection_new_style.h"
#include <ESP8266WiFi.h>
//#include "common.h"

float* avgPriceToday;
float* avgPriceTomorrow;
float* realPriceNow;

const float redPrice = 12.00;

const char* ssid = "yourNetworkName";
const char* password = "yourNetworkPassword";

const uint8_t letter_H[]      PROGMEM = {0x66, 0x66, 0x66, 0x7e, 0x7e, 0x66, 0x66, 0x66};
const uint8_t letter_O[]      PROGMEM = {0x18, 0x3c, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x18};
const uint8_t letter_M[]      PROGMEM = {0xe7, 0xe7, 0xff, 0xdb, 0xdb, 0xc3, 0xc3, 0xc3};
const uint8_t letter_N[]      PROGMEM = {0x66, 0x76, 0x76, 0x7e, 0x7e, 0x6e, 0x6e, 0x66};
const uint8_t letter_E[]      PROGMEM = {0x7e, 0x7e, 0x60, 0x78, 0x78, 0x60, 0x7e, 0x7e};
const uint8_t letter_K[]      PROGMEM = {0x66, 0x66, 0x6c, 0x78, 0x78, 0x6c, 0x66, 0x66};
const uint8_t letter_S[]      PROGMEM = {0x3c, 0x7e, 0x62, 0x38, 0x1c, 0x46, 0x7e, 0x3c};
const uint8_t letter_I[]      PROGMEM = {0x7e, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x7e, 0x7e};
const uint8_t letter_T[]      PROGMEM = {0x7e, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18};
const uint8_t letter_AE[]     PROGMEM = {0x42, 0x18, 0x3c, 0x66, 0x66, 0x7e, 0x7e, 0x66};
const uint8_t letter_A[]      PROGMEM = {0x18, 0x3c, 0x66, 0x66, 0x7e, 0x7e, 0x66, 0x66};

void getValuesFromServer();

class prices {
  private:
    uint16_t cur_x, cur_y, avg_color;
    uint16_t hourlyText, hourlyBg;
    int textHeight = 110;

  public:

    void todayAverage(float averagePrice)
    {
      cur_x = 3;
      cur_y = (display.height() / 2) + 4;

      if (averagePrice >= redPrice)
      {
        avg_color = GxEPD_WHITE;
        display.fillRect(0, 
                        (display.height() / 2) + 2, 
                        (display.width() / 2) - 2,
                        13,
                        GxEPD_RED);
      }

      display.drawBitmap(cur_x, cur_y, letter_T, 8, 8, avg_color);  cur_x += 8;
      display.drawBitmap(cur_x, cur_y, letter_AE, 8, 8, avg_color); cur_x += 8;
      display.drawBitmap(cur_x, cur_y, letter_N, 8, 8, avg_color);  cur_x += 8;
      display.drawBitmap(cur_x, cur_y, letter_A, 8, 8, avg_color);  cur_x += 8;
      display.drawBitmap(cur_x, cur_y, letter_N, 8, 8, avg_color);  cur_x += 8;
      display.drawBitmap(cur_x, cur_y, letter_E, 8, 8, avg_color);  cur_x += 14;

      display.drawBitmap(cur_x, cur_y, letter_K, 8, 8, avg_color); cur_x += 8;
      display.drawBitmap(cur_x, cur_y, letter_E, 8, 8, avg_color); cur_x += 8;
      display.drawBitmap(cur_x, cur_y, letter_S, 8, 8, avg_color); cur_x += 8;
      display.drawBitmap(cur_x, cur_y, letter_K, 8, 8, avg_color); cur_x += 8;
      display.drawBitmap(cur_x, cur_y, letter_M, 8, 8, avg_color); cur_x += 8;
      display.drawBitmap(cur_x, cur_y, letter_I, 8, 8, avg_color); cur_x += 8;
      display.drawBitmap(cur_x, cur_y, letter_N, 8, 8, avg_color); cur_x += 8;
      display.drawBitmap(cur_x, cur_y, letter_E, 8, 8, avg_color); cur_x += 8;

      display.setFont(&FreeMonoBold12pt7b);
      display.setTextColor(GxEPD_BLACK);
      display.setCursor(25,textHeight);
      display.print(averagePrice);
    }

    void tomorrowAverage(float averagePrice)
    {
      cur_x = (display.width() / 2) + 10;
      cur_y = (display.height() / 2) + 4;

      avg_color = GxEPD_BLACK;

      if (averagePrice >= redPrice)
      {
        avg_color = GxEPD_WHITE;
        display.fillRect((display.width() / 2) + 2, 
                        (display.height() / 2) + 2, 
                        display.width(),
                        13,
                        GxEPD_RED);
      }

      display.drawBitmap(cur_x, cur_y, letter_H, 8, 8, avg_color); cur_x += 8;
      display.drawBitmap(cur_x, cur_y, letter_O, 8, 8, avg_color); cur_x += 8;
      display.drawBitmap(cur_x, cur_y, letter_M, 8, 8, avg_color); cur_x += 8;
      display.drawBitmap(cur_x, cur_y, letter_N, 8, 8, avg_color); cur_x += 8;
      display.drawBitmap(cur_x, cur_y, letter_E, 8, 8, avg_color); cur_x += 14;

      display.drawBitmap(cur_x, cur_y, letter_K, 8, 8, avg_color); cur_x += 8;
      display.drawBitmap(cur_x, cur_y, letter_E, 8, 8, avg_color); cur_x += 8;
      display.drawBitmap(cur_x, cur_y, letter_S, 8, 8, avg_color); cur_x += 8;
      display.drawBitmap(cur_x, cur_y, letter_K, 8, 8, avg_color); cur_x += 8;
      display.drawBitmap(cur_x, cur_y, letter_M, 8, 8, avg_color); cur_x += 8;
      display.drawBitmap(cur_x, cur_y, letter_I, 8, 8, avg_color); cur_x += 8;
      display.drawBitmap(cur_x, cur_y, letter_N, 8, 8, avg_color); cur_x += 8;
      display.drawBitmap(cur_x, cur_y, letter_E, 8, 8, avg_color); cur_x += 8;

      display.setFont(&FreeMonoBold12pt7b);
      display.setTextColor(GxEPD_BLACK);
      display.setCursor(155,textHeight);
      display.print(averagePrice);
    }

    void hourlyNow(float priceRightNow)
    {

      if (priceRightNow >= redPrice)
      {
        hourlyText  = GxEPD_WHITE;
        hourlyBg    = GxEPD_RED;
      } else {
        hourlyText  = GxEPD_BLACK;
        hourlyBg    = GxEPD_WHITE;
      }

      display.setRotation(1);
      display.setFont(&FreeMonoBold24pt7b);
      display.setTextColor(hourlyText);
      int16_t tbx, tby; uint16_t tbw, tbh;

      display.getTextBounds((priceRightNow >= 100.00 ? "000.00" : "00.00"), 0, 0, &tbx, &tby, &tbw, &tbh);
      // center the bounding box by transposition of the origin:
      uint16_t x = ((display.width() - tbw) / 2) - tbx;
      display.fillScreen(GxEPD_WHITE);
      display.setCursor(x, 45);
      display.fillRect(0, (display.height() / 2), display.width(), -(display.height() / 2), hourlyBg);
      display.print(priceRightNow);

    }

};

class Screen{
  public:

  void wifiConnected()
  {
    const char* connectedTo = "Connected to SSID ";
    char connectSentence[50];
    strcpy(connectSentence, connectedTo);
    strcat(connectSentence, ssid);

    display.setFullWindow();
    display.firstPage();
    do
    {
      display.setRotation(1);
      display.setFont(&FreeMonoBold12pt7b);
      display.setTextColor(GxEPD_BLACK);
      int16_t tbx, tby; uint16_t tbw, tbh;
      display.getTextBounds(connectSentence, 0, 0, &tbx, &tby, &tbw, &tbh);
      // center the bounding box by transposition of the origin:
      uint16_t x = ((display.width() - tbw) / 2) - tbx;
      uint16_t y = ((display.height() - tbh) / 2) - tby;
      display.fillScreen(GxEPD_WHITE);
      display.setCursor(x, y + 12);
      display.print(connectedTo);
      display.getTextBounds("000.000.000.000", 0, 0, &tbx, &tby, &tbw, &tbh);
      x = ((display.width() - tbw) / 2) - tbx;
      y = ((display.height() - tbh) / 2) - tby;
      display.setCursor(x, y - 12);
      display.print(WiFi.localIP());
    }
    while (display.nextPage());

    delete connectedTo;
    connectedTo = nullptr;
  }


  void displayUpdate()
  {
    prices price;
    
    display.setFullWindow();
    display.firstPage();
    do
    {
      getValuesFromServer();
      price.hourlyNow(*realPriceNow);
      price.tomorrowAverage(*avgPriceTomorrow);
      price.todayAverage(*avgPriceToday);

      display.drawFastHLine(0, (display.height() / 2) - 1, display.width(), GxEPD_BLACK);
      display.drawFastHLine(0, (display.height() / 2) + 0, display.width(), GxEPD_BLACK);
      display.drawFastHLine(0, (display.height() / 2) + 1, display.width(), GxEPD_BLACK);

      display.drawLine((display.width() / 2), (display.height() / 2), (display.width() / 2), display.height(), GxEPD_BLACK);
      display.drawLine((display.width() / 2) - 1, (display.height() / 2), (display.width() / 2) - 1, display.height(), GxEPD_BLACK);
      display.drawLine((display.width() / 2) + 1, (display.height() / 2), (display.width() / 2) + 1, display.height(), GxEPD_BLACK);
      
    }
    while (display.nextPage());

    delete avgPriceToday; 
    delete avgPriceTomorrow; 
    delete realPriceNow;

    avgPriceToday     = nullptr; 
    avgPriceTomorrow  = nullptr; 
    realPriceNow      = nullptr; 
  }
};

void getValuesFromServer()
{
  //Demo code to get values from server
  float a = 12.58;
  float b = 11.52;
  float c = 132.45;

  avgPriceToday = new float(a);
  avgPriceTomorrow = new float(b);
  realPriceNow = new float(c);

}


void setup()
{
  Screen screen;
  Serial.begin(115200);
  Serial.println();

  WiFi.begin(ssid,password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  display.init(115200);
  screen.displayUpdate();
  display.hibernate();
}

void loop() {};