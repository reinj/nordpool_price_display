#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include "GxEPD2_display_selection_new_style.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <lwip/dns.h>
#include "credentials.h"
//#include "common.h"

float* avgPriceToday;
float* avgPriceTomorrow;
float* realPriceNow;

// Zero-allocation stream helpers — read one char at a time, no heap involved.
static bool streamSkipTo(WiFiClient& s, const char* pat, unsigned long timeoutMs) {
  int pLen = strlen(pat), matched = 0;
  unsigned long deadline = millis() + timeoutMs;
  while (millis() < deadline) {
    if (!s.available()) { yield(); continue; }
    char c = s.read();
    matched = (c == pat[matched]) ? matched + 1 : (c == pat[0] ? 1 : 0);
    if (matched == pLen) return true;
  }
  return false;
}
static long streamReadLong(WiFiClient& s, unsigned long timeoutMs) {
  long v = 0; bool any = false;
  unsigned long deadline = millis() + timeoutMs;
  while (millis() < deadline) {
    if (!s.available()) { yield(); continue; }
    char c = s.read();
    if (c >= '0' && c <= '9') { v = v * 10 + (c - '0'); any = true; }
    else if (any) break;
  }
  return any ? v : -1;
}
static float streamReadFloat(WiFiClient& s, unsigned long timeoutMs) {
  long intPart = 0, fracPart = 0, fracDiv = 1;
  bool hasDot = false, any = false;
  unsigned long deadline = millis() + timeoutMs;
  while (millis() < deadline) {
    if (!s.available()) { yield(); continue; }
    char c = s.read();
    if (c >= '0' && c <= '9') {
      if (!hasDot) { intPart = intPart * 10 + (c - '0'); }
      else         { fracPart = fracPart * 10 + (c - '0'); fracDiv *= 10; }
      any = true;
    } else if (c == '.') { hasDot = true; }
    else if (any) break;
  }
  return any ? (float)intPart + (float)fracPart / fracDiv : 0.0f;
}
// After streamReadFloat consumes the '}' that closes an entry object, check whether
// the next significant character is ']' (end of array) or ',' (another entry).
static bool streamAtArrayEnd(WiFiClient& s, unsigned long timeoutMs) {
  unsigned long deadline = millis() + timeoutMs;
  while (millis() < deadline) {
    if (!s.available()) { yield(); continue; }
    char c = s.read();
    if (c == ']') return true;
    if (c == ',') return false;
  }
  return true;
}

const float redPrice = 12.00;

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
void syncTime();

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

