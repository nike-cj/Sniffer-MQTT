#pragma once

//______________________________________________________________________________
// libraries
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

//----- C++ libraries ----------------------------------------------------------
#include <iostream>
#include <string>
#include <map>
#include <list>
#include <exception>
#include <mutex>

//----- C libraries ------------------------------------------------------------


//----- Arduino libraries ------------------------------------------------------
#include <WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>

//----- ESP32 libraries --------------------------------------------------------
#include "esp_wifi.h"

#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"


//----- custom components ------------------------------------------------------
// #include "WiFi.h"
// #include "Status.h"
// #include "mDNS.h"



//______________________________________________________________________________
// types
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

// packet struct
typedef struct {
	std::string mac_address;	// source MAC address
	int rssi;					// signal strength
	long timestamp;				// time since 1/1/1970	
	int seq_number; 			// sequence number of probe request
	long hash;					// hash of the packet computed with DJB2 algorithm
	//std::unique_ptr<char[]> probe_req_payload; // smart pointer to the probe request dump
	int probe_req_payload_len;	// length of probe request dump

	//TODO add relevant fields
} Packet;

// callback types
typedef void (*SNIFFER_CALLBACK) (std::list<Packet>& packet);



//______________________________________________________________________________
// global variables
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// dimensionality
const int CHANNEL_FIRST = 1;
const int CHANNEL_LAST = 13;



//______________________________________________________________________________
// class
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
class Sniffer
{

	private:
		//----- attributes -----------------------------------------------------
		// sniffing parameters
		int _seconds;
		int _current_channel;

		// status
		bool _is_sniffing = false;


	public:
		//----- static variables -----------------------------------------------
		// list of packet sniffed
		static std::list<Packet> _list_packets;

		// time-handling information
		static WiFiUDP		_ntp_udp;
		static NTPClient	_time_client;

		// callback for external code
		static SNIFFER_CALLBACK _callback_sniffing_terminated;


	public:
		//----- constructors and destructors -----------------------------------
		Sniffer(int seconds, int starting_channel = CHANNEL_FIRST);


		//----- methods --------------------------------------------------------
		// connect
		void start(SNIFFER_CALLBACK callback);

		// sniff packet from one channel
		void sniff();

		// getter for status
		bool is_sniffing();


	private:
		//----- internal facilities --------------------------------------------
		// callback triggered each time a packet is received by the WiFi card
		static void _callback_received_packet(void* buf, wifi_promiscuous_pkt_type_t type);

};