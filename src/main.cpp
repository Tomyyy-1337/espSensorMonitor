#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h> 
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPI.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "index_html.h"	
#include "IntervalArray60.cpp"

void display(String line1, String line2 = "", String line3 = "", String line4 = "", String line5 = "", String line6 = "");
void initWebSocket();
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
String processor(const String& var);
void sendWebSocketMessage(String type, int value);

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);
WiFiManager wm;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

int WLAN_RESET_PIN = 14;

int POTENTIOMETER_PIN = 39;
int potentiometerPercent = 0;

IntervalArray60 minuite_values(60000 / 60);
IntervalArray60 hour_values(60000);
IntervalArray60 day_values(60000 * 60);

void setup() {
	Serial.begin(9600);

	// Init Pins
	analogReadResolution(12);
	pinMode(POTENTIOMETER_PIN, INPUT);
	pinMode(WLAN_RESET_PIN, INPUT_PULLUP);

	// Init Display
	u8g2.begin();      
	u8g2.setFont(u8g2_font_ncenB08_tr);

	// Connect to WiFi
	display("Connect to espConfig", "SSID: espConfig", "Password: none");
	if (!wm.autoConnect("espConfig")) {
		Serial.println("Failed to connect, press reset to try again or reset settings using the wlan reset button");
		display("Failed to connect", "Press reset to try again", "or reset settings using", "the wlan reset button");
		while (true) {
			if (digitalRead(WLAN_RESET_PIN) == LOW) {
				wm.resetSettings();
				ESP.restart();
			}
		}
	}
	String ip = WiFi.localIP().toString();
	display("Connected to", WiFi.SSID(), "IP: " + ip);
	Serial.println("Connected to " + WiFi.SSID() + " IP: " + ip);

	// Start Web Server
	initWebSocket();
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send_P(200, "text/html", index_html, processor);
	});
	server.begin();

	int pot_value = map(analogRead(POTENTIOMETER_PIN), 0, 4095, 0, 100);
	minuite_values.add_unchecked(pot_value);
	hour_values.add_unchecked(pot_value);
	day_values.add_unchecked(pot_value);
}

void loop() {
	// Check for WLAN Reset
	if (!digitalRead(WLAN_RESET_PIN)) {
		wm.resetSettings();
		ESP.restart();
  	}
	ws.cleanupClients();

	// Read Potentiometer
	potentiometerPercent = map(analogRead(POTENTIOMETER_PIN), 0, 4095, 0, 100);
	sendWebSocketMessage("P", potentiometerPercent);

	minuite_values.add(potentiometerPercent);
	hour_values.add(potentiometerPercent);
	day_values.add(potentiometerPercent);

	String filler = potentiometerPercent < 100 ? potentiometerPercent < 10 ? "  " : " " : "";
	display(
		"Connected to ", 
		WiFi.SSID(), 
		"IP: " + WiFi.localIP().toString(), 
		"Helligkeit: "+ filler + String(potentiometerPercent) + "%"
	);

	delay(500);
}

void sendWebSocketMessage(String type, int value) {
	String message = type + ":" + String(value);
	ws.textAll(message);
}

void initWebSocket() {
	ws.onEvent(onEvent);
	server.addHandler(&ws);
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
	switch (type) {
		case WS_EVT_CONNECT:
			Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
			break;
		case WS_EVT_DISCONNECT:
			Serial.printf("WebSocket client #%u disconnected\n", client->id());
			break;
		case WS_EVT_DATA:
			handleWebSocketMessage(arg, data, len);
			break;
		case WS_EVT_PONG:
		case WS_EVT_ERROR:
			break;
	}
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
	AwsFrameInfo *info = (AwsFrameInfo*)arg;
	if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
		data[len] = 0;
		String message = (char*)data;
		if (message == "M") {
			String response = "D:";
			for (int i = 0; i < minuite_values.length; i++) {
				response += String(minuite_values.values[i]) + ",";
			}
			ws.textAll(response);
		} else if (message == "H") {
			String response = "D:";
			for (int i = 0; i < hour_values.length; i++) {
				response += String(hour_values.values[i]) + ",";
			}
			ws.textAll(response);
		} else if (message == "D") {
			String response = "D:";
			for (int i = 0; i < day_values.length; i++) {
				response += String(day_values.values[i]) + ",";
			}
			ws.textAll(response);
		}
		
	}
}

String processor(const String& var) {
	if (var == "POT_STATE") {
		return String(potentiometerPercent);
	} 
	return String();
}

void display(String line1, String line2, String line3, String line4, String line5, String line6) {
	u8g2.clearBuffer();
	u8g2.setCursor(3, 10);
	u8g2.print(line1);
	u8g2.setCursor(3, 20);
	u8g2.print(line2);
	u8g2.setCursor(3, 30);
	u8g2.print(line3);
	u8g2.setCursor(3, 40);
	u8g2.print(line4);
	u8g2.setCursor(3, 50);
	u8g2.print(line5);
	u8g2.setCursor(3, 60);
	u8g2.print(line6);
	u8g2.sendBuffer();
}