void syncTime()
{
  IPAddress ntpIP;
  if (WiFi.hostByName("pool.ntp.org", ntpIP))
    Serial.printf("NTP DNS ok: %s\n", ntpIP.toString().c_str());
  else
    Serial.println("NTP DNS failed");

  configTime(0, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
  setenv("TZ", "EET-2EEST,M3.5.0/3,M10.5.0/4", 1);
  tzset();

  Serial.print("Syncing time");
  unsigned long start = millis();
  while (time(nullptr) < 1000000000ul) {
    if (millis() - start > 30000) {
      Serial.printf(" timed out (ts=%lu)\n", (unsigned long)time(nullptr));
      return;
    }
    delay(500);
    Serial.print(".");
  }
  Serial.printf(" done (ts=%lu)\n", (unsigned long)time(nullptr));
}

void getValuesFromServer()
{
  float todayAvg = 0, tomorrowAvg = 0, currentPrice = 0;

  time_t now = time(nullptr);
  struct tm localNow;
  localtime_r(&now, &localNow);

  struct tm todayMidnight = localNow;
  todayMidnight.tm_hour = 0; todayMidnight.tm_min = 0; todayMidnight.tm_sec = 0;
  time_t todayStart = mktime(&todayMidnight);

  struct tm tomorrowMidnight = localNow;
  tomorrowMidnight.tm_mday += 1;
  tomorrowMidnight.tm_hour = 0; tomorrowMidnight.tm_min = 0; tomorrowMidnight.tm_sec = 0;
  time_t tomorrowStart = mktime(&tomorrowMidnight);

  struct tm tomorrowEndTm = localNow;
  tomorrowEndTm.tm_mday += 1;
  tomorrowEndTm.tm_hour = 23; tomorrowEndTm.tm_min = 59; tomorrowEndTm.tm_sec = 59;
  time_t tomorrowEnd = mktime(&tomorrowEndTm);

  char startStr[30], endStr[30];
  struct tm utcStart, utcEnd;
  gmtime_r(&todayStart, &utcStart);
  gmtime_r(&tomorrowEnd, &utcEnd);
  strftime(startStr, sizeof(startStr), "%Y-%m-%dT%H:%M:%S.000Z", &utcStart);
  strftime(endStr,   sizeof(endStr),   "%Y-%m-%dT%H:%M:%S.000Z", &utcEnd);

  char url[200];
  snprintf(url, sizeof(url),
    "https://dashboard.elering.ee/api/nps/price?start=%s&end=%s",
    startStr, endStr);

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;
  https.setTimeout(10000);
  https.useHTTP10(true);  // HTTP/1.0 prevents chunked Transfer-Encoding so getStream() is parseable

  Serial.printf("Free heap before HTTPS: %u\n", ESP.getFreeHeap());

  if (https.begin(client, url)) {
    int code = https.GET();
    Serial.printf("Elering HTTP %d (%s)\n", code, https.errorToString(code).c_str());

    if (code == 200) {
      WiFiClient* stream = https.getStreamPtr();
      float todayTotal = 0, tomorrowTotal = 0;
      int todayCount = 0, tomorrowCount = 0;
      float foundPrice = -1;
      time_t currentHourStart = now - (now % 3600);

      if (streamSkipTo(*stream, "\"ee\":[", 8000)) {
        while (streamSkipTo(*stream, "\"timestamp\":", 3000)) {
          long ts = streamReadLong(*stream, 1000);
          if (ts < 0) break;
          if (!streamSkipTo(*stream, "\"price\":", 2000)) break;
          float price = streamReadFloat(*stream, 1000);

          if ((time_t)ts == currentHourStart) foundPrice = price;
          if ((time_t)ts >= todayStart && (time_t)ts < tomorrowStart) {
            todayTotal += price; todayCount++;
          } else if ((time_t)ts >= tomorrowStart && (time_t)ts <= tomorrowEnd) {
            tomorrowTotal += price; tomorrowCount++;
          }

          // streamReadFloat consumed the '}' closing this entry; stop if ']' follows
          if (streamAtArrayEnd(*stream, 1000)) break;
        }
      }

      Serial.printf("todayCount=%d tomorrowCount=%d foundPrice=%.2f\n",
        todayCount, tomorrowCount, foundPrice);

      if (todayCount > 0)    todayAvg     = todayTotal    / todayCount;
      if (tomorrowCount > 0) tomorrowAvg  = tomorrowTotal / tomorrowCount;
      currentPrice = (foundPrice >= 0) ? foundPrice : todayAvg;
    }
    https.end();
  } else {
    Serial.println("HTTPS begin failed");
  }

  avgPriceToday    = new float(todayAvg);
  avgPriceTomorrow = new float(tomorrowAvg);
  realPriceNow     = new float(currentPrice);
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

  Serial.printf("Connected, IP: %s\n",
    WiFi.localIP().toString().c_str());

  // Force DNS via lwIP — ESP8266 sometimes doesn't pick it up from DHCP.
  // This core maps ip_addr_t to ip4_addr_t (lwIP1 style: single .addr field).
  ip_addr_t dns1, dns2;
  IP4_ADDR(&dns1, 8, 8, 8, 8);
  IP4_ADDR(&dns2, 8, 8, 4, 4);
  dns_setserver(0, &dns1);
  dns_setserver(1, &dns2);
  delay(100);

  IPAddress testIP;
  Serial.printf("DNS google.com: %s\n",
    WiFi.hostByName("google.com", testIP) ? testIP.toString().c_str() : "FAILED");

  syncTime();
  display.init(115200);
  screen.displayUpdate();
  display.hibernate();
}

void loop() {};