#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#define LOGFILENAME "sim.log"
static std::stringstream LOG_SS;

#define do_file_log(message) LOG_SS << std::dec << message; \
        std::ofstream(LOGFILENAME, std::ios_base::app) << LOG_SS.str() << std::endl; \
        LOG_SS.str(std::string())

#define do_stdout_log(message) LOG_SS << std::dec << message; \
        cout << LOG_SS.str(); \
        LOG_SS.str(std::string())

#ifdef LOGGING_ENABLED
    #define vlog(x) if(verbose) { do_stdout_log(x); }
  #else
    #define vlog(x) ;
#endif