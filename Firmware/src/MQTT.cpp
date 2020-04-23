//______________________________________________________________________________
// libraries
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

//----- class declaration ------------------------------------------------------
#include "MQTT.h"


//----- namespaces -------------------------------------------------------------
using namespace std;



//______________________________________________________________________________
// costructors and destructors
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
MQTT::MQTT(std::string broker_address, int port) {
	
	// store values
	this->_wifi				= new WiFiClient();
	this->_broker_address	= broker_address;
	this->_broker_port		= port;

	// forge device ID
	String mac 		= WiFi.macAddress();
	_client_id		= "esp32_" + string(mac.c_str());
}


MQTT::~MQTT() {
	// release resources (RAII paradigm)
	delete _wifi;
}



//______________________________________________________________________________
// methods
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

//----- activate ---------------------------------------------------------------
void MQTT::start() {

	// check WiFi status
	if (!WiFi.isConnected()) {
		cerr << "Impossible to start MQTT client if there is no WiFi connection available" << endl;
		return;
	}

	// initialize PubSubClient
	setClient(*_wifi);
	setServer(_broker_address.c_str(), (uint16_t)_broker_port);
	setCallback(
		[&](char* topic, byte* message, unsigned int length) {
			this->_event_handler(topic, message, length);
		});

	// actually connect
	cout << "Attempting MQTT connection...";
	reconnect();
	cout << " connected" << endl;
}


//----- deactivate -------------------------------------------------------------
void MQTT::stop() {

	//TODO
}

void MQTT::reconnect() {
	// Loop until we're reconnected
	int err_counter = 0;
	while (!connected() && err_counter < 5) {
		Serial.print("Attempting MQTT connection...");
		// Attempt to connect
		if (connect(_client_id.c_str())) {
			Serial.println("connected");
		} else {
			Serial.print("failed, rc=");
			Serial.print(state());
			Serial.println(" try again in 2 seconds");
			// Wait 2 seconds before retrying
			delay(2000);
		}
		err_counter++;
	}

	if (err_counter >= 5) {
		cerr << "Unable to reach the broker. Reboot..." << endl;
		ESP.restart();
	}

	_resubscribe_all();
}


//----- send data ------------------------------------------------------------------------------------------------------
void MQTT::publish(string topic, string message, bool retained) {

	// check wifi connection
	while (!WiFi.isConnected()) {
		cout << "wifi not connected" << endl;
		WiFi.reconnect();
		delay(1000);
	}

	// core MQTT level
	while (!PubSubClient::publish(topic.c_str(), message.c_str(), retained)) {
		cerr << "Publishing failed, retry..." << endl;
		delay(2000);
	}

	// console output
	log_debug << "Publishing MQTT:"
		<< "\n\ttopic: " << topic
		<< "\n\tdata: " << message << endl;
}


//----- receive data ---------------------------------------------------------------------------------------------------
void MQTT::subscribe(string topic, MQTT_CALLBACK func_ptr, QoS qos) {

	// check quality-of-service level
	if (qos >= EXACTLY_ONCE) {
		cerr << "Underlying library does not allow subscribing at QoS EXACTLY_ONCE" << endl;
		return;
	}

	// check wifi connection
	while (!WiFi.isConnected())
		WiFi.reconnect();

	// class level
    topic_t entry;
    entry.topic     = topic;
    entry.callback  = func_ptr;
    entry.qos       = qos;
	_m.lock();
	_subscription_callbacks.insert(make_pair(topic, entry));
	_m.unlock();

	// core MQTT level
	while (!PubSubClient::subscribe(topic.c_str(), qos))
		cerr << "Subscribe failed, retry..." << endl; 
	cout << "Subscribed to '" << topic << "' topic" << endl;
}


//----- stop receiving data --------------------------------------------------------------------------------------------
void MQTT::unsubscribe(std::string topic) {
	// class level
	_m.lock();
	_subscription_callbacks.erase(topic);
	_m.unlock();

	// core MQTT level
	PubSubClient::unsubscribe(topic.c_str());
	cout << "Unsubscribed from '" << topic << "' topic" << endl;
}



//______________________________________________________________________________
// internal facilities
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

//----- event handler ----------------------------------------------------------
void MQTT::_event_handler(char* topic, byte* message, unsigned int length) {

	// retrieve relevant information
	string _topic, _message;
	try {
		_topic		= string(topic);
		_message	= string((char*)message, length);
	} catch (exception& e) {
		cerr << e.what() << endl;
		return;
	}

	// notify the event
	log_debug << "Received MQTT message:"
		<< "\n\ttopic: " << _topic
		<< "\n\tmessage: " << _message << endl;

	// invoke predisposed callback
	MQTT_CALLBACK function = _find_subscription(_topic);
	function(_topic, _message);
}


//----- find interesting MQTT subscription callback --------------------------------------------------------------------
MQTT_CALLBACK MQTT::_find_subscription(std::string topic) {
	// local variables
	lock_guard<mutex> lg(_m);

	// retrieve callback related to the topic
    auto iter = _subscription_callbacks.find(topic);
    if (iter != _subscription_callbacks.end()) {
        return iter->second.callback;
	}

	// requested topic does not exist, return default callback
	return [](string topic, string data) {
		cerr << "Triggered topic '" << topic << "', but there are no associated callbacks" << endl;
	};
}


//----- unsubscribe all topics -----------------------------------------------------------------------------------------
void MQTT::_unsubscribe_all() {
	// local variables
	lock_guard<mutex> lg(_m);

    // core MQTT level
    for (auto entry : _subscription_callbacks)
		PubSubClient::unsubscribe(entry.second.topic.c_str());

    // class level
    _subscription_callbacks.clear();
}


//----- re-subscribe all already interesting topics ----------------------------
void MQTT::_resubscribe_all() {
	// local variables
	lock_guard<mutex> lg(_m);

	// core MQTT level
	for (auto entry : _subscription_callbacks)
		PubSubClient::subscribe(entry.second.topic.c_str(), entry.second.qos);
}
