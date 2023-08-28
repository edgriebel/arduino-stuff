#include <Arduino.h>
#include "esp_sleep.h"
#include "HT_SSD1306Wire.h"

#define DEEP_SLEEP (0)
#define LIGHT_SLEEP (1)
#define DISPLAY (1)

#if DISPLAY
SSD1306Wire  factory_display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // addr , freq , i2c group , resolution , rst
#endif

void setup() {
	Serial.begin(115200);
  Serial.printf("Compiled on %s %s, file %s\n", __DATE__, __TIME__, __FILE__);

#if DISPLAY
	factory_display.init();
	factory_display.clear();
	factory_display.display();
  factory_display.setFont(ArialMT_Plain_16);
#endif /* DISPLAY */
}

int count = 0;
int deep_sleep_sec = 5;
int light_sleep_sec = 1;
char buf[256];
void loop() {
  count++;
  Serial.printf("LOOP START, count=%d\n", count);
  delay(10);

#if DISPLAY
  sprintf(buf, "Loop Start, c=%d", (int)count);
  factory_display.clear();
  factory_display.drawString(0, 0, buf);
  // factory_display.drawString(0, 10, count);
  factory_display.display();
  delay(10);
#endif /* DISPLAY */

#if DEEP_SLEEP
  Serial.println("Deep sleep");
  esp_deep_sleep(1000000LL * deep_sleep_sec);
#endif /* DEEP_SLEEP */

#if LIGHT_SLEEP
  Serial.println("Light sleep");
  esp_sleep_enable_timer_wakeup(1000000LL * light_sleep_sec);
  esp_light_sleep_start();
  Serial.println("Light sleep DONE");
#endif /* LIGHT_SLEEP */

  Serial.println("LOOP END");
  delay(10);
}
