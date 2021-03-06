//______________________________________________________________________________
// libraries
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

//----- C++ standard libraries -------------------------------------------------
#include <string>
#include <iostream>

//----- hardware libraries -----------------------------------------------------
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

//----- custom libraries -------------------------------------------------------
#include "MQTT.h"
#include "Synchronizer.h"
#include "Sniffer.h"
#include "Log.h"



//______________________________________________________________________________
// constants
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

//----- wifi credentials -------------------------------------------------------
const std::string ssid = "<INSERT WIFI NAME>";
const std::string password = "<INSERT WIFI PASSWORD>";

//----- mqtt credentials -------------------------------------------------------
const std::string mqtt_broker = "<INSERT BROKER HOSTNAME>";



//______________________________________________________________________________
// global variables
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

//----- network ----------------------------------------------------------------
MQTT mqtt(mqtt_broker);
Sniffer sniffer;
Synchronizer sync;



//______________________________________________________________________________
// functions signatures
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

void connect_wifi(std::string ssid, std::string password);



//______________________________________________________________________________
// Arduino callback
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯


//----- put your setup code here, to run once ----------------------------------
void setup() {
	Serial.begin(9600);

	delay(10);
	connect_wifi(ssid, password);
	
	mqtt.start();

	auto callback_packet = [&](std::list<Packet>& list) {
		std::cout << "MQTT callback for sniffing terminated" << std::endl;
		if (!WiFi.isConnected())
			WiFi.reconnect();
		mqtt.reconnect();
		for (auto iterator = list.begin(); iterator != list.end(); ++iterator) {
			std::string msg = iterator->jsonify();
			mqtt.publish(Synchronizer::topic_sniffing_data, msg);
		}
	};
	sniffer.start(callback_packet);


	sync.init(&mqtt, &sniffer);
	sync.ask_is_sniffing();
	
	std::cout << "Waiting for sniff/start command..." << std::endl;
}


//----- put your main code here, to run repeatedly -----------------------------
void loop() {

	// check if server requested to sniff
	if (sync.is_sniffing()) {
		// actually sniff
		mqtt.disconnect();
		sniffer.sniff();
		mqtt.reconnect();

		// signal sniffing terminated
		sync.terminate_sniff();
		std::cout << "Waiting for sniff/start command..." << std::endl;
	}

	// check MQTT messages
	mqtt.loop();
	delay(1000);
}



//______________________________________________________________________________
// functions
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

void connect_wifi(std::string ssid, std::string password) {
	// We start by connecting to a WiFi network
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(ssid.c_str());

	WiFi.begin(ssid.c_str(), password.c_str());

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}

	Serial.println("");
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());
}