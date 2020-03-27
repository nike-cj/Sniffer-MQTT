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
void Synchronizer::init(MQTT* mqtt) {

	// store the MQTT client
	this->_mqtt = mqtt;

	// subscribe to 'setup' topics
	_mqtt->subscribe(topic_discovery_req, provide_mac);
	_mqtt->subscribe(topic_blink, blink);

	// subscribe to 'setup' topics
	_mqtt->subscribe(topic_sniffing_start, start_sniff);
}

void Synchronizer::request_is_sniffing() {
	// retrieve its own MAC address
	String mac_arduino = WiFi.macAddress();
	string mac = string(mac_arduino.c_str());

	// request if I should sniff
	_mqtt->publish(topic_sniffing_sync, mac);
}



//______________________________________________________________________________
// MQTT callbacks
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

void Synchronizer::provide_mac(std::string topic, std::string data) {
	// retrieve its own MAC address
	String mac_arduino = WiFi.macAddress();
	string mac = string(mac_arduino.c_str());

	// notify its own address
	_mqtt->publish(topic_discovery_res, mac);
}

void Synchronizer::blink(std::string topic, std::string data) {

	String mac_arduino = WiFi.macAddress();
	string mac = string(mac_arduino.c_str());
	if (mac != data)
		return;


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

	String mac_arduino = WiFi.macAddress();
	string mac = string(mac_arduino.c_str());
	if (mac != data.substr(0, data.find(" ")))
		return;

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