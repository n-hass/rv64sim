/* ****************************************************************
   RISC-V Instruction Set Simulator
   Computer Architecture, Semester 1, 2023

   Class members for processor

**************************************************************** */

#include <cstring>
#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <bitset>


#include "memory.h"
#include "processor.h"
#include "Instructions.h"
#include "LogControl.hpp"
#include "Bits.h"

using namespace std;


// Consructor
processor::processor (memory* main_memory, bool verbose, bool stage2) {
  this->verbose = verbose;
  this->stage2 = stage2;
  this->mem = main_memory;
  this->pc = 0;
  this->breakpoint = 0;
  memset(reg, 0, sizeof(uint64_t)*32);
}

// Display PC value
void processor::show_pc() {
  cout << setw(16) << setfill('0') << hex << pc << endl;
}

// Set PC
void processor::set_pc(uint64_t new_pc) {
  pc = new_pc;
}

// Prints a register value
void processor::show_reg(unsigned int reg_num) {
  cout << setw(16) << setfill('0') << hex << reg[reg_num] << endl;
}

// Sets a register value
void processor::set_reg(unsigned int reg_num, uint64_t new_value) {
  reg[reg_num] = new_value;
}

// Execute 'num' instructions
void processor::execute(unsigned int num, bool breakpoint_check) {
  bool exit = false;
  do {
    if (breakpoint_check && pc == breakpoint) {
      cout << "Breakpoint reached at ";
      show_pc();
      clear_breakpoint();
      break;
    }

    if (pc % 4 != 0) {
      cout << "Error: misaligned PC" << endl;
      continue;
    }

    step();

    instruction_count++;
    set_pc(pc+4);
    num--;

  } while (num > 0);

  if (exit) {
    std::cout << "Finished execution at ";
    show_pc();
  }
}

