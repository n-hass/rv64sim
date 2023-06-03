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
#include "Definitions.h"
#include "LogControl.hpp"
#include "Bits.h"

using namespace std;
using rv64::csr;

#define set_reg_m(reg_num, value) \
  if ((reg_num) != 0) { \
    reg[reg_num] = (value); \
  }
//

// Consructor
processor::processor (memory* main_memory, bool verbose, bool stage2) {
  this->verbose = verbose;
  this->stage2 = stage2;
  this->mem = main_memory;
  
  // initial states
  this->pc = 0;
  this->breakpoint = NO_BREAKPOINT;
  this->pc_changed = false;
  this->instruction_count = 0;
  memset(reg, 0, sizeof(int64_t)*32);
  this->prv = 3;
  
  // initialise the CSRs
  this->csr[0xf11] = 0x00;               // mvendorid
  this->csr[0xf12] = 0x00;               // marchid
  this->csr[0xf13] = 0x2023020000000000; // mimpid
  this->csr[0xf14] = 0x00;               // mhartid
  this->csr[0x300] = 0x0000000200000000; // mstatus
  this->csr[0x301] = 0x8000000000100100; // misa
  this->csr[0x304] = 0x00;               // mie
  this->csr[0x305] = 0x00;               // mtvec
  this->csr[0x340] = 0x00;               // mscratch
  this->csr[0x341] = 0x00;               // mepc
  this->csr[0x342] = 0x00;               // mcause
  this->csr[0x343] = 0x00;               // mtval
  this->csr[0x344] = 0x00;               // mip


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
      // cout << "Error: misaligned pc" << endl;
      exception(0, 0);
    } else {
      // check for interrupts in order of priority
      if ( (csr[csr::mstatus] & 0x8) || prv == 0) {
        if (EXTRACT_BIT(csr[csr::mie], 11) & EXTRACT_BIT(csr[csr::mip], 11)) {
          interrupt(rv64::interrupt::machine_external);
        } 
        else if (EXTRACT_BIT(csr[csr::mie], 3) & EXTRACT_BIT(csr[csr::mip], 3)) {
          interrupt(rv64::interrupt::machine_software);
        } 
        else if (EXTRACT_BIT(csr[csr::mie], 7) & EXTRACT_BIT(csr[csr::mip], 7)) {
          interrupt(rv64::interrupt::machine_timer);
        }
        else if (EXTRACT_BIT(csr[csr::mie], 8) & EXTRACT_BIT(csr[csr::mip], 8)) {
          interrupt(rv64::interrupt::user_external);
        }
        else if ((csr[csr::mie] & 0x1) & (csr[csr::mip] & 0x1) ) {
          interrupt(rv64::interrupt::user_software);
        }
        else if (EXTRACT_BIT(csr[csr::mie], 4) & EXTRACT_BIT(csr[csr::mip], 4)) {
          interrupt(rv64::interrupt::user_timer);
        }
      }
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
    cout << "Breakpoint reached at "; show_pc();
  }
  vlog("Finished execution block at pc: " << std::hex << pc << std::endl);
}

