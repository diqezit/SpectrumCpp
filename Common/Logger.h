#ifndef SPECTRUM_CPP_LOGGER_H
#define SPECTRUM_CPP_LOGGER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines a placeholder logging system for the application.
// 
// This header provides basic logging macros that can be replaced with a more
// robust logging framework in the future without changing call sites. It
// ensures a single source of truth for logging definitions.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include <iostream>

#ifndef LOG_ERROR
#define LOG_ERROR(msg) (std::cerr << "ERROR: " << msg << std::endl)
#endif

#ifndef LOG_WARNING
#define LOG_WARNING(msg) (std::cout << "WARNING: " << msg << std::endl)
#endif

#ifndef LOG_INFO
#define LOG_INFO(msg) (std::cout << "INFO: " << msg << std::endl)
#endif

#ifndef LOG_CRITICAL
#define LOG_CRITICAL(msg) (std::cerr << "CRITICAL: " << msg << std::endl)
#endif

#endif // SPECTRUM_CPP_LOGGER_H