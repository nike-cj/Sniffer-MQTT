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
#include <memory>
#include <sstream>
#include <iomanip>

//----- C libraries ------------------------------------------------------------


//----- Arduino libraries ------------------------------------------------------
#include <WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <ArduinoJson.h>

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

/*  CLASS PACKET: this class defines an object that contains all the details we need about the probe request plus a smart pointer to a memory location 
    where we dumped the entire probe request payload in order to analyze its content (supported rates, vendor, capabilities etc...) */
typedef struct {
	std::string mac_address;	// source MAC address
	int rssi;					// signal strength
	long timestamp;				// time since 1/1/1970
	int seq_number; 			// sequence number of probe request
	long hash;					// hash of the packet computed with DJB2 algorithm
	std::unique_ptr<char[]> probe_req_payload; // smart pointer to the probe request dump
	int probe_req_payload_len;	// length of probe request dump

	std::string jsonify();
	std::string bytes2string(char* data, int len);

} pkt_data;


/*	this structure is used to grab the 802.11 header of the probe request packet which normally consists of 24 byte
	but can be 28 bytes long if it is a 802.11n packet and the "order" bit (LSB in frame control segment) is set to '1'.
	so with this structure we parse the entire header except for the optional "HT control" field, then we check into 
	the frame control bytes if 4 additional byte (HT control) are present and in that case we increase the offset to
	start reading the Element IDs associated to the probe request. */	
typedef struct {
	uint8_t version:2;  // 802.11 version (2 bit)
	uint8_t type:2; // type (management, control, data etc...) (2 bit)
	uint8_t subtype:4; // subtype (probe req, prob resp etc...) (4 bit)
	uint8_t flags; // these are remaining 8 bit of frame control element (we are interested in the last bit, order)
	uint16_t duration:16; // 16 bit
	uint8_t dest_addr[6]; // 6 byte
	uint8_t source_addr[6]; // 6 byte
	uint8_t bssid[6]; // 6 byte
	uint8_t frag_number:4; // fragment number, 4 bit
	uint16_t seq_ctrl:12; // sequence number of 802.11 frame (12 bits
} mac_header_t;


// callback types
typedef void (*SNIFFER_CALLBACK) (std::list<pkt_data>& packet);



//______________________________________________________________________________
// constants
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// dimensionality
const int CHANNEL_FIRST = 1;
const int CHANNEL_LAST = 13;

// buffers length
#define MAC_LENGTH 18	// length of the MAC address (i.e. da:a1:19:0b:3e:7f + 1 byte for '\0')
#define HEADERLEN 24 	// header length for 802.11b\g (non-HT) frames (802.11n frames have 4 more bytes in the header)



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
		static std::list<pkt_data> _list_packets;

		// time-handling information
		static WiFiUDP		_ntp_udp;
		static NTPClient	_time_client;

		// timestamps
		static long _device_timestamp;
		static long _server_timestamp;

		// callback for external code
		static SNIFFER_CALLBACK _callback_sniffing_terminated;


	public:
		//----- constructors and destructors -----------------------------------
		Sniffer(int starting_channel = CHANNEL_FIRST);


		//----- methods --------------------------------------------------------
		// connect
		void start(SNIFFER_CALLBACK callback);

		// sniff packet from one channel
		void sniff();

		// getter for status
		bool is_sniffing();

		// setter for sniffing parameters
		void set_duration(int seconds);
		void set_channel(int channel);
		void set_timestamp(long server_timestamp);


	private:
		//----- internal facilities --------------------------------------------
		// callback triggered each time a packet is received by the WiFi card
		static void _callback_received_packet(void* buf, wifi_promiscuous_pkt_type_t type);

		// to compute the hash of the packet
		static uint32_t _djb2(unsigned char *data, size_t len);

};