#pragma once

// use extract bits to get the bit from start to end from value. IE (0x1234, 0, 3) will return 0x4.
// start = LSB of result, end = MSB of result. Start must always be less than end.
#define BITMASK(high, low) (((1ULL << ((high) - (low) + 1)) - 1) << (low))

/*
* Extracts bits from a value by specifying the high and low bits
*/
#define EXTRACT_BITS(value, high, low) (((value) & BITMASK((high), (low))) >> (low))

/**
* Extracts bits from a value by masking the original and THEN shifting the result.
*/
#define USE_BITMASK(value, mask, shift) (((value) & (mask)) >> (shift))

// Extraction from a 32-bit RISC-V instruction
#define EXTRACT_OPCODE_FROM_INST(value) ((value) & 0x7f)
#define EXTRACT_FUNCT3_FROM_INST(value) (((value) >> 12) & 0x7)
#define EXTRACT_FUNCT7_FROM_INST(value) (((value) >> 25) & 0x7f)
// #define EXTRACT_FUNCT3_AND_7_FROM_INST(value) (EXTRACT_FUNCT7_FROM_INST((value)) << 3) | EXTRACT_FUNCT3_FROM_INST(value)
#define EXTRACT_FUNCT7_AND_3_FROM_INST(value) ( (((value) & 0xFE000000) >> 22) | (((value) >> 12) & 0x7) )
#define EXTRACT_RD_FROM_INST(value) (((value) >> 7) & 0x1f)
#define EXTRACT_RS1_FROM_INST(value) (((value) >> 15) & 0x1f)
#define EXTRACT_RS2_FROM_INST(value) (((value) >> 20) & 0x1f)
// #define EXTRACT_IMM12_FROM_INST(value) ( int64_t((uint64_t)(value) << 32) >> 52 ) // includes sign extension
#define EXTRACT_IMM12_FROM_INST(value) (((int64_t)((uint64_t)(value) << 32)) >> 52) // inlcudes sign extension

