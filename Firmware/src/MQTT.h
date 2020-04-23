#pragma once

//______________________________________________________________________________
// libraries
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

//----- C++ libraries ----------------------------------------------------------
#include <iostream>
#include <cstdlib>
#include <string>
#include <map>
#include <exception>
#include <mutex>
#include <thread>
#include <sstream>

//----- C libraries ------------------------------------------------------------


//----- Arduino libraries ------------------------------------------------------
#include <WiFi.h>
#include <PubSubClient.h>

//----- ESP32 libraries --------------------------------------------------------


//----- custom components ------------------------------------------------------
#include "Log.h"



//______________________________________________________________________________
// types
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

// callback types
typedef void (*MQTT_CALLBACK) (std::string topic, std::string data);

// enumerative
enum QoS {
	AT_MOST_ONCE	= 0,
	AT_LEAST_ONCE	= 1,
	EXACTLY_ONCE	= 2
};

// aggregating struct
typedef struct {
	std::string		topic;
	MQTT_CALLBACK	callback;
	QoS				qos;
} topic_t;



//______________________________________________________________________________
// class
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
class MQTT: public PubSubClient
{

	private:
		//----- attributes -----------------------------------------------------
		// connection info
		WiFiClient*		_wifi;
		std::string		_client_id;
		std::string		_broker_address;
		int				_broker_port;
		std::string		_broker_uri;
		std::string		_broker_username;
		std::string		_broker_password;

		// subscribing info
		std::map<std::string, topic_t>	_subscription_callbacks;

		// synchronization info
		std::mutex		_m;



	public:
		//----- constructors and destructors -----------------------------------
		MQTT(std::string broker_address, int port = 1883);
		~MQTT();



		//----- methods --------------------------------------------------------

		// connect
		void start();

		// disconnect
		void stop();

		void reconnect();

		// send data
		void publish(std::string topic, std::string message, bool retained = false);

		// receive data
		void subscribe(std::string topic, MQTT_CALLBACK func_ptr, QoS qos = QoS::AT_LEAST_ONCE);

		// stop receiving data
		void unsubscribe(std::string topic);



	private:
		//----- internal facilities --------------------------------------------
		// event handler
		void _event_handler(char* topic, byte* message, unsigned int length);

		// find interesting MQTT subscription callback
		MQTT_CALLBACK _find_subscription(std::string topic);

		// unsubscribe all topics
		void _unsubscribe_all();

		// re-subscribe all already interesting topics
		void _resubscribe_all();

};