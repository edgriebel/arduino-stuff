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

/********************************* lora  *********************************************/

SSD1306Wire  factory_display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // addr , freq , i2c group , resolution , rst

WiFiUDP wifiUdp;
NTP ntp(wifiUdp);

int UPDATE_MINUTES = 1;
int HOUR_ON = 6;
int HOUR_OFF = 23;

char *SSID = "SSID_NAME";
char *PASSWORD = "WIFI_PASSWORD";

void logo(){
	factory_display.clear();
	factory_display.drawXbm(0,5,logo_width,logo_height,(const unsigned char *)logo_bits);
	factory_display.display();
}

void WIFISetUp(void)
{
	// Set WiFi to station mode and disconnect from an AP if it was previously connected
	// WiFi.disconnect(true);
	// delay(100);
	// WiFi.mode(WIFI_STA);
	// WiFi.setAutoConnect(true);
	WiFi.begin(SSID, PASSWORD);//fill in "Your WiFi SSID","Your Password"

	byte count = 0;
	while(WiFi.status() != WL_CONNECTED && count < 40)
	{
		count++;
		delay(500);
    Serial.print(".");
		factory_display.drawString(0, 0, "WiFi Connecting...");
    factory_display.drawProgressBar(0, 10, 40, 5, count);
		factory_display.display();
	}

	factory_display.clear();
	if(WiFi.status() == WL_CONNECTED)
	{
		factory_display.drawString(0, 0, "WiFi Connecting...OK.");
		factory_display.display();
	}
	else
	{
		factory_display.clear();
		factory_display.drawString(0, 0, "WiFi Connecting...Failed");
		factory_display.display();
    Serial.println("WiFi Connecting...Failed");
	}
	factory_display.drawString(0, 8 /*10*/, "WIFI Setup done");
	factory_display.display();
	delay(500);
}


bool resendflag=false;
bool deepsleepflag=false;
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
  ntp.ruleSTD("EST", Last, Sun, Oct, 3, -(5*60)); // last sunday in october 3:00, timezone +60min (+1 GMT)
  ntp.begin();
  ntp.updateInterval(10000);
  getTime();
  Serial.println("Done Setup NTP");
}

void getTime(void) 
{
  Serial.println("getTime NTP");
  if (!WiFi.isConnected())
  {
    Serial.println("WiFi disconnected, skipping NTP time");
    return;
  }

  ntp.update();
  // Serial.println(ntp.formattedTime("%d/%m/%Y %A %T"));
  // Serial.println(ntp.epoch());

  // set RTC to current UTC
  struct timeval tv{ntp.epoch(), 0};
  if (settimeofday(&tv, 0)) 
  {
    Serial.println(errno);
    Serial.println(strerror(errno));
  } else {
    Serial.println("RTC set");
  }
}

void showTime()
{
  factory_display.clear();
  factory_display.setFont(ArialMT_Plain_24);
  time_t epoch = ntp.epoch();
  uint32_t utc = ntp.utc();
  time_t t = time(NULL);
  // Serial.printf("UTC:       %s", asctime(gmtime(&t)));
  // printf("local:     %s", asctime(localtime(&t)));

  putenv("TZ=EST5EDT");
  char est_time_buf[64];
  char est_date_buf[64];
  char pst_time_buf[64];
  int seconds = ntp.seconds();
  strftime(est_time_buf, sizeof (est_time_buf),
    "%H:%M", localtime(&t));
  strftime(est_date_buf, sizeof (est_date_buf),
    "%m/%d/%y", localtime(&t));
  putenv("TZ=PST8PDT");
  strftime(pst_time_buf, sizeof (pst_time_buf),
    "%H:%M", localtime(&t));
  Serial.printf("Date %s EST5EDT: %s :%d PST8PDT %s", est_date_buf, est_time_buf, seconds, pst_time_buf);

  if (ntp.hours() >= HOUR_ON && ntp.hours() <= HOUR_OFF) 
  {
    int spacer = (epoch/60) % 4;
    factory_display.drawString(spacer, 0, est_time_buf); // ntp.formattedTime("%T")); // hh:mm:ss
    // ntp.timeZone(int8_t tzHours)
    factory_display.drawString(spacer, 21, pst_time_buf); // ntp.formattedTime("%T")); // hh:mm:ss
    factory_display.drawString(spacer, 42, est_date_buf); // ntp.formattedTime("%m/%d/%y")); // mm/dd/yy
  } else {
    factory_display.drawString(epoch%20, (epoch/100) % 4, ".");
  }
  int time_width = factory_display.getStringWidth(est_time_buf);
  factory_display.drawProgressBar(time_width+5, 12, (factory_display.width() - time_width - 6), 5, (int) (seconds*100/60));
  factory_display.display();
  Serial.printf("UTC: %d Epoch: %d, delta %d\n", utc, epoch, (epoch - utc));
  // Serial.println(ntp.utc());

  const auto p0 = std::chrono::time_point<std::chrono::system_clock>{};
  const auto p1 = std::chrono::system_clock::now();

  std::time_t epoch_time = std::chrono::system_clock::to_time_t(p0);
  Serial.printf("epoch: %s", std::ctime(&epoch_time));
  std::time_t today_time = std::chrono::system_clock::to_time_t(p1);
  Serial.printf("today: %s", std::ctime(&today_time));
  // std::cout << "hours since epoch: "
  //           << std::chrono::duration_cast<std::chrono::hours>(
  //                 p1.time_since_epoch()).count() 
  //           << '\n';

  // time_t now = time(0);
  // // Convert now to tm struct for local timezone
  // tm* localtm = localtime(&now);
  // Serial.printf("The local date and time is: %s", asctime(localtm));
  #ifdef __STDC_LIB_EXT1__
      struct tm buf;
      char str[26];
      asctime_s(str,sizeof str,gmtime_s(&t, &buf));
      Serial.printf("UTC:       %s", str);
      asctime_s(str,sizeof str,localtime_s(&t, &buf));
      Serial.printf("local:     %s", str);
  #endif

}

void doit() {
	// WiFi.disconnect(); //
	// WiFi.mode(WIFI_STA);
  if (ntp.year() < 2020 || (ntp.epoch() - ntp.utc()) > UPDATE_MINUTES * 60)
  {
    Serial.println("Refreshing time...");
    Serial.println(ntp.epoch() - ntp.utc());
    getTime();
  }
  showTime();

	// WIFIScan(1);

	attachInterrupt(0,interrupt_GPIO0,FALLING);

}


void loop()
{
  // uint64_t chipid=ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).
  // Serial.printf("ESP32ChipID=%04X",(uint16_t)(chipid>>32));//print High 2 bytes
  // Serial.printf("%08X\n",(uint32_t)chipid);//print Low 4bytes.
  digitalWrite(LED, LOW);  
  doit();
  delay(100);
  interrupt_handle();
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
