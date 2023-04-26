/* ****************************************************************
   RISC-V Instruction Set Simulator
   Computer Architecture, Semester 1, 2023

   Main program

**************************************************************** */

#include <iostream>
#include <iomanip>
#include <string>
#include <stdlib.h>
#include <unistd.h>

#include "memory.h"
#include "processor.h"
#include "commands.h"

#include "LogControl.hpp"

using namespace std;

int main(int argc, char* argv[]) {
    #ifdef LOGGING_ENABLED
        std::ofstream ofs;
        ofs.open(LOGFILENAME, std::ofstream::out | std::ofstream::trunc);
        ofs.close();
    #endif
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));

    // Values of command line options. 
    string arg;
    string testPath;
    bool verbose = false;
    bool cycle_reporting = false;
    bool stage2 = false;

    memory* main_memory;
    processor* cpu;

    unsigned long int cpu_instruction_count;

    for (int i = 1; i < argc; i++) {
	// Process the next option
	arg = string(argv[i]);
	if (arg == "-v")  // Verbose output
	    verbose = true;
	else if (arg == "-c")  // Cycle and instruction reporting enabled
	    cycle_reporting = true;
	else if (arg == "-s2")  // Stage 2 functionality enabled
	    stage2 = true;
	else if (arg == "--testHex"){
	    testPath = string(argv[i+1]);
        i++;
        continue;
    }
	else {
	    cout << argv[0] << ": Unknown option: " << arg << endl;
	}
    }

    main_memory = new memory (verbose);
    cpu = new processor (main_memory, verbose, stage2);
    if (testPath != "") {
        uint64_t start_address;
        if (main_memory->load_file(testPath, start_address))
            cpu->set_pc(start_address);
    }
    interpret_commands(main_memory, cpu, verbose);

    // Report final statistics

    cpu_instruction_count = cpu->get_instruction_count();
    cout << "Instructions executed: " << dec << cpu_instruction_count << endl;

    if (cycle_reporting) {
        // Required for postgraduate Computer Architecture course
        unsigned long int cpu_cycle_count;

        cpu_cycle_count = cpu->get_cycle_count();

        cout << "CPU cycle count: " << dec << cpu_cycle_count << endl;
    }
}
