#ifdef BOOST_ENABLE
  #include "Logger.hpp"
  #define do_log(message) Logger::glog(message)
  #define FullLogger
#else
  #include <fstream>
  #define do_log(message) std::ofstream("sim.log", std::ios_base::app) << message << std::endl;
#endif
