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



//______________________________________________________________________________
// constants
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

//----- wifi credentials -------------------------------------------------------
const std::string ssid = "Vodafone-33596891";
const std::string password = "ibaaudjhcy7j7p9";

//----- mqtt credentials -------------------------------------------------------
const std::string mqtt_broker = "jarvis";



//______________________________________________________________________________
// global variables
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

//----- network ----------------------------------------------------------------
MQTT mqtt(mqtt_broker);
Sniffer sniffer(5);
Synchronizer sync;


//______________________________________________________________________________
// functions callback
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

void callback(std::string topic, std::string data) {
	Serial.print("Message arrived on topic: ");
	Serial.print(topic.c_str());
	Serial.print(". Message: ");
	Serial.print(data.c_str());
}


//______________________________________________________________________________
// Arduino callback
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯


//----- put your setup code here, to run once ----------------------------------
void setup() {
	Serial.begin(9600);

	delay(10);
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
	
	mqtt.start();

	auto callback_packet = [&](std::list<Packet>& list) {
		std::cout << "MQTT callback for sniffing terminated" << std::endl;
		if (!WiFi.isConnected())
			WiFi.reconnect();
		mqtt.reconnect();
		for (auto iterator = list.begin(); iterator != list.end(); ++iterator)
			mqtt.publish(Synchronizer::topic_sniffing_data, "packet information");
	};
	sniffer.start(callback_packet);


	sync.init(&mqtt);
	sync.request_is_sniffing();
	Synchronizer::wait_setup();
}


//----- put your main code here, to run repeatedly -----------------------------
void loop() {

	std::cout << "Waiting for sniff/start command..." << std::endl;
	Synchronizer::wait_sniff();


	mqtt.publish(Synchronizer::topic_sniffing_data, "begin");
	mqtt.disconnect();
	sniffer.sniff();
	mqtt.reconnect();
	mqtt.publish(Synchronizer::topic_sniffing_data, "terminated");
	
}