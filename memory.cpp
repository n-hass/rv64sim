/* ****************************************************************
   RISC-V Instruction Set Simulator
   Computer Architecture, Semester 1, 2023

   Class members for memory

**************************************************************** */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdlib.h>
#include <cstring>
#include <cstdio>

#include "memory.h"
using namespace std;

#include "LogControl.h"

#define validate(address) \
  index = address/blockSize; \
  auto temp_find = mem_m.find(index); \
  if (index == cached_block_index) { \
    block = cached_block_addr; \
  } \
  else if (temp_find == mem_m.end()) { \
    block = (uintptr_t)malloc(blockSize); \
    mem_m[index] = block; \
    cached_block_index = index; \
    cached_block_addr = block; \
  } else { \
    block = temp_find->second; \
    cached_block_index = index; \
    cached_block_addr = block; \
  }
//

// #define validate(address) \
//   index = address/blockSize; \
//   if (mem_m.find(index) == mem_m.end()) { \
//     block = (uintptr_t)malloc(blockSize); \
//     mem_m[index] = block; \
//   } else { \
//     block = mem_m[index]; \
//   }
//

#define validate_with_memset(address) \
  index = address/blockSize; \
  auto temp_find = mem_m.find(index); \
  if (index == cached_block_index) { \
    block = cached_block_addr; \
  } \
  else if (temp_find == mem_m.end()) { \
    block = (uintptr_t)malloc(blockSize); \
    memset((void*)block, 0, blockSize); \
    mem_m[index] = block; \
    cached_block_index = index; \
    cached_block_addr = block; \
  } else { \
    block = temp_find->second; \
    cached_block_index = index; \
    cached_block_addr = block; \
  }
//

// #define validate_with_memset(address) \
//   index = address/blockSize; \
//   if (mem_m.find(index) == mem_m.end()) { \
//     block = (uintptr_t)malloc(blockSize); \
//     memset((void*)block, 0, blockSize); \
//     mem_m[index] = block; \
//   } else { \
//     block = mem_m[index]; \
//   }
//

// Constructor
memory::memory(bool verbose) : mem_m() {
  this->verbose = verbose;
  cached_block_index = -1;
  cached_block_addr = 0x00;
}

// Read a doubleword of data from a doubleword-aligned address.
// If the address is not a multiple of 8, it is rounded down to a multiple of 8.
uint64_t memory::read_doubleword (uint64_t address) {
  // align address to doubleword
  address -= (address % 8);

  uint64_t index;
  uintptr_t block;
  validate(address);

  vlog("Memory read doubleword: address = " << setfill('0') << setw(16) << std::hex << address << ", data = " << *reinterpret_cast< uint64_t* > (block + (address % blockSize)));
  return *reinterpret_cast< uint64_t* > (block + (address % blockSize));
}

uint32_t memory::read_word (uint64_t address) {

  uint64_t index;
  uintptr_t block;
  validate(address);
  

  vlog("Memory read word: address = " << setfill('0') << setw(16) << std::hex << address << ", data = " << *reinterpret_cast< uint64_t* > (block + (address % blockSize)));
  return *reinterpret_cast< uint32_t* > (block + (address % blockSize));
}

// Write a doubleword of data to a doubleword-aligned address.
// If the address is not a multiple of 8, it is rounded down to a multiple of 8.
// The mask contains 1s for bytes to be updated and 0s for bytes that are to be unchanged.
void memory::write_doubleword (uint64_t address, uint64_t data, uint64_t mask) {
  // align address to doubleword
  address -= (address % 8);

  uint64_t index;
  uintptr_t block;
  validate_with_memset(address);

  uint64_t* dw = reinterpret_cast< uint64_t* > (block + (address % blockSize));
  *dw = (*dw & ~mask) | (data & mask);
  vlog("Memory doublewrite word: address = " << setfill('0') << setw(16) << std::hex << address << ", data = " << data << ", mask = " << mask);
}

void memory::write_word (uint64_t address, uint64_t data, uint64_t mask) { 

  uint64_t index;
  uintptr_t block;
  validate(address);

  uint32_t* dw = reinterpret_cast< uint32_t* > (block + (address % blockSize));
  *dw = (*dw & ~mask) | (data & mask);
  vlog("Memory write word: address = " << setfill('0') << setw(16) << std::hex << address << ", data = " << data << ", mask = " << mask);
}

