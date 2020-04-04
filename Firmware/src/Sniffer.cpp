//______________________________________________________________________________
// libraries
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

//----- class declaration ------------------------------------------------------
#include "Sniffer.h"


//----- namespaces -------------------------------------------------------------
using namespace std;



//______________________________________________________________________________
// definition
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// static attribute definition
SNIFFER_CALLBACK	Sniffer::_callback_sniffing_terminated;
std::list<pkt_data>	Sniffer::_list_packets;
WiFiUDP				Sniffer::_ntp_udp;
long 				Sniffer::_device_timestamp;
long				Sniffer::_server_timestamp;
NTPClient			Sniffer::_time_client(Sniffer::_ntp_udp, "europe.pool.ntp.org");



//______________________________________________________________________________
// costructors and destructors
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
Sniffer::Sniffer(int starting_channel) {
	// sniffing parameter
	this->_seconds 			= 60;
	this->_current_channel	= starting_channel;
}



//______________________________________________________________________________
// methods
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

//----- activate ---------------------------------------------------------------
void Sniffer::start(SNIFFER_CALLBACK callback) {

	// store callback onPacketReceived
	Sniffer::_callback_sniffing_terminated = callback;

	// start the NTP connection
	_time_client.begin();
	_time_client.setTimeOffset(3600); // GMT +1 = 3600 seconds
}


void Sniffer::sniff() {

	// retrieve timestamp (from NTP)
	_device_timestamp = millis();
	_time_client.forceUpdate();

	cout << "Start sniffing probe requests on channel " << _current_channel << endl;

	ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(_callback_received_packet)); // register the callback function
	
	wifi_promiscuous_filter_t filter; // packet sniffing filters and parameters
	filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT; // filter management packets (probe requests are among them)
	ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&filter)); // set the filter
	ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true)); // set the wifi interface into promiscuous mode
	ESP_ERROR_CHECK(esp_wifi_set_channel(_current_channel, WIFI_SECOND_CHAN_NONE)); // set channel to be scanned, do not use a second channel
	_is_sniffing = true;
	
	delay(_seconds * 1000); // wait 60 seconds...in the meantime the callback method will be triggered
	cout << endl << "Sniffing terminated, listened " << _list_packets.size() << " BEACON packets" << endl;
	
	ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false)); // disable promiscuous mode
	_is_sniffing = false;
	bool is_promiscous = true;
	while (is_promiscous)
		esp_wifi_get_promiscuous(&is_promiscous);

	// execute external code
	Sniffer::_callback_sniffing_terminated(_list_packets);
	_list_packets.clear();

	// increase the channel
	_current_channel++;
	if (_current_channel > CHANNEL_LAST)
		_current_channel = CHANNEL_FIRST;
}


// setter for sniffing parameters
void Sniffer::set_duration(int seconds) {
	this->_seconds			= seconds;
}

void Sniffer::set_channel(int channel) {
	this->_current_channel	= channel;
}

void Sniffer::set_timestamp(long server_timestamp) {
	this->_server_timestamp = server_timestamp;
}



//______________________________________________________________________________
// internal facilities
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

