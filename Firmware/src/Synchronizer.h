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
#include <condition_variable>

//----- C libraries ------------------------------------------------------------


//----- Arduino libraries ------------------------------------------------------
#include <WiFi.h>
#include <ArduinoJson.h>

//----- ESP32 libraries --------------------------------------------------------


//----- custom components ------------------------------------------------------
#include "MQTT.h"
#include "Sniffer.h"
#include "Led.h"
#include "Log.h"



//______________________________________________________________________________
// types
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯




//______________________________________________________________________________
// class
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
class Synchronizer
{

	public:
		//----- constants ------------------------------------------------------
		// topics setup
		static const std::string topic_discovery_req;
		static const std::string topic_discovery_res;
		static const std::string topic_blink;

		// topics sniffing
		static const std::string topic_sniffing_start;
		static const std::string topic_sniffing_data;
		static const std::string topic_sniffing_stop;
		static const std::string topic_sniffing_sync;


	private:
		//----- attributes -----------------------------------------------------
		// connection info
		static MQTT*	_mqtt;

		// controlled module
		static Sniffer*	_sniffer;

		// synchronization info
		static std::mutex	_m;
		static bool			_is_sniffing;
		static int			_server_challenge;



	public:
		//----- constructors and destructors -----------------------------------
		Synchronizer();



		//----- methods --------------------------------------------------------

		// connect
		void init(MQTT* mqtt, Sniffer* sniffer);

		// synchronization message
		void ask_is_sniffing();

		// envelop MAC address in JSON message
		static std::string jsonify_mac();



	private:
		//----- MQTT callbacks -------------------------------------------------
		static void provide_mac(std::string topic, std::string data);

		static void blink(std::string topic, std::string data);

		static void start_sniff(std::string topic, std::string data);



	public:
		//----- synchronization facilities -------------------------------------
		static bool is_sniffing();

		static void terminate_sniff();



	private:
		//----- internal facilities --------------------------------------------
		static bool _is_my_mac(String mac_address);
};