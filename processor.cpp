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

  uint64_t dwa; // doubleword A, a buffer for reading doublewords
  // * //
  
  switch (rv64::opcode(opcode)) {
    case rv64::opcode::nop_op:
      do_log("NOP at " << pc);
    break;

    case rv64::opcode::lui_op:
      rd = EXTRACT_RD_FROM_INST(inst);
      reg[rd] = static_cast<int64_t>(static_cast<int32_t>(inst & 0xFFFFF000));
    break;

    case rv64::opcode::auipc_op:
      rd = EXTRACT_RD_FROM_INST(inst);
      reg[rd] = static_cast<int64_t>(pc) + static_cast<int64_t>( static_cast<int32_t>(inst & 0xFFFFF000) );
      // reg[rd] = static_cast<int64_t>(pc) + static_cast<int64_t>( static_cast<int32_t>(inst) >> 12 ); // cast imm[31:12] to a signed 64-bit value
      // reg[rd] = static_cast<int64_t>(pc) + (static_cast<int64_t>((static_cast<uint64_t>(inst) & 0xFFFFF000) << 32) >> 32); // upper 20 bits sign extended to a doubleword
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

      switch (rv64::reg_funct73(funct7_3)) {
        case rv64::reg_funct73::add_f:
          reg[rd] = reg[rs1] + reg[rs2];
        break;

        case rv64::reg_funct73::sub_f:
          reg[rd] = reg[rs1] - reg[rs2];
        break;

        case rv64::reg_funct73::sll_f:
          // reg[rd] = reg[rs1] << EXTRACT_BITS(reg[rs2], 4, 0);
          reg[rd] = reg[rs1] << reg[rs2];
        break;

        case rv64::reg_funct73::slt_f:
          reg[rd] = (static_cast<int64_t>(reg[rs1]) < static_cast<int64_t>(reg[rs2])) ? 1 : 0;
        break;

        case rv64::reg_funct73::sltu_f:
          reg[rd] = (static_cast<uint64_t>(reg[rs1]) < static_cast<uint64_t>(reg[rs2])) ? 1 : 0;
        break;

        case rv64::reg_funct73::xor_f:
          reg[rd] = reg[rs1] ^ reg[rs2];
        break;

        case rv64::reg_funct73::srl_f:
          // reg[rd] = reg[rs1] >> EXTRACT_BITS(reg[rs2], 4, 0);
          reg[rd] = reg[rs1] >> reg[rs2];
        break;

        case rv64::reg_funct73::sra_f:
          // reg[rd] = static_cast<int64_t>(reg[rs1]) >> EXTRACT_BITS(reg[rs2], 4, 0);
          reg[rd] = static_cast<int64_t>(reg[rs1]) >> reg[rs2];
        break;

        case rv64::reg_funct73::or_f:
          reg[rd] = reg[rs1] | reg[rs2];
        break;

        case rv64::reg_funct73::and_f:
          reg[rd] = reg[rs1] & reg[rs2];
        break;
      }

    break;

    case rv64::opcode::fen_op:
      // do nothing in stage 1
      do_log("fence operations decoded but not implemented");
      // stage 2:
      funct3 = EXTRACT_FUNCT3_FROM_INST(inst);
      switch(rv64::fen_funct3(funct3)) {
        case rv64::fen_funct3::fence_f:
          // do nothing in stage 2
          do_log("fence operation decoded but not implemented");
        break;
        case rv64::fen_funct3::fence_i_f:
          // do nothing in stage 2
          do_log("fence.i operation decoded but not implemented");
        break;
      }
    break;

    case rv64::opcode::sys_op:
      rs2 = EXTRACT_RS2_FROM_INST(inst);
      switch (rv64::sys_rs2(funct3)) { // these system instructions are weird. they use rs2 to differentiate between them
        case rv64::sys_rs2::ecall_f:
          cout << "ecall is not implemented as part of stage 1\n";
        break;
        case rv64::sys_rs2::ebreak_f:
          cout << "ebreak is not implemented as part of stage 1\n";
        break;
      }
    break;

    case rv64::opcode::load_op:
      imm = EXTRACT_IMM12_FROM_INST(inst); // upper 12 bits sign extended to a doubleword
      rs1 = EXTRACT_RS1_FROM_INST(inst);
      rd = EXTRACT_RD_FROM_INST(inst);
      address = reg[rs1] + imm;

      do_log("load_op: address = " << address);
      dwa = mem->read_doubleword(address);
      
      funct3 = EXTRACT_FUNCT3_FROM_INST(inst);
      switch (rv64::load_funct3(funct3)){
        
        case rv64::load_funct3::lb_f:
          reg[rd] = int64_t(uint64_t(dwa) << 56) >> 56;
        break;

        case rv64::load_funct3::lh_f:
          if (address % 2 != 0) {
            // misaligned 
            do_log("Error: misaligned address when doing: lh x"<< rd << ", " << imm << "(" << rs1 << ")");
            return;
          }
          reg[rd] = int64_t(uint64_t(dwa) << 48) >> 48;
        break;

        case rv64::load_funct3::lw_f:
          if (address % 4 != 0) {
            // misaligned 
            do_log("Error: misaligned address when doing: lw x"<< rd << ", " << imm << "(" << rs1 << ")");
            return;
          }
          reg[rd] = int64_t(uint64_t(dwa) << 32) >> 32;
        break;

        case rv64::load_funct3::ld_f:
          if (address % 8 != 0) {
            // misaligned 
            do_log("Error: misaligned address when doing: ld x"<< rd << ", " << imm << "(" << rs1 << ")");
            return;
          }
          reg[rd] = dwa;
        break;

        case rv64::load_funct3::lbu_f:
          reg[rd] = dwa & 0xFF;
        break;

        case rv64::load_funct3::lhu_f:
          if (address % 2 != 0) {
            // misaligned 
            do_log("Error: misaligned address when doing: lhu x"<< rd << ", " << imm << "(" << rs1 << ")");
            return;
          }
          reg[rd] = dwa & 0xFFFF;
        break;

        case rv64::load_funct3::lwu_f:
          if (address % 4 != 0) {
            // misaligned 
            do_log("Error: misaligned address when doing: lwu x"<< rd << ", " << imm << "(" << rs1 << ")");
            return;
          }
          reg[rd] = dwa & 0xFFFFFFFF;
        break;
      }

    break;

    case rv64::opcode::store_op:
      imm = USE_BITMASK(inst, 0xFE00000, 20) | USE_BITMASK(inst, 0x00000F80, 7); // 1111 1110 0000 0000 0000 1111 1000 0000
      rs2 = EXTRACT_RS2_FROM_INST(inst);
      rs1 = EXTRACT_RS1_FROM_INST(inst);
      funct3 = EXTRACT_FUNCT3_FROM_INST(inst);

      address = reg[rs1] + imm;
      
      switch (rv64::store_funct3(funct3)) {

        case (rv64::store_funct3::sb_f):
          mem->write_doubleword(address, reg[rs2], 0xFF);
        break;

        case (rv64::store_funct3::sh_f):
          if (address % 2 != 0) {
            // misaligned 
            do_log("Error: misaligned address when doing: sh x"<< rs2 << ", " << imm << "(" << rs1 << ")");
            return;
          }
          mem->write_doubleword(address, reg[rs2], 0xFFFF);
        break;

        case (rv64::store_funct3::sw_f):
          if (address % 4 != 0) {
            // misaligned 
            do_log("Error: misaligned address when doing: sw x"<< rs2 << ", " << imm << "(" << rs1 << ")");
            return;
          }
          mem->write_doubleword(address, reg[rs2], 0xFFFFFFFF);
        break;

        case (rv64::store_funct3::sd_f):
          if (address % 8 != 0) {
            // misaligned 
            do_log("Error: misaligned address when doing: sd x"<< rs2 << ", " << imm << "(" << rs1 << ")");
            return;
          }
          mem->write_doubleword(address, reg[rs2], 0xFFFFFFFFFFFFFFFF);
        break;
      }

    break;

    case rv64::opcode::jal_op:
      // imm = USE_BITMASK(inst, 0x80000000, 11) | USE_BITMASK(inst, 0x7FE00000, 20) | USE_BITMASK(inst, 0x00100000, 9) | USE_BITMASK(inst, 0x000FF000, 8);
      imm = (int32_t)(((((inst) >> 21) & ((1 << 10) - 1)) << 1) |
                     ((((inst) >> 20) & ((1 << 1) - 1)) << 11) |
                     ((((inst) >> 12) & ((1 << 8) - 1)) << 12) |
                     ((-(((inst) >> 31) & 1)) << 20));
      rd = EXTRACT_RD_FROM_INST(inst);
      reg[rd] = pc + 4; // TODO: check this incrementation against the execute function
      pc = pc + imm;
    break;

    case rv64::opcode::jalr_op:
      imm = EXTRACT_IMM12_FROM_INST(inst);
      rs1 = EXTRACT_RS1_FROM_INST(inst);
      rd = EXTRACT_RD_FROM_INST(inst);
      reg[rd] = pc + 4; // TODO: check this incrementation against the execute function
      pc = (reg[rs1] + imm) & 0xFFFFFFFE;
    break;

    case rv64::opcode::branch_op:
      // imm = USE_BITMASK(inst, 0x80000000, 11) | USE_BITMASK(inst, 0x7E000000, 5) | USE_BITMASK(inst, 0x00100000, 4) | USE_BITMASK(inst, 0x00000F00, 1);
      
      // imm = (int64_t)(uint64_t(((inst & 0xFE000000) >> 25) >> 6) << 63 |
      //                 uint64_t(((inst & 0x00000F80) >> 7) % 2) << 62 |
      //                 uint64_t(((inst & 0xFE000000) >> 25) & 0x3F) << 56 | 
      //                 uint64_t(((inst & 0x00000F80) >> 7) >> 1) << 52) >> 51;
      
      // imm = EXTRACT_RD_FROM_INST(inst);
      // if (imm & 1) {
      //   imm = imm - 1;
      //   imm = imm | (1<<11);
      // }
      // imm = imm | (EXTRACT_FUNCT7_FROM_INST(inst) << 5);
      // imm = (int64_t)((int32_t)(imm & BITMASK(31, 20))) >> 20;

      // imm = 0;
      // rd = EXTRACT_RD_FROM_INST(inst);
      // funct7 = EXTRACT_FUNCT7_FROM_INST(inst);

      // imm = ( (funct7 & 0x40) << 6 ) |
      //       ( (rd & 0x1) << 11 ) |
      //       ( (funct7 & 0x3F) << 5 ) |
      //       ( rd & 0x1E ) ;
      // if (imm & 0x800) { // sign extend where necessary
      //   imm = imm | 0xFFFFF000;
      // }

      imm = int64_t(
                    uint64_t(USE_BITMASK(inst, 0xFE000000, 25) >> 6) << 63 | 
                    uint64_t(USE_BITMASK(inst, 0x00000F80, 7) % 2) << 62 |
                    uint64_t(USE_BITMASK(inst, 0xFE000000, 25) & 0x3F) << 56 | 
                    uint64_t(USE_BITMASK(inst, 0x00000F80, 7) >> 1) << 52
                   ) >> 51;
      imm -= 4; // because pc will be incremented by 4 in processor::execute()

      rs2 = EXTRACT_RS2_FROM_INST(inst);
      rs1 = EXTRACT_RS1_FROM_INST(inst);
      funct3 = EXTRACT_FUNCT3_FROM_INST(inst);

      switch (rv64::branch_funct3(funct3)) {
        case rv64::branch_funct3::beq_f:
          if (reg[rs1] == reg[rs2]) {
            pc = pc + imm;
          }
        break;

        case rv64::branch_funct3::bne_f:
          if (reg[rs1] != reg[rs2]) {
            pc = pc + imm;
          }
        break;

        case rv64::branch_funct3::blt_f:
          if (static_cast<int64_t>(reg[rs1]) < static_cast<int64_t>(reg[rs2])) {
            pc = pc + imm;
          }
        break;

        case rv64::branch_funct3::bge_f:
          if (static_cast<int64_t>(reg[rs1]) >= static_cast<int64_t>(reg[rs2])) {
            pc = pc + imm;
          }
        break;

        case rv64::branch_funct3::bltu_f:
          if (reg[rs1] < reg[rs2]) {
            pc = pc + imm;
          }
        break;

        case rv64::branch_funct3::bgeu_f:
          if (reg[rs1] >= reg[rs2]) {
            pc = pc + imm;
          }
        break;
      }
    break;

    case rv64::opcode::imm64_op:
      rs1 = EXTRACT_RS1_FROM_INST(inst);
      rd = EXTRACT_RD_FROM_INST(inst);
      funct3 = EXTRACT_FUNCT3_FROM_INST(inst);

      
      switch (rv64::imm64_funct3(funct3)) {
        case rv64::imm64_funct3::addiw_f3:
          imm = EXTRACT_IMM12_FROM_INST(inst);
          reg[rd] = int64_t(uint64_t(int64_t(reg[rs1]) + imm) << 32) >> 32;
        break;

        case rv64::imm64_funct3::slliw_f3:
          // shamt is rs2
          rs2 = EXTRACT_RS2_FROM_INST(inst);
          reg[rd] = (int64_t( uint32_t(reg[rs1]) << rs2 ) << 32 ) >> 32;
        break;

        case rv64::imm64_funct3::srliw_f3: // sraiw has the same funct3
          funct7_3 = EXTRACT_FUNCT7_AND_3_FROM_INST(inst);
          rs2 = EXTRACT_RS2_FROM_INST(inst); // this is the shamt

          switch (rv64::imm64_funct73(funct7_3)){
            case rv64::imm64_funct73::srliw_f73:
              reg[rd] = (int64_t( uint32_t(reg[rs1]) >> rs2 ) << 32 ) >> 32;
            break;

            case rv64::imm64_funct73::sraiw_f73:
              reg[rd] = (int64_t( int32_t(reg[rs1]) >> rs2 ) << 32 ) >> 32;
            break;
          }

        break;

      }

    break;
    
    case rv64::opcode::reg64_op:

    break;


  }
  
}