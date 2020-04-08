//______________________________________________________________________________
// libraries
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

//----- class declaration ------------------------------------------------------
#include "Synchronizer.h"


//----- namespaces -------------------------------------------------------------
using namespace std;



//______________________________________________________________________________
// definition
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// static constant definition
const std::string Synchronizer::topic_discovery_req		= "/esp32/setup/discovery/request";
const std::string Synchronizer::topic_discovery_res		= "/esp32/setup/discovery/response";
const std::string Synchronizer::topic_blink				= "/esp32/setup/blink";
const std::string Synchronizer::topic_sniffing_start	= "/esp32/sniffing/start";
const std::string Synchronizer::topic_sniffing_data		= "/esp32/sniffing/data";
const std::string Synchronizer::topic_sniffing_stop		= "/esp32/sniffing/stop";
const std::string Synchronizer::topic_sniffing_sync		= "/esp32/sniffing/sync";


// static attribute definition
MQTT*			Synchronizer::_mqtt;
Sniffer*		Synchronizer::_sniffer;
std::mutex		Synchronizer::_m;
bool			Synchronizer::_is_sniffing = false;
int				Synchronizer::_server_challenge = -1;



//______________________________________________________________________________
// costructors and destructors
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
Synchronizer::Synchronizer() {

}



//______________________________________________________________________________
// methods
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

//----- activate ---------------------------------------------------------------
void Synchronizer::init(MQTT* mqtt, Sniffer* sniffer) {

	// store the MQTT client
	this->_mqtt = mqtt;

	// store the Sniffer module
	this->_sniffer = sniffer;

	// subscribe to 'setup' topics
	_mqtt->subscribe(topic_discovery_req, provide_mac);
	_mqtt->subscribe(topic_blink, blink);

	// subscribe to 'setup' topics
	_mqtt->subscribe(topic_sniffing_start, start_sniff);
}


//----- synchronization message ------------------------------------------------
void Synchronizer::ask_is_sniffing() {

	// retrieve its own MAC address
	string message = jsonify_mac();

	// notify its own address
	_mqtt->publish(topic_sniffing_sync, message);
}


//----- envelop MAC address in JSON message ------------------------------------
string Synchronizer::jsonify_mac() {
	// retrieve its own MAC address
	String mac = WiFi.macAddress();

	// serialize JSON message
	int len = JSON_OBJECT_SIZE(1) + mac.length();	// capacity for a JSON with 1 members + string duplication
	DynamicJsonDocument doc(len);

	doc["mac"] = mac;
	string message;
	serializeJson(doc, message);

	// return desired json
	return message;
}




//______________________________________________________________________________
// MQTT callbacks
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

void Synchronizer::provide_mac(std::string topic, std::string data) {

	// retrieve its own MAC address
	string message = jsonify_mac();

	// notify its own address
	_mqtt->publish(topic_discovery_res, message);
}


void Synchronizer::blink(std::string topic, std::string data) {

	// parse JSON message
	int len = JSON_OBJECT_SIZE(1) + data.length();	// capacity for a JSON with 1 members + string duplication
	DynamicJsonDocument doc(len);

	DeserializationError err = deserializeJson(doc, data);
	if (err) {
		cerr << "JSON deserialization failed with code " << err << endl;
		return;
	}

	// retrieve single fields
	String mac = doc["mac"];

	// check target
	if (!_is_my_mac(mac))
		return;

	// actually LED blinking
	Led.blink();
}


void Synchronizer::start_sniff(std::string topic, std::string data)
{
	// lock
	lock_guard<mutex> lg(_m);

	// parse JSON message
	int len = JSON_OBJECT_SIZE(4) + data.length();	// capacity for a JSON with 1 members + string duplication
	DynamicJsonDocument doc(len);

	DeserializationError err = deserializeJson(doc, data);
	if (err) {
		cerr << "JSON deserialization failed with code " << err << endl;
		return;
	}

	// retrieve single fields
	String mac				= doc["mac"];
	int channel				= doc["channel"].as<int>();
	int timestamp 			= doc["timestamp"].as<int>();
	long sniffing_seconds	= doc["sniffing_seconds"].as<long>();
	int server_challenge	= doc["server_challenge"].as<int>();

	// check target
	if (!_is_my_mac(mac))
		return;

	// store sniffing parameters (or keep default)
	if (doc.containsKey("channel"))
		_sniffer->set_channel(channel);
	if (doc.containsKey("sniffing_seconds"))
		_sniffer->set_duration(sniffing_seconds);
	if (doc.containsKey("timestamp"))
		_sniffer->set_timestamp(timestamp);

	// store synchronization parameters (or keep default)
	if (doc.containsKey("server_challenge"))
		_server_challenge = server_challenge;
	else
		_server_challenge = -1;

	// start sniffing
	cout << "Sniffing requested" << endl;
	_is_sniffing = true;
}



//______________________________________________________________________________
// synchronization facilities
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

bool Synchronizer::is_sniffing()
{
	unique_lock<mutex> lg(_m);
	return _is_sniffing;
}


void Synchronizer::terminate_sniff()
{
	// lock
	lock_guard<mutex> lg(_m);

	// retrieve its own MAC address
	String mac = WiFi.macAddress();

	// serialize JSON message
	int len = JSON_OBJECT_SIZE(3) + mac.length();	// capacity for a JSON with 3 members + string duplication
	DynamicJsonDocument doc(len);

	doc["mac"] 				= mac;
	doc["channel"] 			= _sniffer->get_channel_sniffed();
	if (_server_challenge != -1)
		doc["server_challenge"] = _server_challenge;
	string message;
	serializeJson(doc, message);

	// notify the server
	_mqtt->publish(Synchronizer::topic_sniffing_stop, message);

	// terminate sniffing
	_is_sniffing = false;
	_server_challenge = -1;
}



//______________________________________________________________________________
// internal facilities
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

bool Synchronizer::_is_my_mac(String mac_address) {
	// retrieve device MAC
	String my_mac = WiFi.macAddress();

	// compare MAC addresses
	my_mac.toLowerCase();
	mac_address.toLowerCase();
	return (my_mac == mac_address);
}