//----- event handler ----------------------------------------------------------
void Sniffer::_callback_received_packet(void* buf, wifi_promiscuous_pkt_type_t type) {

	//TODO fix timestamping
	ulong millis_from_sniffing_begin = (millis() - _device_timestamp) % 1000;
	long t = _time_client.getEpochTime();
	unsigned long timestamp = _time_client.getEpochTime();// * 1000;// + millis_from_sniffing_begin;

	// cout << "received packet at timestamp " << _time_client.getFormattedTime().c_str()
	// 	<< " (" << timestamp << ") where millis "
	// 	<< millis() << " - " << _device_timestamp << " = " << millis_from_sniffing_begin << endl;
	// Sniffer::_list_packets.push_back(packet);
	// return;


	
	uint64_t epoch = 0; // callback arrival time
	uint8_t pkt_type = 0; // packet type
	uint8_t pkt_subtype = 0; // packet subtype
	uint16_t seq_num = 0; // packet sequence number
	int8_t rssi = 0; // signal strength indicator
	uint16_t pkt_totlen = 0; // total length of packet (802.11 header + payload)
	uint16_t payload_len = 0; // length of payload
	uint32_t hash = 0; // digest of the packet
	char tmp_mac[MAC_LENGTH]; // temporary buffer to store source MAC address
	string source_mac; // packet's source mac address
	struct timeval current_time;
	uint8_t payload_offset = HEADERLEN; // this is the offset of the payload of the probe request within the packet (MAC header included)
	mac_header_t *header = nullptr; // pointer to the first byte of the captured packet, mac_header_t is a struct that mimics the header
	wifi_promiscuous_pkt_t *packet_captured = nullptr; // pointer to the packet captured by the ESP32 and passed to the callback
	char *packet_ptr = nullptr; // pointer to the first byte of the captured packet
	unique_ptr<char[]> payload_ptr = nullptr; // this is a smart pointer that points to the memory where we dump the payload of the probe request for further analysis
	
	// when callback is called get system time as soon as possible
	if(gettimeofday(&current_time, nullptr) != 0){
		return; // no reason to go on...the packet must have the right timestamp otherwise it is useless
	} else {
		epoch = ((uint64_t)current_time.tv_sec*1000)+((uint64_t)current_time.tv_usec/1000); // convert system time to epoch time
	}
	
	/* wifi_promiscuous_pkt_t is a type defined by Espressif that includes radio metadata header and a pointer to the packet itself 
	   including the MAC header. buf is the parameter, received by the callback, that points to the wifi_promiscuous_pkt_t. */
	packet_captured = (wifi_promiscuous_pkt_t *)buf;
	if(packet_captured == nullptr){
		return; // if pointer still null no reason to go on
	}
	
	/* wifi_promiscuous_pkt_t is a struct that includes some metadata so we store them into local variables. moreover it includes
	   also a pointer to the captured packet, we use it to store the informations about the mac header. */
	rssi = packet_captured->rx_ctrl.rssi; // this is the signal strength indicator
	pkt_totlen = packet_captured->rx_ctrl.sig_len - 4; // length of packet excluding FCS (which is 4 byte, that's why we remove them from sig_len)
	header = (mac_header_t *)packet_captured->payload; // retrieve the header (not all of it if packet was sent on 802.11n)
	if(header == nullptr){
		return;
	}
	
	/* mac_header_t allows to automatically parse the content of the header */
	pkt_type = header->type; // type of the packet (should be 00 -> management)
	pkt_subtype = header->subtype; // subtype of the packet (should be 0100 -> probe request)
	if(pkt_type!=0 || pkt_subtype!=4){
		return; // return if not a probe request packet
	}
	
	seq_num = header->seq_ctrl; // retrieve sequence number
	memset(tmp_mac, '0', MAC_LENGTH); // retrieve and convert source mac address
	int res = sprintf(tmp_mac, "%02x:%02x:%02x:%02x:%02x:%02x", header->source_addr[0], header->source_addr[1], header->source_addr[2], header->source_addr[3], header->source_addr[4], header->source_addr[5]);
	if(res != MAC_LENGTH-1){ // handle failure of sprintf setting the MAC to zero
		return; // no reason to go on if we can't label the packet with the sourca MAC address
	}
	try{
		source_mac.assign(tmp_mac);
	} catch(...) {
		return; // abort callback in case of any exception...we simply ignore this probe request
	}
	
	if(header->flags & 0x01){ // check if last bit of frame control is 1
		#ifdef VERBOSE
			ESP_LOGI(TAG, "Detected MAC frame with HT control field. Flags: %02x", header->flags);
		#endif
		payload_offset += 4; // HT control is 4 byte long so MAC header is 28 byte long and not 24
	}
	
	packet_ptr = (char*)packet_captured->payload; // this is a pointer to the first byte of the packet (MAC header and FCS included)
	payload_len = pkt_totlen - payload_offset; // this is the actual quantity of bytes we have in the payload of the probe request	
	
	try{
		payload_ptr = unique_ptr<char[]>(new char[payload_len]());
	}
	catch(...){
		payload_ptr = nullptr;
	}
	
	if(payload_ptr == nullptr){
		ESP_LOGE(TAG, "Callback failure...probably not enough heap memory.");
		return; // we simply give up the current callback
	} else {
		memcpy(payload_ptr.get(), packet_ptr+payload_offset, payload_len); // brutally dump the packet into RAM excluding header and FCS
		hash = _djb2((unsigned char*)packet_ptr, pkt_totlen); // compute the hash of the packet
	}
	
	pkt_data pkt = {
		source_mac,
		rssi,
		t,
		seq_num,
		hash,
		move(payload_ptr),
		payload_len
	}; // create packet object 
	try{
		_list_packets.push_back(move(pkt)); // insert packet into list of packets to be sent to the computer
	} catch(...){
		return; // if insertion into list fails no problem...we return from the callback without memory leakage because the shared pointer will be destroyed, as the pkt_data object
	}
	#ifdef VERBOSE
		ESP_LOGI(TAG, "Source: %s | Timestamp: %llu | RSSI: %d | Seq.Numb: %u | Hash: %u", source_mac.c_str(), epoch, rssi, seq_num, hash);
	#endif

	// progress bar
	cout << "." << flush;
}


//----- djb2 hash function (xor variant) to compute the digest of the packet ---
uint32_t Sniffer::_djb2(unsigned char *data, size_t len){
	uint32_t hash = 5381;
	for(int i=0; i<len; i++){
		hash = hash * 33 ^ data[i];
	}
	return hash;
}




string pkt_data::jsonify() {

	// create the object
	int len = JSON_OBJECT_SIZE(7) + mac_address.length() + 5*probe_req_payload_len;	// capacity for a JSON with 7 members + string duplication
	DynamicJsonDocument doc(len);

	// insert single fields
	doc["mac"] = mac_address;
	doc["rssi"] = rssi;
	doc["timestamp"] = timestamp;
	doc["sequence_number"] = seq_number;
	doc["hash"] = hash;
	doc["probe_req_payload"] = bytes2string(probe_req_payload.get(), probe_req_payload_len);
	doc["probe_req_payload_len"] = probe_req_payload_len;

	// serialize JSON
	string message;
	serializeJson(doc, message);

	if (message.length() >= 1024)
		cerr << "message too heavy" << endl;

	// return desired json
	return message;
}


string pkt_data::bytes2string(char* data, int len) {
	// local variable
	stringstream ss;

	// encode each 1 byte as 2 nibbles
	ss << hex;
	for (int i=0; i<len; i++) {
		ss << "0x" << setw(2) << setfill('0') << (int)data[i];
		if (i != len - 1)
			ss << " ";
	}

	// return string of hexadecimals
	return ss.str();
}