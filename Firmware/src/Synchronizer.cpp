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
const std::string Synchronizer::topic_sniffing_start	= "/esp32/sniff/start";
const std::string Synchronizer::topic_sniffing_data		= "/esp32/sniff/data";
const std::string Synchronizer::topic_sniffing_stop		= "/esp32/sniff/stop";
const std::string Synchronizer::topic_sniffing_sync		= "/esp32/sniff/sync";


// static attribute definition
MQTT*					Synchronizer::_mqtt;
Sniffer*				Synchronizer::_sniffer;
std::mutex				Synchronizer::_m;
std::condition_variable	Synchronizer::_cv_setup;
std::condition_variable	Synchronizer::_cv_sniff;



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
	//TODO update with external class
	int led_pin = 2;
	pinMode(led_pin, OUTPUT);

	for (int i=0; i<10; i++) {
		digitalWrite(led_pin, HIGH);
		delay(100);
		digitalWrite(led_pin, LOW);
		delay(100);
	}
}


void Synchronizer::start_sniff(std::string topic, std::string data) {

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

	// start sniffing
	cout << "Start sniffing" << endl;
	signal_setup();
	delay(1000);
	signal_sniff();
}



//______________________________________________________________________________
// synchronization facilities
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

void Synchronizer::wait_setup() {

	unique_lock<mutex> ul(_m);
	_cv_setup.wait(ul);
}


void Synchronizer::signal_setup() {

	lock_guard<mutex> lg(_m);
	_cv_setup.notify_all();
}


void Synchronizer::wait_sniff() {

	unique_lock<mutex> ul(_m);
	_cv_sniff.wait(ul);
}


void Synchronizer::signal_sniff() {

	lock_guard<mutex> lg(_m);
	_cv_sniff.notify_all();
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