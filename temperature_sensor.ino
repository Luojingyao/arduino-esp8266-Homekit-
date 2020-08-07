/*
 * temperature_sensor.ino
 *
 * This example shows how to:
 * 1. define a temperature sensor accessory and its characteristics (in my_accessory.c).
 * 2. report the sensor value to HomeKit (just random value here, you need to change it to your real sensor value).
 *
 *  Created on: 2020-05-15
 *      Author: Mixiaoxiao (Wang Bin)
 *
 * Note:
 *
 * You are recommended to read the Apple's HAP doc before using this library.
 * https://developer.apple.com/support/homekit-accessory-protocol/
 *
 * This HomeKit library is mostly written in C,
 * you can define your accessory/service/characteristic in a .c file,
 * since the library provides convenient Macro (C only, CPP can not compile) to do this.
 * But it is possible to do this in .cpp or .ino (just not so conveniently), do it yourself if you like.
 * Check out homekit/characteristics.h and use the Macro provided to define your accessory.
 *
 * Generally, the Arduino libraries (e.g. sensors, ws2812) are written in cpp,
 * you can include and use them in a .ino or a .cpp file (but can NOT in .c).
 * A .ino is a .cpp indeed.
 *
 * You can define some variables in a .c file, e.g. int my_value = 1;,
 * and you can access this variable in a .ino or a .cpp by writing extern "C" int my_value;.
 *
 * So, if you want use this HomeKit library and other Arduino Libraries together,
 * 1. define your HomeKit accessory/service/characteristic in a .c file
 * 2. in your .ino, include some Arduino Libraries and you can use them normally
 *                  write extern "C" homekit_characteristic_t xxxx; to access the characteristic defined in your .c file
 *                  write your logic code (eg. read sensors) and
 *                  report your data by writing your_characteristic.value.xxxx_value = some_data; homekit_characteristic_notify(..., ...)
 * done.
 */

#include <Arduino.h>
#include <arduino_homekit_server.h>
#include "wifi_info.h"
//include the Arduino library for your real sensor here, e.g. <DHT.h>
#include "DHTesp.h"
DHTesp dht;
#include <PMserial.h>
SerialPM pms(PMSx003, Serial); 

#define LOG_D(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);

void setup() {
	Serial.begin(9600);
	wifi_connect(); // in wifi_info.h
	my_homekit_setup();
  dht.setup(D6, DHTesp::DHT11);
  pms.init();
}

void loop() {
	my_homekit_loop();
	delay(10);
}

//==============================
// Homekit setup and loop
//==============================

// access your homekit characteristics defined in my_accessory.c
extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t cha_current_temperature;
extern "C" homekit_characteristic_t cha_humidity;
extern "C" homekit_characteristic_t cha_pm25;
extern "C" homekit_characteristic_t cha_aqi;
static uint32_t next_heap_millis = 0;
static uint32_t next_report_millis = 0;

void my_homekit_setup() {
	arduino_homekit_setup(&config);
}

void my_homekit_loop() {
	arduino_homekit_loop();
	const uint32_t t = millis();
	if (t > next_report_millis) {
		// report sensor values every 10 seconds
		next_report_millis = t + 10 * 1000;
		my_homekit_report();
	}
	if (t > next_heap_millis) {
		// show heap info every 5 seconds
		next_heap_millis = t + 5 * 1000;
		LOG_D("Free heap: %d, HomeKit clients: %d",
				ESP.getFreeHeap(), arduino_homekit_connected_clients_count());

	}
}

void my_homekit_report() {
  float h = dht.getHumidity();
  float t = dht.getTemperature();

  int pm25_value = 0;
  uint8_t air_aqi = 0;

  pms.read();
  if (pms){
  pm25_value = pms.pm25;
  }

  if(pm25_value<35){
    air_aqi = 1;
  }
  else if(pm25_value<75.0){
    air_aqi = 2;
  }
  else if(pm25_value<115.0){
    air_aqi = 3;
  }
  else if(pm25_value<150.0){
    air_aqi = 4;
  }
  else if(pm25_value<250.0){
    air_aqi = 5;
  }
  else if(pm25_value<300.0){
    air_aqi = 6;   
  }

	cha_current_temperature.value.float_value = t;
	homekit_characteristic_notify(&cha_current_temperature, cha_current_temperature.value);
  cha_humidity.value.float_value = h;
  homekit_characteristic_notify(&cha_humidity, cha_humidity.value);

  cha_pm25.value.float_value = pm25_value;
  homekit_characteristic_notify(&cha_pm25, cha_pm25.value);
  cha_aqi.value.uint8_value = air_aqi;
  homekit_characteristic_notify(&cha_aqi, cha_aqi.value);
  
 
}

int random_value(int min, int max) {
	return min + random(max - min);
}
