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
		static std::mutex				_m;
		static std::condition_variable	_cv_setup;
		static std::condition_variable	_cv_sniff;



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
		static void wait_setup();

		static void signal_setup();

		static void wait_sniff();

		static void signal_sniff();



	private:
		//----- internal facilities --------------------------------------------
		static bool _is_my_mac(String mac_address);
};