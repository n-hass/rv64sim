#pragma once

// use extract bits to get the bit from start to end from value. IE (0x1234, 0, 3) will return 0x4.
// start = LSB of result, end = MSB of result. Start must always be less than end.
#define BITMASK(high, low) (((1ULL << ((high) - (low) + 1)) - 1) << (low))

/**
 * Extracts bits from a value by specifying the high and low bits
 */
#define EXTRACT_BITS(value, high, low) (((value) & BITMASK((high), (low))) >> (low))

/**
 * Extact a singular bit from a value
 */
#define EXTRACT_BIT(value, bit_num_little_endian) ((value >> bit_num_little_endian) & 0x1)

/**
 * Extracts bits from a value by masking the original and THEN shifting the result.
 */
#define USE_BITMASK(value, mask, shift) (((value) & (mask)) >> (shift))

// Field extraction from a RV64 RISC-V instructions
#define EXTRACT_OPCODE_FROM_INST(value) ((value) & 0x7f)
#define EXTRACT_FUNCT3_FROM_INST(value) (((value) >> 12) & 0x7)
#define EXTRACT_FUNCT7_FROM_INST(value) (((value) >> 25) & 0x7f)
#define EXTRACT_FUNCT7_AND_3_FROM_INST(value) ( (((value) & 0xFE000000) >> 22) | (((value) >> 12) & 0x7) )
#define EXTRACT_RD_FROM_INST(value) (((value) >> 7) & 0x1f)
#define EXTRACT_RS1_FROM_INST(value) (((value) >> 15) & 0x1f)
#define EXTRACT_RS2_FROM_INST(value) (((value) >> 20) & 0x1f)
#define EXTRACT_IMM12_FROM_INST(value) ( (int64_t)(((int32_t)(value)) >> 20) ) // inlcudes sign extension
#define EXTRACT_SHAMT32_FROM_INST(value) (((value) >> 20) & 0x1f)
#define EXTRACT_SHAMT64_FROM_INST(value) (((value) >> 20) & 0x3f)
#define EXTRACT_STORE_OFFSET_FROM_INST(value) ( (int64_t)((int32_t)(((value) & 0xFE000000) | (((value) & 0x00000F80)<<13))) >> 20) 
#define EXTRACT_BRANCH_OFFSET_FROM_INST(value) (int64_t( \
                                      uint64_t(USE_BITMASK((value), 0xFE000000, 25) >> 6) << 63 | \
                                      uint64_t(USE_BITMASK((value), 0x00000F80, 7) % 2) << 62 | \
                                      uint64_t(USE_BITMASK((value), 0xFE000000, 25) & 0x3F) << 56 | \
                                      uint64_t(USE_BITMASK((value), 0x00000F80, 7) >> 1) << 52 \
                                    ) >> 51 )
#define EXTRACT_JAL_OFFSET_FROM_INST(value) ( (int32_t)((((((value)) >> 21) & ((1 << 10) - 1)) << 1) | \
                                                        (((((value)) >> 20) & ((1 << 1) - 1)) << 11) | \
                                                        (((((value)) >> 12) & ((1 << 8) - 1)) << 12) | \
                                                        ((-((((value)) >> 31) & 1)) << 20)) )