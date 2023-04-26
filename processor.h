#ifndef PROCESSOR_H
#define PROCESSOR_H

/* ****************************************************************
   RISC-V Instruction Set Simulator
   Computer Architecture, Semester 1, 2023

   Class for processor

**************************************************************** */

#include "memory.h"
#include <string>
#include "LogControl.hpp"

using namespace std;

#define NO_BREAKPOINT ~0ULL

class processor {

 private:
  bool verbose;
  bool stage2;
  memory* mem;
  uint64_t pc;
  int64_t reg[32];
  uint64_t breakpoint;
  uint64_t instruction_count;
  uint64_t cycle_count;

  bool pc_changed;
  bool alive;

  // functionally irrelevant for stage 1:
  // uint8_t prv; // privilege level
  // uint64_t csr[4096]; // CSR registers

 public:

  // Consructor
  processor(memory* main_memory, bool verbose, bool stage2);

  // Display PC value
  void show_pc();

  // Set PC to new value
  inline void set_pc(uint64_t new_pc) {
    //do_log("Setting pc to " + to_string(new_pc));
    pc = new_pc - (new_pc % 2);
  }

  // increment the PC after an instruction. Will do nothing if the last instruction was a jump/branch
  inline void increment_pc() {
    if (!pc_changed) {
      pc += 4;
    }
  }

  // Display register value
  void show_reg(unsigned int reg_num);

  // Set register to new value
  inline void set_reg(unsigned int reg_num, uint64_t new_value) {
    if (reg_num != 0) reg[reg_num] = new_value;
    //do_log("Setting x" + to_string(reg_num) + " to " + to_string(new_value));
  }

  // Execute a number of instructions
  void execute(unsigned int num, bool breakpoint_check);

  // Execute a single instruction at the PC - a step through the program
  void step();

  // Clear breakpoint
  void clear_breakpoint();

  // Set breakpoint at an address
  void set_breakpoint(uint64_t address);

  // Show privilege level
  // Empty implementation for stage 1, required for stage 2
  void show_prv();

  // Set privilege level
  // Empty implementation for stage 1, required for stage 2
  void set_prv(uint8_t prv_num);

  // Display CSR value
  // Empty implementation for stage 1, required for stage 2
  void show_csr(unsigned int csr_num);

  // Set CSR to new value
  // Empty implementation for stage 1, required for stage 2
  void set_csr(unsigned int csr_num, uint64_t new_value);

  uint64_t get_instruction_count();

  // Used for Postgraduate assignment. Undergraduate assignment can return 0.
  uint64_t get_cycle_count();

};

#endif
