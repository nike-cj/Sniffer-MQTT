#pragma once

//______________________________________________________________________________
// libraries
//‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾

//----- C++ libraries ----------------------------------------------------------
#include <sstream>

//----- ESP32 libraries --------------------------------------------------------
#include "freertos/FreeRTOS.h"
#include "esp_log.h"



//______________________________________________________________________________
// internal functions
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
inline std::string method_name(const std::string& pretty_function)
{
    size_t colons	= pretty_function.find("::");
    size_t begin	= pretty_function.substr(0, colons).rfind(" ") + 1;
    size_t len		= pretty_function.rfind("(") - begin;

    return pretty_function.substr(begin, len) + "()";
}


//______________________________________________________________________________
// internal class
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

// temporary stream object that actually prints when deallocated (RAII paradigm)
class log_stream: public std::ostringstream  {
public:
	//----- attributes -----
	std::string		_method;
	esp_log_level_t	_level;

	//----- constructor and destructor -----
    log_stream(std::string method_name, esp_log_level_t level):
	 	_method(method_name), _level(level) {}

    ~log_stream() {
		// discard final '\n'
		std::string output = this->str();
		if (output.substr(output.length()-1, output.length()) == "\n")
			output = output.substr(0, output.length()-1);

		// print according to logging level
		ESP_LOG_LEVEL(_level, _method.c_str(), "%s", output.c_str());
    }
};


//______________________________________________________________________________
// public macro
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
#define __METHOD_NAME__ method_name(__PRETTY_FUNCTION__)

#define log_error	log_stream(__FILE__, ESP_LOG_ERROR)
#define log_warning	log_stream(__FILE__, ESP_LOG_WARN)
#define log_info	log_stream(__FILE__, ESP_LOG_INFO)
#define log_debug	log_stream(__FILE__, ESP_LOG_DEBUG)
#define log_verbose	log_stream(__FILE__, ESP_LOG_VERBOSE)