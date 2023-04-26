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
#include <string>


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
  this->breakpoint = NO_BREAKPOINT;
  memset(reg, 0, sizeof(uint64_t)*32);
}

// Display PC value
void processor::show_pc() {
  cout << setw(16) << setfill('0') << hex << pc << endl;
}

// Prints a register value
void processor::show_reg(unsigned int reg_num) {
  cout << setw(16) << setfill('0') << hex << reg[reg_num] << endl;
}

// Execute 'num' instructions
void processor::execute(unsigned int num, bool breakpoint_check) {
  this->alive = true;

  while (num > 0 && alive){
    if (breakpoint_check && (pc == breakpoint)) {
      cout << "Breakpoint reached at ";
      show_pc();
      clear_breakpoint();
      return;
    }

    if (pc % 4 != 0) {
      cout << "Error: misaligned pc" << endl;
      return;
    }

    pc_changed = false;
    step();
    
    instruction_count++;
    increment_pc();
    num--;
  };
  pc_changed = false;

  // this only runs when the program has finished executing
  if (pc == breakpoint && !alive) {
    cout << "Breakpoint reached at ";    show_pc();
  }
  // //do_log("Finished execution block" << std::hex << pc << std::endl);
}

void processor::step() {
  uint64_t inst = mem->read_doubleword(pc);
  if (pc % 8 != 0) inst = inst >> 32; // adjust for the next instruction in the dword

  uint8_t opcode = EXTRACT_OPCODE_FROM_INST(inst);

  // storage //
  uint8_t funct3;
  uint16_t funct7_3;
  uint8_t rd;
  uint8_t rs1;
  uint8_t rs2;
  int64_t imm;

  uint64_t address;
  uint32_t offset;

  uint64_t dwa; // doubleword A, a buffer for reading doublewords
  // * //
  
  switch (rv64::opcode(opcode)) {

    case rv64::opcode::lui_op:
      //do_log("LUI at PC: " << pc);
      rd = EXTRACT_RD_FROM_INST(inst);
      set_reg(rd, static_cast<int64_t>(static_cast<int32_t>(inst & 0xFFFFF000)));
      //do_log("lui x"<<(unsigned int)rd<<", "<<static_cast<int64_t>(static_cast<int32_t>(inst & 0xFFFFF000)>>12));
    break;

    case rv64::opcode::auipc_op:
      //do_log("AUIPC at " << pc);
      rd = EXTRACT_RD_FROM_INST(inst);
      set_reg(rd, static_cast<int64_t>(pc) + static_cast<int64_t>( static_cast<int32_t>(inst & 0xFFFFF000) ));
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
          //do_log("ADDI at PC: " << pc);
          set_reg(rd, reg[rs1] + imm);
          // //do_log("addi "<<(unsigned int)rd<<", "<<(unsigned int)rs1<<", "<<imm);
          //do_log("x"<<(unsigned int)rd<<" = "<<(unsigned int)reg[rs1]<<" + "<<imm);
        break;

        case rv64::imm_funct3::slti_f3:
          //do_log("SLTI at " << pc);
          set_reg(rd, (static_cast<int64_t>(reg[rs1]) < imm) ? 1 : 0);
        break;

        case rv64::imm_funct3::sltiu_f3:
          //do_log("SLTIU at " << pc);
          set_reg(rd, (uint64_t(reg[rs1]) < uint64_t(imm)) ? 1 : 0);
        break;

        case rv64::imm_funct3::xori_f3:
          //do_log("XORI at " << pc);
          set_reg(rd, reg[rs1] ^ imm);
        break;

        case rv64::imm_funct3::ori_f3:
          //do_log("ORI at " << pc); 
          set_reg(rd, reg[rs1] | imm);
        break;
        
        case rv64::imm_funct3::andi_f3:
          //do_log("ANDI at " << pc);
          set_reg(rd, reg[rs1] & imm);
        break;

        case rv64::imm_funct3::srli_f3: // srai shares the same funct3 as srli: 101
        case rv64::imm_funct3::slli_f3:
          // NOTE: rs2 is used as shamt here
          // NOTE: this is to reuse the already declared variable rs2 prior to the switches
          // NOTE: in RV64, rs2 includes bit 25. RV32 just uses same field as rs2
          rs2 = EXTRACT_SHAMT64_FROM_INST(inst);
          
          // NOT actually funct7_3, drops bit 25
          funct7_3 = (USE_BITMASK(inst, 0xFC000000, 23)) | USE_BITMASK(inst, 0x00007000, 12);
          
          switch (rv64::imm_funct9(funct7_3)) {
            case rv64::imm_funct9::slli_f73:
              //do_log("SLLI at " << pc);
              set_reg(rd, reg[rs1] << rs2);
            break;

            case rv64::imm_funct9::srli_f73:
              //do_log("SRLI at " << pc);
              set_reg(rd, (((uint64_t)reg[rs1]) >> rs2));
            break;

            case rv64::imm_funct9::srai_f73:
              //do_log("SRAI at " << pc);
              set_reg(rd, static_cast<int64_t>(reg[rs1]) >> rs2);
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
          //do_log("ADD at " << pc);
          set_reg(rd, reg[rs1] + reg[rs2]);
        break;

        case rv64::reg_funct73::sub_f:
          //do_log("SUB at " << pc);
          set_reg(rd, reg[rs1] - reg[rs2]);
        break;

        case rv64::reg_funct73::sll_f:
          //do_log("SLL at " << pc);
          set_reg(rd, reg[rs1] << reg[rs2]);
        break;

        case rv64::reg_funct73::slt_f:
          //do_log("SLT at " << pc);
          set_reg(rd, (static_cast<int64_t>(reg[rs1]) < static_cast<int64_t>(reg[rs2])) ? 1 : 0);
        break;

        case rv64::reg_funct73::sltu_f:
          //do_log("SLTU at " << pc);
          set_reg(rd, (static_cast<uint64_t>(reg[rs1]) < static_cast<uint64_t>(reg[rs2])) ? 1 : 0);
        break;

        case rv64::reg_funct73::xor_f:
          //do_log("XOR at " << pc);
          set_reg(rd, reg[rs1] ^ reg[rs2]);
        break;

        case rv64::reg_funct73::srl_f:
          //do_log("SRL at " << pc);
          set_reg(rd, uint64_t((uint64_t)reg[rs1] >> reg[rs2]));
          // reg[rd] = reg[rs1] >> reg[rs2];
        break;

        case rv64::reg_funct73::sra_f:
          //do_log("SRA at " << pc);
          set_reg(rd, static_cast<int64_t>(reg[rs1]) >> reg[rs2]);
          
        break;

        case rv64::reg_funct73::or_f:
          //do_log("OR at " << pc);
          set_reg(rd, reg[rs1] | reg[rs2]);
        break;

        case rv64::reg_funct73::and_f:
          //do_log("AND at " << pc);
          set_reg(rd, reg[rs1] & reg[rs2]);
        break;
      }

    break;

    case rv64::opcode::fen_op:
      // do nothing in stage 1
      //do_log("fence operations decoded but not implemented");
      // stage 2:
      funct3 = EXTRACT_FUNCT3_FROM_INST(inst);
      switch(rv64::fen_funct3(funct3)) {
        case rv64::fen_funct3::fence_f:
          // do nothing in stage 2
          //do_log("fence operation decoded but not implemented");
        break;
        case rv64::fen_funct3::fence_i_f:
          // do nothing in stage 2
          //do_log("fence.i operation decoded but not implemented");
        break;
      }
    break;

    case rv64::opcode::sys_op:
      rs2 = EXTRACT_RS2_FROM_INST(inst);
      switch (rv64::sys_rs2(rs2)) { // these system instructions are weird. they use rs2 to differentiate between them
        case rv64::sys_rs2::ecall_f:
          cout << "ecall: not implemented\n";
        break;
        case rv64::sys_rs2::ebreak_f:
          cout << "ebreak: not implemented\n";
        break;
      }
    break;

    case rv64::opcode::load_op:
      imm = EXTRACT_IMM12_FROM_INST(inst); // upper 12 bits sign extended to a doubleword
      rs1 = EXTRACT_RS1_FROM_INST(inst);
      rd = EXTRACT_RD_FROM_INST(inst);
      address = reg[rs1] + imm;

      //do_log("load_op: address = " << address);
      
      funct3 = EXTRACT_FUNCT3_FROM_INST(inst);
      switch (rv64::load_funct3(funct3)){
        
        case rv64::load_funct3::lb_f:
          offset = address % 8;

          dwa = mem->read_doubleword(address);
          set_reg(rd, int64_t((uint64_t((dwa >> (offset*8))) & 0xFF) << 56) >> 56);
          //do_log("lb x" << (unsigned int)rd << ", " << imm << "(x" << (unsigned int)rs1 << ")");
        break;

        case rv64::load_funct3::lh_f:
          offset = address % 8;

          dwa = mem->read_doubleword(address);
          set_reg(rd, int64_t((uint64_t((dwa >> (offset*8))) & 0xFFFF) << 48) >> 48);
          //do_log("lh x" << (unsigned int)rd << ", " << imm << "(x" << (unsigned int)rs1 << ")");
        break;

        case rv64::load_funct3::lw_f:
          dwa = int32_t(mem->read_word(address));
          set_reg(rd, int64_t(uint64_t(dwa) << 32) >> 32);
          //do_log("lw x" << (unsigned int)rd << ", " << imm << "(x" << (unsigned int)rs1 << ")");
        break;

        case rv64::load_funct3::ld_f:
          //do_log("LD at " << pc);
          if (address % 4 != 0) {
            cout << "Error: misaligned address for ld" << endl;
          }
          dwa = mem->read_doubleword(address);
          set_reg(rd, dwa);
          //do_log("ld x" << (unsigned int)rd << ", " << imm << "(x" << (unsigned int)rs1 << ")");
        break;

        case rv64::load_funct3::lbu_f:
          offset = address % 8;
          dwa = mem->read_doubleword(address);
          if (offset == 0) {
            set_reg(rd, (uint64_t)(dwa & 0xFF));
          } else {
            set_reg(rd, (uint64_t)((dwa >> (offset*8)) & 0xFF));
          }
          
          //do_log("lbu x" << (unsigned int)rd << ", " << imm << "(x" << (unsigned int)rs1 << ")");
        break;

        case rv64::load_funct3::lhu_f:
          offset = address % 8;
          dwa = mem->read_doubleword(address);
          if (offset == 0) {
            set_reg(rd, (uint64_t)(dwa & 0xFFFF));
          } else {
            set_reg(rd, (uint64_t)((dwa >> (offset*8)) & 0xFFFF));
          }
          //do_log("lhu x" << (unsigned int)rd << ", " << imm << "(x" << (unsigned int)rs1 << ")");
        break;

        case rv64::load_funct3::lwu_f:
          dwa = uint32_t(mem->read_word(address));
          set_reg(rd, dwa);
          //do_log("lwu x" << (unsigned int)rd << ", " << imm << "(x" << (unsigned int)rs1 << ")");
        break;
      }

    break;

    case rv64::opcode::store_op:
      imm = EXTRACT_STORE_OFFSET_FROM_INST(inst); // 1111 1110 0000 0000 0000 1111 1000 0000
      rs2 = EXTRACT_RS2_FROM_INST(inst);
      rs1 = EXTRACT_RS1_FROM_INST(inst);
      funct3 = EXTRACT_FUNCT3_FROM_INST(inst);

      address = reg[rs1] + imm;
      
      switch (rv64::store_funct3(funct3)) {

        case (rv64::store_funct3::sb_f):
          mem->write_byte(address, reg[rs2], 0xFF);
          //do_log("sb x" << (unsigned int)rs2 << ", " << imm << "(x" << (unsigned int)rs1 << ")");
        break;

        case (rv64::store_funct3::sh_f):
          mem->write_half(address, reg[rs2], 0xFFFF);
          //do_log("sh x" << (unsigned int)rs2 << ", " << imm << "(x" << (unsigned int)rs1 << ")");
        break;

        case (rv64::store_funct3::sw_f):
          mem->write_word(address, reg[rs2], ~0UL);
          //do_log("sw x" << (unsigned int)rs2 << ", " << imm << "(x" << (unsigned int)rs1 << ")");
        break;

        case (rv64::store_funct3::sd_f):
          mem->write_doubleword(address, reg[rs2], ~0ULL);
          //do_log("sd x" << (unsigned int)rs2 << ", " << imm << "(x" << (unsigned int)rs1 << ")");
        break;
      }

    break;

    case rv64::opcode::jal_op:
      //do_log("JAL at " << pc);
      // imm = USE_BITMASK(inst, 0x80000000, 11) | USE_BITMASK(inst, 0x7FE00000, 20) | USE_BITMASK(inst, 0x00100000, 9) | USE_BITMASK(inst, 0x000FF000, 8);
      // imm = (int32_t)(((((inst) >> 21) & ((1 << 10) - 1)) << 1) |
      //                ((((inst) >> 20) & ((1 << 1) - 1)) << 11) |
      //                ((((inst) >> 12) & ((1 << 8) - 1)) << 12) |
      //                ((-(((inst) >> 31) & 1)) << 20));
      imm = EXTRACT_JAL_OFFSET_FROM_INST(inst);
      rd = EXTRACT_RD_FROM_INST(inst);
      set_reg(rd, pc + 4);
      set_pc(pc + imm);
      pc_changed = true;
    break;

    case rv64::opcode::jalr_op:
      //do_log("JALR at " << pc);
      imm = EXTRACT_IMM12_FROM_INST(inst);
      rs1 = EXTRACT_RS1_FROM_INST(inst);
      rd = EXTRACT_RD_FROM_INST(inst);
      
      // using dwa here as a buffer
      dwa = reg[rs1] + imm;

      if (dwa == 0 && rd == 0 && rs1 == 0) {
        // this signals that the program has ended
        alive = false;
        breakpoint = 0;
      }

      set_reg(rd, pc + 4);
      set_pc(dwa);
      pc_changed = true;
    break;

    case rv64::opcode::branch_op:
      imm = EXTRACT_BRANCH_OFFSET_FROM_INST(inst);

      rs2 = EXTRACT_RS2_FROM_INST(inst);
      rs1 = EXTRACT_RS1_FROM_INST(inst);
      funct3 = EXTRACT_FUNCT3_FROM_INST(inst);

      switch (rv64::branch_funct3(funct3)) {
        case rv64::branch_funct3::beq_f:
          //do_log("BEQ at " << pc);
          if (reg[rs1] == reg[rs2]) {
            pc = pc + imm;
            pc_changed = true;
          }
        break;

        case rv64::branch_funct3::bne_f:
          //do_log("BNE at " << pc);
          if (reg[rs1] != reg[rs2]) {
            pc = pc + imm;
            pc_changed = true;
          }
        break;

        case rv64::branch_funct3::blt_f:
          //do_log("BLT at " << pc);
          if (reg[rs1] < reg[rs2]) {
            pc = pc + imm;
            pc_changed = true;
          }
        break;

        case rv64::branch_funct3::bge_f:
          //do_log("BGE at " << pc);
          if (reg[rs1] >= reg[rs2]) {
            pc = pc + imm;
            pc_changed = true;
          }
        break;

        case rv64::branch_funct3::bltu_f:
          //do_log("BLTU at " << pc);
          if ((uint64_t)reg[rs1] < (uint64_t)reg[rs2]) {
            pc = pc + imm;
            pc_changed = true;
          }
        break;

        case rv64::branch_funct3::bgeu_f:
          //do_log("BGEU at " << pc);
          if ((uint64_t)reg[rs1] >= (uint64_t)reg[rs2]) {
            pc = pc + imm;
            pc_changed = true;
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
          //do_log("ADDIW at " << pc);
          imm = EXTRACT_IMM12_FROM_INST(inst);
          set_reg(rd, int64_t(uint64_t(int64_t(reg[rs1]) + imm) << 32) >> 32);
        break;

        case rv64::imm64_funct3::slliw_f3:
          //do_log("SLLIW at " << pc);
          // shamt is rs2
          rs2 = EXTRACT_SHAMT32_FROM_INST(inst);
          set_reg(rd, (int32_t)reg[rs1] << rs2 );
        break;

        case rv64::imm64_funct3::srliw_f3: // sraiw has the same funct3
          rs2 = EXTRACT_SHAMT32_FROM_INST(inst);

          funct7_3 = EXTRACT_FUNCT7_FROM_INST(inst);

          switch (rv64::imm64_funct7(funct7_3)) {
            case rv64::imm64_funct7::srliw_f7: 
              //do_log("SRLIW at " << pc);
              set_reg(rd, (int64_t)((int32_t)(((uint32_t)reg[rs1]) >> rs2)) );
            break;

            case rv64::imm64_funct7::sraiw_f7:
              //do_log("SRAIW at " << pc);
              set_reg(rd, int64_t ( ((int32_t)reg[rs1]) >> rs2) );
            break;
          }

        break;

      }

    break;
    
    case rv64::opcode::reg64_op:
      rd = EXTRACT_RD_FROM_INST(inst);
      funct7_3 = EXTRACT_FUNCT7_AND_3_FROM_INST(inst);
      rs1 = EXTRACT_RS1_FROM_INST(inst);
      rs2 = EXTRACT_RS2_FROM_INST(inst);

      switch (rv64::reg64_funct73(funct7_3)) {

        case rv64::reg64_funct73::addw_f:
          set_reg(rd, int64_t(
                            uint64_t( int32_t(reg[rs1]) + int32_t(reg[rs2]) ) << 32
                           ) >> 32);
        break;

        case rv64::reg64_funct73::subw_f:
          set_reg(rd, int64_t(
                            uint64_t( int32_t(reg[rs1]) - int32_t(reg[rs2]) ) << 32
                           ) >> 32);
        break;

        case rv64::reg64_funct73::sllw_f:
          set_reg(rd, (int64_t(
                            uint32_t(reg[rs1]) << (reg[rs2] & 0x1F)) << 32)
                            >> 32);
        break;

        case rv64::reg64_funct73::srlw_f:
          set_reg(rd, (int64_t(
                            uint32_t(reg[rs1]) >> (reg[rs2] & 0x1F)) << 32)
                            >> 32);
        break;

        case rv64::reg64_funct73::sraw_f:
          set_reg(rd, (int64_t(
                            int32_t(reg[rs1]) >> (reg[rs2] & 0x1F)) << 32)
                            >> 32);
        break;
      }
    break;

    default:
      //do_log("Unknown instruction at " << pc);
      std::cout << "Illegal instruction at " << pc << std::endl;
    break;

  }
  
}

void processor::clear_breakpoint() {
  breakpoint = NO_BREAKPOINT;
}

void processor::set_breakpoint(uint64_t addr) {
  breakpoint = addr;
  //do_log("Breakpoint set at " << std::hex << addr);
}

void processor::show_prv() {
  // switch (prv) {
  //   case 0:
  //     std::cout << "Machine mode" << std::endl;
  //   break;

  //   case 1:
  //     std::cout << "Supervisor mode" << std::endl;
  //   break;

  //   case 3:
  //     std::cout << "User mode" << std::endl;
  //   break;
  // }
}

void processor::set_prv(uint8_t prv) {
  // this->prv = prv;
}

void processor::show_csr(unsigned int csr_num) {
  // std::cout << "CSR " << csr_num << " = " << csr[csr_num] << std::endl;
}

void processor::set_csr(unsigned int csr_num, uint64_t value) {
  // csr[csr_num] = value;
}

uint64_t processor::get_instruction_count() {
  return instruction_count;
}

uint64_t processor::get_cycle_count() {
  return cycle_count;
}