void memory::write_half(uint64_t address, uint64_t data, uint64_t mask) {

  uint64_t index;
  uintptr_t block;
  validate(address);

  uint16_t *mem = reinterpret_cast<uint16_t *>(block + (address % blockSize));
  *mem = (*mem & ~mask) | (data & mask);
  vlog("Memory write half: address = " << setfill('0') << setw(16) << std::hex << address << ", data = " << data << ", mask = " << mask);
}

void memory::write_byte(uint64_t address, uint64_t data, uint64_t mask) {

  uint64_t index;
  uintptr_t block;
  validate(address);

  uint16_t *mem = reinterpret_cast<uint16_t *>(block + (address % blockSize));
  *mem = (*mem & ~mask) | (data & mask);
  vlog("Memory write byte: address = " << setfill('0') << setw(16) << std::hex << address << ", data = " << data << ", mask = " << mask);
}

// Load a hex image file and provide the start address for execution from the file in start_address.
// Return true if the file was read without error, or false otherwise.
bool memory::load_file(string file_name, uint64_t &start_address) {
  ifstream input_file(file_name);
  string input;
  unsigned int line_count = 0;
  unsigned int byte_count = 0;
  char record_start;
  char byte_string[3];
  char halfword_string[5];
  unsigned int record_length;
  unsigned int record_address;
  unsigned int record_type;
  unsigned int record_data;
  unsigned int record_checksum;
  bool end_of_file_record = false;
  uint64_t load_address;
  uint64_t load_data;
  uint64_t load_mask;
  uint64_t load_base_address = 0x0000000000000000ULL;
  start_address = 0x0000000000000000ULL;
  if (input_file.is_open()) {
    while (true) {
      line_count++;
      input_file >> record_start;
      if (record_start != ':') {
	cout << "Input line " << dec << line_count << " does not start with colon character" << endl;
	return false;
      }
      input_file.get(byte_string, 3);
      sscanf(byte_string, "%x", &record_length);
      input_file.get(halfword_string, 5);
      sscanf(halfword_string, "%x", &record_address);
      input_file.get(byte_string, 3);
      sscanf(byte_string, "%x", &record_type);
      switch (record_type) {
      case 0x00:  // Data record
	for (unsigned int i = 0; i < record_length; i++) {
	  input_file.get(byte_string, 3);
	  sscanf(byte_string, "%x", &record_data);
	  load_address = (load_base_address | (uint64_t)(record_address)) + i;
	  load_data = (uint64_t)(record_data) << ((load_address % 8) * 8);
	  load_mask = 0x00000000000000ffULL << ((load_address % 8) * 8);
	  write_doubleword(load_address & 0xfffffffffffffff8ULL, load_data, load_mask);
	  byte_count++;
	}
	break;
      case 0x01:  // End of file
	end_of_file_record = true;
	break;
      case 0x02:  // Extended segment address (set bits 19:4 of load base address)
	load_base_address = 0x0000000000000000ULL;
	for (unsigned int i = 0; i < record_length; i++) {
	  input_file.get(byte_string, 3);
	  sscanf(byte_string, "%x", &record_data);
	  load_base_address = (load_base_address << 8) | (record_data << 4);
	}
	break;
      case 0x03:  // Start segment address (ignored)
	for (unsigned int i = 0; i < record_length; i++) {
	  input_file.get(byte_string, 3);
	  sscanf(byte_string, "%x", &record_data);
	}
	break;
      case 0x04:  // Extended linear address (set upper halfword of load base address)
	load_base_address = 0x0000000000000000ULL;
	for (unsigned int i = 0; i < record_length; i++) {
	  input_file.get(byte_string, 3);
	  sscanf(byte_string, "%x", &record_data);
	  load_base_address = (load_base_address << 8) | (record_data << 16);
	}
	break;
      case 0x05:  // Start linear address (set execution start address)
	start_address = 0x0000000000000000ULL;
	for (unsigned int i = 0; i < record_length; i++) {
	  input_file.get(byte_string, 3);
	  sscanf(byte_string, "%x", &record_data);
	  start_address = (start_address << 8) | record_data;
	}
	break;
      }
      input_file.get(byte_string, 3);
      sscanf(byte_string, "%x", &record_checksum);
      input_file.ignore();
      if (end_of_file_record)
	break;
    }
    input_file.close();
    cout << dec << byte_count << " bytes loaded, start address = "
	 << setw(16) << setfill('0') << hex << start_address << endl;
    return true;
  }
  else {
    cout << "Failed to open file" << endl;
    return false;
  }
}

memory::~memory() {
  // free every block, which is every key/value in the map
  for (auto it = mem_m.begin(); it != mem_m.end(); ++it) {
    // free an individual block
    free( reinterpret_cast<void*>(it->second) );
  }
}