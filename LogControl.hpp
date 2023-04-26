#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#define LOGFILENAME "sim.log"
static std::stringstream LOG_SS;
#define do_log(message) LOG_SS << std::dec << message; \
        std::ofstream(LOGFILENAME, std::ios_base::app) << LOG_SS.str() << std::endl; \
        LOG_SS.str(std::string())

