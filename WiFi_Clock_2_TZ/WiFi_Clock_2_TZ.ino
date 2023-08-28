/*
 * Based on HelTec Automation(TM) WIFI_LoRa_32 factory test code:
 * * by Aaron.Lee from HelTec AutoMation, ChengDu, China
 * https://heltec.org
 * https://github.com/HelTecAutomation/Heltec_ESP32
*/

#include <Arduino.h>
#include <WiFi.h>
#include "WiFiUdp.h"
#include "images.h"
#include <Wire.h>
#include <NTP.h>
#include <chrono>
#include "HT_SSD1306Wire.h"
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>
#include "wifi_info.h"

/********************************* lora  *********************************************/

SSD1306Wire  factory_display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // addr , freq , i2c group , resolution , rst

WiFiUDP wifiUdp;
NTP ntp(wifiUdp);

int UPDATE_MINUTES = 60;
int HOUR_ON = 7;
int HOUR_OFF = 22;

void logo(){
	factory_display.clear();
	factory_display.drawXbm(0,5,logo_width,logo_height,(const unsigned char *)logo_bits);
	factory_display.display();
}

void WIFISetUp(bool display=true, bool first_time=true)
{
	WiFi.mode(WIFI_STA);
	// WiFi.setAutoConnect(true);
  if (first_time) {
    WiFi.begin(WIFI_SSID, WIFI_PASSKEY);
  } else {
    WiFi.begin();
  }

	byte count = 0;
	while(WiFi.status() != WL_CONNECTED && count < 50)
	{
		count++;
		delay(50);
    Serial.print(".");
    if (display)
    {
      factory_display.drawString(0, 0, "WiFi Connecting...");
      factory_display.drawProgressBar(0, 10, 40, 5, count);
      factory_display.display();
    }
	}


  char *msg;
	if(WiFi.status() == WL_CONNECTED)
	{
		msg = "WiFi Connecting...OK.";
	}
	else
	{
    msg = "WiFi Connecting...Failed";
	}

  Serial.println(msg);
  if (display) {
    factory_display.clear();
		factory_display.drawString(0, 0, msg);
  	factory_display.drawString(0, 10, "WIFI Setup done");
		factory_display.display();
    delay(500);
  }
}


bool resendflag=false;
bool deepsleepflag=false;
bool lightsleepflag=true;
bool interrupt_flag = false;
void interrupt_GPIO0()
{
	interrupt_flag = true;
}

void interrupt_handle(void)
{
	if(interrupt_flag)
	{
		interrupt_flag = false;
		if(digitalRead(0)==0)
		{
			deepsleepflag=true;
		}
	}
}

void VextON(void)
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void) //Vext default OFF
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, HIGH);
}

void setup()
{
	Serial.begin(115200);
  Serial.printf("Compiled on %s %s, file %s\n", __DATE__, __TIME__, __FILE__);
	VextON();
	delay(100);
	factory_display.init();
	factory_display.clear();
	factory_display.display();
  pinMode(LED, OUTPUT);
  delay(200);

  digitalWrite(LED, LOW);  
	logo();
	delay(300);
	factory_display.clear();
  digitalWrite(LED, HIGH);  

	WIFISetUp();
  setupNtp();
}

void setupNtp(void)
{ 
  if (!WiFi.isConnected())
  {
    Serial.println("WiFi disconnected, skipping NTP setup");
    return;
  }
  Serial.println("Setup NTP");
  ntp.ruleDST("EDT", Last, Sun, Mar, 12, -(4*60)); // last sunday in march 2:00, timetone +120min (+1 GMT + 1h summertime offset)
  ntp.ruleSTD("EST", First, Sun, Nov, 1, -(5*60)); // last sunday in october 3:00, timezone +60min (+1 GMT)
  ntp.begin();
  ntp.updateInterval(10000);
  getTime();
  Serial.println("Done Setup NTP");
}