void processor::step() {
  uint64_t inst = mem->read_doubleword(pc);
  uint8_t opcode = EXTRACT_OPCODE_FROM_INST(inst);

  // storage //
  uint8_t funct3;
  uint8_t funct7;
  uint16_t funct7_3;
  uint8_t rd;
  uint8_t rs1;
  uint8_t rs2;
  int64_t imm;

  uint64_t address;

  uint64_t dwa; // doubleword A
  // * //
  
  switch (rv64::opcode(opcode)) {
    case rv64::opcode::load_op:
      imm = EXTRACT_IMM12_FROM_INST(inst); // upper 12 bits sign extended to a doubleword
      rs1 = EXTRACT_RS1_FROM_INST(inst);
      rd = EXTRACT_RD_FROM_INST(inst);
      address = reg[rs1] + imm;

      do_log("load_op: address = " << address);
      dwa = mem->read_doubleword(address);
      
      funct3 = EXTRACT_FUNCT3_FROM_INST(inst);
      switch (rv64::load_funct(funct3)){
        
        case rv64::load_funct::lb_f:
          reg[rd] = int64_t(uint64_t(dwa) << 56) >> 56;
        break;

        case rv64::load_funct::lh_f:
          if (address % 2 != 0) {
            // misaligned 
            do_log("Error: misaligned address when doing: lh x"<< rd << ", " << imm << "(" << rs1 << ")");
            return;
          }
          reg[rd] = int64_t(uint64_t(dwa) << 48) >> 48;
        break;

        case rv64::load_funct::lw_f:
          if (address % 4 != 0) {
            // misaligned 
            do_log("Error: misaligned address when doing: lw x"<< rd << ", " << imm << "(" << rs1 << ")");
            return;
          }
          reg[rd] = int64_t(uint64_t(dwa) << 32) >> 32;
        break;

        case rv64::load_funct::ld_f:
          if (address % 8 != 0) {
            // misaligned 
            do_log("Error: misaligned address when doing: ld x"<< rd << ", " << imm << "(" << rs1 << ")");
            return;
          }
          reg[rd] = dwa;
        break;

        case rv64::load_funct::lbu_f:
          reg[rd] = dwa & 0xFF;
        break;

        case rv64::load_funct::lhu_f:
          if (address % 2 != 0) {
            // misaligned 
            do_log("Error: misaligned address when doing: lhu x"<< rd << ", " << imm << "(" << rs1 << ")");
            return;
          }
          reg[rd] = dwa & 0xFFFF;
        break;

        case rv64::load_funct::lwu_f:
          if (address % 4 != 0) {
            // misaligned 
            do_log("Error: misaligned address when doing: lwu x"<< rd << ", " << imm << "(" << rs1 << ")");
            return;
          }
          reg[rd] = dwa & 0xFFFFFFFF;
        break;
      }

    break;

    case rv64::opcode::fen_op:
      // do nothing in stage 1
      do_log("fence operations decoded but not implemented");
      // stage 2:
      funct3 = EXTRACT_FUNCT3_FROM_INST(inst);
      switch(rv64::fen_funct(funct3)) {
        case rv64::fen_funct::fence_f:
          // do nothing in stage 2
          do_log("fence operation decoded but not implemented");
        break;
        case rv64::fen_funct::fence_i_f:
          // do nothing in stage 2
          do_log("fence.i operation decoded but not implemented");
        break;
      }
    break;

    case rv64::opcode::imm_op:
      rs1 = EXTRACT_RS1_FROM_INST(inst);
      rd = EXTRACT_RD_FROM_INST(inst);
      funct3 = EXTRACT_FUNCT3_FROM_INST(inst);
      imm = EXTRACT_IMM12_FROM_INST(inst); // upper 12 bits sign extended to a doubleword

      switch (rv64::imm_funct3(funct3)) {
        case rv64::imm_funct3::addi_f3:
          reg[rd] = reg[rs1] + imm;
        break;

        case rv64::imm_funct3::slti_f3:
          reg[rd] = (static_cast<int64_t>(reg[rs1]) < imm) ? 1 : 0;
        break;

        case rv64::imm_funct3::sltiu_f3:
          reg[rd] = (uint64_t(reg[rs1]) < uint64_t(imm)) ? 1 : 0;
        break;

        case rv64::imm_funct3::xori_f3:
          reg[rd] = reg[rs1] ^ imm;
        break;

        case rv64::imm_funct3::ori_f3: 
          reg[rd] = reg[rs1] | imm;
        break;
        
        case rv64::imm_funct3::andi_f3:
          reg[rd] = reg[rs1] & imm;
        break;

        case rv64::imm_funct3::srli_f3: // srai shares the same funct3 as srli: 101
        case rv64::imm_funct3::slli_f3:
          // NOTE: rs2 is shamt: bits 24-20
          // NOTE: the 'definition' of rs2 changes for the duration of this case (and its nested switch)
          // NOTE: this is to reuse the already declared variable rs2 prior to the switches
          rs2 = EXTRACT_RS2_FROM_INST(inst);
          funct7_3 = EXTRACT_FUNCT7_AND_3_FROM_INST(inst);
          
          switch (rv64::imm_funct73(funct7_3)) {
            case rv64::imm_funct73::slli_f73:
              reg[rd] = reg[rs1] << rs2; // rs2 is actually shamt, not a register
            break;

            case rv64::imm_funct73::srli_f73:
              reg[rd] = reg[rs1] >> rs2; // rs2 is actually shamt, not a register
            break;

            case rv64::imm_funct73::srai_f73:
              reg[rd] = static_cast<int64_t>(reg[rs1]) >> rs2; // rs2 is actually shamt, not a register
            break;
          }

        break;
      }
    break;

    case rv64::opcode::reg_op: 
      rd = EXTRACT_RD_FROM_INST(inst);
      funct7_3 = EXTRACT_FUNCT7_AND_3_FROM_INST(inst);
      rs1 = EXTRACT_RS1_FROM_INST(inst);
      rs2 = EXTRACT_RS2_FROM_INST(inst);

      switch (rv64::reg_funct(funct7_3)) {
        case rv64::reg_funct::add_f:
          reg[rd] = reg[rs1] + reg[rs2];
        break;

        case rv64::reg_funct::sub_f:
          reg[rd] = reg[rs1] - reg[rs2];
        break;

        case rv64::reg_funct::sll_f:
          // reg[rd] = reg[rs1] << EXTRACT_BITS(reg[rs2], 4, 0);
          reg[rd] = reg[rs1] << reg[rs2];
        break;

        case rv64::reg_funct::slt_f:
          reg[rd] = (static_cast<int64_t>(reg[rs1]) < static_cast<int64_t>(reg[rs2])) ? 1 : 0;
        break;

        case rv64::reg_funct::sltu_f:
          reg[rd] = (static_cast<uint64_t>(reg[rs1]) < static_cast<uint64_t>(reg[rs2])) ? 1 : 0;
        break;

        case rv64::reg_funct::xor_f:
          reg[rd] = reg[rs1] ^ reg[rs2];
        break;

        case rv64::reg_funct::srl_f:
          // reg[rd] = reg[rs1] >> EXTRACT_BITS(reg[rs2], 4, 0);
          reg[rd] = reg[rs1] >> reg[rs2];
        break;

        case rv64::reg_funct::sra_f:
          // reg[rd] = static_cast<int64_t>(reg[rs1]) >> EXTRACT_BITS(reg[rs2], 4, 0);
          reg[rd] = static_cast<int64_t>(reg[rs1]) >> reg[rs2];
        break;

        case rv64::reg_funct::or_f:
          reg[rd] = reg[rs1] | reg[rs2];
        break;

        case rv64::reg_funct::and_f:
          reg[rd] = reg[rs1] & reg[rs2];
        break;
      }

    break;
    

    case rv64::opcode::sys_op:
      funct3 = (inst >> 12) & 0x7;
      switch (rv64::sys_funct(funct3)) {
        case rv64::sys_funct::ecall_f:
          cout << "ecall is not implemented as part of stage 1\n";
        break;
        case rv64::sys_funct::ebreak_f:
          cout << "ebreak is not implemented as part of stage 1\n";
        break;
      }
    break;

    default:
      cout << "Error: invalid instruction" << endl;
    break;
  }
  
}