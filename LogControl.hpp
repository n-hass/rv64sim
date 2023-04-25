#include <iostream>
#ifdef BOOST_ENABLE
  #include "Logger.hpp"
  #define do_log(message) Logger::glog(message)
  #define FullLogger
#else
  #include <fstream>
  #include <sstream>
  #define LOGFILENAME "sim.log"
  static std::stringstream LOG_SS;
  #define do_log(message) LOG_SS << message; \
          std::ofstream(LOGFILENAME, std::ios_base::app) << LOG_SS.str() << std::endl; \
          LOG_SS.str(std::string())
#endif