void getTime(void) 
{

  WIFISetUp(false, false);

  Serial.println("getTime NTP");
  if (!WiFi.isConnected())
  {
    Serial.println("WiFi disconnected, skipping NTP time");
    WiFi.disconnect(true);
    return;
  }

  ntp.update();

  // set RTC to current UTC so we can use builtin POSIX TZ calculations
  struct timeval tv{ntp.epoch(), 0};
  if (settimeofday(&tv, 0)) 
  {
    Serial.println(errno);
    Serial.println(strerror(errno));
  } else {
    Serial.println("RTC set");
  }
  WiFi.disconnect(true);
}

bool is_night()
{
  return ntp.hours() < HOUR_ON || ntp.hours() > HOUR_OFF;
}

void showTime()
{
  factory_display.clear();
  // Bigger font so easier to read
  factory_display.setFont(ArialMT_Plain_24);
  // scale 0-255
  factory_display.setBrightness(100);
  time_t epoch = ntp.epoch();
  uint32_t utc = ntp.utc();
  time_t t = time(NULL);

  char est_time_buf[64];
  char est_date_buf[64];
  char pst_time_buf[64];
  int seconds = ntp.seconds();
  // get time in EDT then in PDT. Let the lib figure out all the hour offsets
  putenv("TZ=EST5EDT");
  strftime(est_time_buf, sizeof (est_time_buf),
    "%H:%M", localtime(&t));
  strftime(est_date_buf, sizeof (est_date_buf),
    "%m/%d/%y", localtime(&t));
  putenv("TZ=PST8PDT");
  strftime(pst_time_buf, sizeof (pst_time_buf),
    "%H:%M", localtime(&t));
  Serial.printf("Date %s EST5EDT: %s :%d PST8PDT %s ", est_date_buf, est_time_buf, seconds, pst_time_buf);

  if (!is_night()) 
  {
    // wiggle display left/right to prevent burn-in
    int spacer = (epoch/60) % 4;
    factory_display.drawString(spacer, 0, est_time_buf); // ntp.formattedTime("%T")); // hh:mm:ss
    // ntp.timeZone(int8_t tzHours)
    factory_display.drawString(spacer, 21, pst_time_buf); // ntp.formattedTime("%T")); // hh:mm:ss
    factory_display.drawString(spacer, 42, est_date_buf); // ntp.formattedTime("%m/%d/%y")); // mm/dd/yy
    int time_width = factory_display.getStringWidth(est_time_buf);
    factory_display.drawProgressBar(time_width+5+spacer, 12+spacer, (factory_display.width() - time_width - 10), 5, (int) (seconds*100/60));
    factory_display.display();
  } else {
    // show a dot so we know it's alive, move it to different spot to prevent burn-in
    factory_display.clear();
    factory_display.drawString(epoch%20, (epoch/100) % 4, ".");
    factory_display.display();
  }
  Serial.printf("UTC: %d Epoch: %d, delta %d\n", utc, epoch, (epoch - utc));
}

void loop()
{
  long long light_sleep_msec = 330; // * (is_night() ? 10 : 1);
  // uint64_t chipid=ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).
  // Serial.printf("ESP32ChipID=%04X",(uint16_t)(chipid>>32));//print High 2 bytes
  // Serial.printf("%08X\n",(uint32_t)chipid);//print Low 4bytes.
  digitalWrite(LED, LOW);  
  // if ((ntp.epoch() - ntp.utc()) > 30)
  if (ntp.year() < 2020 || (ntp.epoch() - ntp.utc()) > UPDATE_MINUTES * 60)
  {
    Serial.println("Refreshing time...");
    getTime();
  }
  showTime();
	attachInterrupt(0,interrupt_GPIO0,FALLING);
  delay(10);
  interrupt_handle();

  if (lightsleepflag)
  {
    Serial.println("Light sleep...");
    delay(10);
    esp_sleep_enable_timer_wakeup(1000LL * light_sleep_msec);
    esp_light_sleep_start();
    Serial.println("Light sleep DONE");
  }

  if(deepsleepflag)
  {
    Serial.printf("Before deepsleepflag\n");
    VextOFF();
    esp_sleep_enable_timer_wakeup(600*1000*(uint64_t)1000);
    esp_deep_sleep_start();
    Serial.printf("after deepsleepflag\n");
  }
  // digitalWrite(LED, HIGH);
  // delay(100);  
}