void processor::step() {
  uint64_t inst = mem->read_doubleword(pc);
  if (pc % 8 != 0) inst = inst >> 32;   // adjust for next instruction in the doubleword

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
      vlog("LUI at PC: " << pc);
      rd = EXTRACT_RD_FROM_INST(inst);
      set_reg_m(rd, static_cast<int64_t>(static_cast<int32_t>(inst & 0xFFFFF000)));
      vlog("lui x"<<(unsigned int)rd<<", "<<static_cast<int64_t>(static_cast<int32_t>(inst & 0xFFFFF000)>>12));
    break;

    case rv64::opcode::auipc_op:
      vlog("AUIPC at " << pc);
      rd = EXTRACT_RD_FROM_INST(inst);
      set_reg_m(rd, static_cast<int64_t>(pc) + static_cast<int64_t>( static_cast<int32_t>(inst & 0xFFFFF000) ));
    break;
  
    case rv64::opcode::imm_op:
      rs1 = EXTRACT_RS1_FROM_INST(inst);
      rd = EXTRACT_RD_FROM_INST(inst);
      funct3 = EXTRACT_FUNCT3_FROM_INST(inst);
      imm = EXTRACT_IMM12_FROM_INST(inst); // upper 12 bits sign extended to a doubleword

      switch (rv64::imm_funct3(funct3)) {
        case rv64::imm_funct3::addi_f3:
          vlog("ADDI at PC: " << pc);
          set_reg_m(rd, reg[rs1] + imm);
          vlog("x"<<(unsigned int)rd<<" = "<<(unsigned int)reg[rs1]<<" + "<<imm);
        break;

        case rv64::imm_funct3::slti_f3:
          vlog("SLTI at " << pc);
          set_reg_m(rd, (static_cast<int64_t>(reg[rs1]) < imm) ? 1 : 0);
        break;

        case rv64::imm_funct3::sltiu_f3:
          vlog("SLTIU at " << pc);
          set_reg_m(rd, (uint64_t(reg[rs1]) < uint64_t(imm)) ? 1 : 0);
        break;

        case rv64::imm_funct3::xori_f3:
          vlog("XORI at " << pc);
          set_reg_m(rd, reg[rs1] ^ imm);
        break;

        case rv64::imm_funct3::ori_f3:
          vlog("ORI at " << pc); 
          set_reg_m(rd, reg[rs1] | imm);
        break;
        
        case rv64::imm_funct3::andi_f3:
          vlog("ANDI at " << pc);
          set_reg_m(rd, reg[rs1] & imm);
        break;

        case rv64::imm_funct3::srli_f3: // srai shares the same funct3 as srli: 101
        case rv64::imm_funct3::slli_f3:
          // NOTE: rs2 is used as shamt here
          // NOTE: this is to reuse the already declared variable rs2 prior to the switches
          // NOTE: in RV64, rs2 includes bit 25. RV32 just uses same field as rs2
          rs2 = EXTRACT_SHAMT64_FROM_INST(inst);
          
          // NOT actually funct7_3, drops bit 25 as its used by shamt64
          funct7_3 = (USE_BITMASK(inst, 0xFC000000, 23)) | USE_BITMASK(inst, 0x00007000, 12);
          
          switch (rv64::imm_funct9(funct7_3)) {
            case rv64::imm_funct9::slli_f73:
              vlog("SLLI at " << pc);
              set_reg_m(rd, reg[rs1] << rs2);
            break;

            case rv64::imm_funct9::srli_f73:
              vlog("SRLI at " << pc);
              set_reg_m(rd, (((uint64_t)reg[rs1]) >> rs2));
            break;

            case rv64::imm_funct9::srai_f73:
              vlog("SRAI at " << pc);
              set_reg_m(rd, static_cast<int64_t>(reg[rs1]) >> rs2);
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
          vlog("ADD at " << pc);
          set_reg_m(rd, reg[rs1] + reg[rs2]);
        break;

        case rv64::reg_funct73::sub_f:
          vlog("SUB at " << pc);
          set_reg_m(rd, reg[rs1] - reg[rs2]);
        break;

        case rv64::reg_funct73::sll_f:
          vlog("SLL at " << pc);
          set_reg_m(rd, reg[rs1] << reg[rs2]);
        break;

        case rv64::reg_funct73::slt_f:
          vlog("SLT at " << pc);
          set_reg_m(rd, (static_cast<int64_t>(reg[rs1]) < static_cast<int64_t>(reg[rs2])) ? 1 : 0);
        break;

        case rv64::reg_funct73::sltu_f:
          vlog("SLTU at " << pc);
          set_reg_m(rd, (static_cast<uint64_t>(reg[rs1]) < static_cast<uint64_t>(reg[rs2])) ? 1 : 0);
        break;

        case rv64::reg_funct73::xor_f:
          vlog("XOR at " << pc);
          set_reg_m(rd, reg[rs1] ^ reg[rs2]);
        break;

        case rv64::reg_funct73::srl_f:
          vlog("SRL at " << pc);
          set_reg_m(rd, uint64_t((uint64_t)reg[rs1] >> reg[rs2]));
          // reg[rd] = reg[rs1] >> reg[rs2];
        break;

        case rv64::reg_funct73::sra_f:
          vlog("SRA at " << pc);
          set_reg_m(rd, static_cast<int64_t>(reg[rs1]) >> reg[rs2]);
          
        break;

        case rv64::reg_funct73::or_f:
          vlog("OR at " << pc);
          set_reg_m(rd, reg[rs1] | reg[rs2]);
        break;

        case rv64::reg_funct73::and_f:
          vlog("AND at " << pc);
          set_reg_m(rd, reg[rs1] & reg[rs2]);
        break;
      }

    break;

    case rv64::opcode::fen_op:
      // do nothing in stage 1
      // stage 2:
      funct3 = EXTRACT_FUNCT3_FROM_INST(inst);
      switch(rv64::fen_funct3(funct3)) {
        case rv64::fen_funct3::fence_f:
          // do nothing in stage 1
          vlog("fence operation decoded but not implemented");
        break;
        case rv64::fen_funct3::fence_i_f:
          // do nothing in stage 1
          vlog("fence.i operation decoded but not implemented");
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

      vlog("load_op: address = " << address);
      
      funct3 = EXTRACT_FUNCT3_FROM_INST(inst);
      switch (rv64::load_funct3(funct3)){
        
        case rv64::load_funct3::lb_f:
          offset = address % 8;

          dwa = mem->read_doubleword(address);
          set_reg_m(rd, int64_t((uint64_t((dwa >> (offset*8))) & 0xFF) << 56) >> 56);
          vlog("lb x" << (unsigned int)rd << ", " << imm << "(x" << (unsigned int)rs1 << ")");
        break;

        case rv64::load_funct3::lh_f:
          offset = address % 8;

          dwa = mem->read_doubleword(address);
          set_reg_m(rd, int64_t((uint64_t((dwa >> (offset*8))) & 0xFFFF) << 48) >> 48);
          vlog("lh x" << (unsigned int)rd << ", " << imm << "(x" << (unsigned int)rs1 << ")");
        break;

        case rv64::load_funct3::lw_f:
          dwa = int32_t(mem->read_word(address));
          set_reg_m(rd, int64_t(int32_t(dwa)));
          vlog("lw x" << (unsigned int)rd << ", " << imm << "(x" << (unsigned int)rs1 << ")");
        break;

        case rv64::load_funct3::ld_f:
          vlog("LD at " << pc);
          if (address % 4 != 0) {
            cout << "Error: misaligned address for ld" << endl;
          }
          dwa = mem->read_doubleword(address);
          set_reg_m(rd, dwa);
          vlog("ld x" << (unsigned int)rd << ", " << imm << "(x" << (unsigned int)rs1 << ")");
        break;

        case rv64::load_funct3::lbu_f:
          offset = address % 8;
          dwa = mem->read_doubleword(address);
          if (offset == 0) {
            set_reg_m(rd, (uint64_t)(dwa & 0xFF));
          } else {
            set_reg_m(rd, (uint64_t)((dwa >> (offset*8)) & 0xFF));
          }
          
          vlog("lbu x" << (unsigned int)rd << ", " << imm << "(x" << (unsigned int)rs1 << ")");
        break;

        case rv64::load_funct3::lhu_f:
          offset = address % 8;
          dwa = mem->read_doubleword(address);
          if (offset == 0) {
            set_reg_m(rd, (uint64_t)(dwa & 0xFFFF));
          } else {
            set_reg_m(rd, (uint64_t)((dwa >> (offset*8)) & 0xFFFF));
          }
          vlog("lhu x" << (unsigned int)rd << ", " << imm << "(x" << (unsigned int)rs1 << ")");
        break;

        case rv64::load_funct3::lwu_f:
          dwa = uint32_t(mem->read_word(address));
          set_reg_m(rd, dwa);
          vlog("lwu x" << (unsigned int)rd << ", " << imm << "(x" << (unsigned int)rs1 << ")");
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
          vlog("sb x" << (unsigned int)rs2 << ", " << imm << "(x" << (unsigned int)rs1 << ")");
        break;

        case (rv64::store_funct3::sh_f):
          mem->write_half(address, reg[rs2], 0xFFFF);
          vlog("sh x" << (unsigned int)rs2 << ", " << imm << "(x" << (unsigned int)rs1 << ")");
        break;

        case (rv64::store_funct3::sw_f):
          mem->write_word(address, reg[rs2], ~0UL);
          vlog("sw x" << (unsigned int)rs2 << ", " << imm << "(x" << (unsigned int)rs1 << ")");
        break;

        case (rv64::store_funct3::sd_f):
          mem->write_doubleword(address, reg[rs2], ~0ULL);
          vlog("sd x" << (unsigned int)rs2 << ", " << imm << "(x" << (unsigned int)rs1 << ")");
        break;
      }

    break;

    case rv64::opcode::jal_op:
      vlog("JAL at " << pc);
      imm = EXTRACT_JAL_OFFSET_FROM_INST(inst);
      rd = EXTRACT_RD_FROM_INST(inst);
      set_reg_m(rd, pc + 4);
      set_pc(pc + imm);
      pc_changed = true;
    break;

    case rv64::opcode::jalr_op:
      vlog("JALR at " << pc);
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

      set_reg_m(rd, pc + 4);
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
          if (reg[rs1] == reg[rs2]) {
            vlog("taking BEQ at " << pc);
            set_pc( pc + imm );
            pc_changed = true;
          }
        break;

        case rv64::branch_funct3::bne_f:
          if (reg[rs1] != reg[rs2]) {
            vlog("taking BNE at " << pc);
            set_pc( pc + imm );
            pc_changed = true;
          }
        break;

        case rv64::branch_funct3::blt_f:
          if (reg[rs1] < reg[rs2]) {
            vlog("taking BLT at " << pc);
            set_pc( pc + imm );
            pc_changed = true;
          }
        break;

        case rv64::branch_funct3::bge_f:
          if (reg[rs1] >= reg[rs2]) {
            vlog("taking BGE at " << pc);
            set_pc( pc + imm );
            pc_changed = true;
          }
        break;

        case rv64::branch_funct3::bltu_f:
          if ((uint64_t)reg[rs1] < (uint64_t)reg[rs2]) {
            vlog("taking BLTU at " << pc);
            set_pc( pc + imm );
            pc_changed = true;
          }
        break;

        case rv64::branch_funct3::bgeu_f:
          if ((uint64_t)reg[rs1] >= (uint64_t)reg[rs2]) {
            vlog("taking BGEU at " << pc);
            set_pc( pc + imm );
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
          vlog("ADDIW at " << pc);
          imm = EXTRACT_IMM12_FROM_INST(inst);
          set_reg_m(rd, int64_t(int32_t(int32_t(reg[rs1]) + imm)));
        break;

        case rv64::imm64_funct3::slliw_f3:
          vlog("SLLIW at " << pc);
          // shamt is rs2
          rs2 = EXTRACT_SHAMT32_FROM_INST(inst);
          set_reg_m(rd, (int32_t)reg[rs1] << rs2 );
        break;

        case rv64::imm64_funct3::srliw_f3: // sraiw has the same funct3
          rs2 = EXTRACT_SHAMT32_FROM_INST(inst);

          funct7_3 = EXTRACT_FUNCT7_FROM_INST(inst);

          switch (rv64::imm64_funct7(funct7_3)) {
            case rv64::imm64_funct7::srliw_f7: 
              vlog("SRLIW at " << pc);
              set_reg_m(rd, (int64_t)((int32_t)(((uint32_t)reg[rs1]) >> rs2)) );
            break;

            case rv64::imm64_funct7::sraiw_f7:
              vlog("SRAIW at " << pc);
              set_reg_m(rd, int64_t ( ((int32_t)reg[rs1]) >> rs2) );
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
          set_reg_m(rd, int64_t(int32_t( int32_t(reg[rs1]) + int32_t(reg[rs2]) )));
        break;

        case rv64::reg64_funct73::subw_f:
          set_reg_m(rd, int64_t(int32_t( int32_t(reg[rs1]) - int32_t(reg[rs2]) )));
        break;

        case rv64::reg64_funct73::sllw_f:
          set_reg_m(rd, (int64_t(int32_t(reg[rs1]) << (reg[rs2] & 0x1F))));
        break;

        case rv64::reg64_funct73::srlw_f:
          set_reg_m(rd, (int64_t(int32_t(uint32_t(reg[rs1]) >> (reg[rs2] & 0x1F)))));
        break;

        case rv64::reg64_funct73::sraw_f:
          set_reg_m(rd, (int64_t(int32_t(int32_t(reg[rs1]) >> (reg[rs2] & 0x1F)))));
        break;
      }
    break;

    default:
      vlog("Unknown instruction at " << pc);
      std::cout << "Illegal instruction at " << pc << std::endl;
    break;

  }
  
}

void processor::clear_breakpoint() {
  breakpoint = NO_BREAKPOINT;
}

void processor::set_breakpoint(uint64_t addr) {
  breakpoint = addr;
  vlog("Breakpoint set at " << std::hex << addr);
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
  this->prv = prv;
}

void processor::show_csr(unsigned int csr_num) {
  switch (csr_num) {
    case csr::mvendorid:
    case csr::marchid:
    case csr::mimpid:
    case csr::mhartid:
    case csr::mstatus:
    case csr::misa:
    case csr::mie:
    case csr::mtvec:
    case csr::mscratch:
    case csr::mepc:
    case csr::mcause:
    case csr::mtval:
    case csr::mip:
      cout << setw(16) << setfill('0') << hex << csr[csr_num] << endl;
      break;

    default:
      cout << "Illegal CSR number" << endl;
      break;
  }


  cout << "CSR " << csr_num << " = " << csr[csr_num] << endl;
  
}

void processor::set_csr(unsigned int csr_num, uint64_t value) {

  // apply the writable rules to the input value
  switch (csr_num) {
    case csr::mstatus:
      value = value & 0x1888;
      value = value | 0x200000000;
      break;
    case csr::misa:
      value = value & 0x8000000000100100;
      break;
    case csr::mie:
    case csr::mip:
      value = value & 0x999;
      break;
    case csr::mtvec:
      if (value & 0x1) {
        // direct mode
        value = value & 0xfffffffffffffffc;
      } else {
        // vectored mode
        value = value & 0xffffffffffffff01;
      }
      break;
    case csr::mepc:
      value = value & 0xfffffffffffffffc;
      break;
    case csr::mcause:
      value = value & 0x800000000000000f;
      break;
    case csr::mtval:
    case csr::mscratch:
      break;

    case csr::mvendorid:
    case csr::marchid:
    case csr::mimpid:
    case csr::mhartid:
      cout<<"Illegal write to read-only CSR"<<endl;
      return;

    default:
      cout<<"Invalid CSR"<<endl;
      return;
  }

  csr[csr_num] = value;
}

uint64_t processor::get_instruction_count() {
  return instruction_count;
}

uint64_t processor::get_cycle_count() {
  return cycle_count;
}

void processor::exception(uint64_t cause, uint32_t inst) {
  vlog("Exception: " << cause << ", at: " << std::hex << pc << ", instruction = " << inst);

  uint64_t pc_0 = this->pc;

  // store the original pc that caused the exception
  csr[csr::mepc] = pc_0;

  set_csr(csr::mcause, cause);

  // set pc to the address of the exception handler
  if (csr[csr::mtvec] & 0x1) {
    // direct mode
    pc = csr[csr::mtvec] & ~0x1;
  } else {
    // vectored mode
    pc = (csr[csr::mtvec] & ~0x1) + ( (cause & 0x1) << 2);
  }

  // set mstatus based on privelage level
  if (prv == 3) {

    // mpp = 0b11
    csr[csr::mstatus] = csr[csr::mstatus] | 0x1800;

    // mpie = 0b0
    csr[csr::mstatus] = csr[csr::mstatus] & 0xffffffffffffff7f;

  } 
  else if (prv == 0) {

    // mpp = 0b0
    csr[csr::mstatus] = csr[csr::mstatus] & 0xffffffffffffe7ff;

    // set mpie
    if (csr[csr::mstatus] & 0x8) {
      // mpie = 1
      csr[csr::mstatus] = csr[csr::mstatus] | 0x80;
    } else {
      // mpie = 0
      csr[csr::mstatus] = csr[csr::mstatus] & 0xffffffffffffff7f;
    }

    // mie = 0b0
    csr[csr::mstatus] = csr[csr::mstatus] & 0xfffffffffffffff7;
  }

  switch (cause) {
    case rv64::except::pc_misaligned:
      instruction_count++;
      this->pc += 4;
      // set mtval to the original misaligned pc
      set_csr(csr::mtval, pc_0);
      break;
    
    case rv64::except::illegal_instruction:
      // set mtval to the instruction that caused the exception
      set_csr(csr::mtval, inst);
      break;
    
    case rv64::except::load_address_misaligned:
    case rv64::except::store_address_misaligned:
      // set mtval to the misaligned destination memory address
      set_csr(csr::mtval, EXTRACT_RS1_FROM_INST(inst));
      break;

    case rv64::except::ecall_from_u:
      set_prv(3);
      // no break, must do mtval set for both u and m
    case rv64::except::ecall_from_m:
      // set mtval to the instruction that caused the exception
      set_csr(csr::mtval, 0);
      break;

    default:
      vlog("Unimplemented exception");
      break;
  }

  // adjustments
  pc = pc - 4;
  instruction_count --;  
}

void processor::interrupt(uint64_t cause) {
  vlog("Interrupt: " << cause << ", at: " << std::hex << pc);

  // mpie = 1
  csr[csr::mstatus] = csr[csr::mstatus] | 0x80;

  // store pc in mepc
  set_csr(csr::mepc, pc);

  // set mcause to the cause of the interrupt
  set_csr(csr::mcause, 0x8000000000000000 + cause);

  // set mtvec to the address of the interrupt handler
  if (csr[csr::mtvec] & 0x1) {
    // direct mode
    pc = csr[csr::mtvec] & 0xfffffffffffffffc;
  } else {
    // vectored mode
    pc = (csr[csr::mtvec] & 0xfffffffffffffffc) + ( cause << 2);
  }

  if (prv == 3) { // if machine mode
    // mpp = 0b11
    csr[csr::mstatus] = csr[csr::mstatus] | 0x1800;
  } 
  else if (prv == 0) { // if user mode

    // switch to machine mode
    set_prv(3);


    if ((csr[csr::mstatus] & 0x8) == false) {
      // mpie = 0b0
      csr[csr::mstatus] = csr[csr::mstatus] & 0xffffffffffffff7f;
    }
    
  }

  // mie = 0b0
  csr[csr::mstatus] = csr[csr::mstatus] & 0xfffffffffffffff7;

  switch(cause) {
    case rv64::interrupt::user_software:
      vlog("User software interrupt");
      break;
    
    case rv64::interrupt::machine_software:
      vlog("Machine software interrupt");
      break;

    case rv64::interrupt::user_timer:
      vlog("User timer interrupt")
      break;
    
    case rv64::interrupt::machine_timer:
      vlog("Machine timer interrupt");
      break;
    
    case rv64::interrupt::user_external:
      vlog("User external interrupt")
      break;

    case rv64::interrupt::machine_external:
      vlog("Machine external interrupt");
      break;

    default:
      vlog("Unimplemented interrupt");
      break;
  }

}