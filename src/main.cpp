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
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, uint32_t id);
String processor(const String& var);
void sendWebSocketMessage(String type, int value);
void respond_to_request(String tag, SensorData &data, uint32_t id, char identifier);

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);
WiFiManager wm;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

int WLAN_RESET_PIN = 14;

int PHOTORESISTOR_PIN = 39;
int potentiometerPercent = 0;

SensorData lightSensorData;

unsigned long last_update = 0;	

void setup() {
	Serial.begin(9600);

	// Init Pins
	analogReadResolution(12);
	pinMode(PHOTORESISTOR_PIN, INPUT);
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

	int pot_value = map(analogRead(PHOTORESISTOR_PIN), 0, 4095, 0, 100);
	lightSensorData.addSensorDataUnChecked(pot_value);
}

void loop() {
	// Check for WLAN Reset
	if (!digitalRead(WLAN_RESET_PIN)) {
		wm.resetSettings();
		ESP.restart();
  	}
	if (WiFi.status() != WL_CONNECTED) {
		WiFi.reconnect();
	}
	ws.cleanupClients();

	potentiometerPercent = map(analogRead(PHOTORESISTOR_PIN), 0, 4095, 0, 100);
	if (millis() - last_update > 500) {
		last_update = millis();
		sendWebSocketMessage("H", potentiometerPercent);
	}
	lightSensorData.addSensorData(potentiometerPercent);

	String filler = potentiometerPercent < 100 ? potentiometerPercent < 10 ? "  " : " " : "";
	display(
		"Connected to ", 
		WiFi.SSID(), 
		"IP: " + WiFi.localIP().toString(), 
		"Helligkeit: "+ filler + String(potentiometerPercent) + "%"
	);

	delay(100);
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
			handleWebSocketMessage(arg, data, len, client->id());
			break;
		case WS_EVT_PONG:
		case WS_EVT_ERROR:
			break;
	}
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, uint32_t id) {
	AwsFrameInfo *info = (AwsFrameInfo*)arg;
	if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
		data[len] = 0;
		String message = (char*)data;
		switch (message[0]) {
			case 'H':
				respond_to_request("DH", lightSensorData, id, message[1]);
				break;
		}
	}
}

void respond_to_request(String tag, SensorData &data, uint32_t id, char identifier) {
	String response = tag + ":";
	IntervalArray60 &array = data.getArray(identifier);

	unsigned long time_since_last_update = millis() / 100 - array.last_update;
	response += String(time_since_last_update) + ",";
	for (int i = 0; i < array.length; i++) {
		response += String(array.values[i]) + ",";
	}
	ws.text(id, response);
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