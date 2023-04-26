#ifndef INSTRUCTION_DEFS_H
#define INSTRUCTION_DEFS_H
namespace rv64 {
  // opcode 
  enum opcode {
    lui_op = 0b0110111,
    auipc_op = 0b0010111,
    imm_op = 0b0010011,
    reg_op = 0b0110011,
    fen_op = 0b0001111,
    sys_op = 0b1110011,
    load_op = 0b0000011,
    store_op = 0b0100011,
    jal_op = 0b1101111,
    jalr_op = 0b1100111,
    branch_op = 0b1100011,
    imm64_op = 0b0011011,
    reg64_op = 0b0111011,
  };

  /* funct codes.
     funct3 = strictly: funct3
     funct73 = strictly: (funct3 << 7) & funct7

     The individual enum attributes are named with f7/3 when the enum for a single 
     instruction type is split in two (e.g. imm_funct3 for 14:12 of the I-type instructions
      and imm_funct73 for 31:25+14:12 of the I-type instructions, with funct3 concatenated from the LSB of funct7)
  */
  enum imm_funct3 {
    addi_f3 = 0b000,
    slti_f3 = 0b010,
    sltiu_f3 = 0b011,
    xori_f3 = 0b100,
    ori_f3 = 0b110,
    andi_f3 = 0b111,
    slli_f3 = 0b001,
    srli_f3 = 0b101,
    srai_f3 = 0b101, // note is a duplicate of above, is differentiated by funct7 (most significant 7 bits)
  };
  // note: is 31:26 + 14:12, NOT full funct7
  enum imm_funct9 { // this must be split as these are imm instructions (I-format) that don't use 12-bit immediate values and instead emulate the functionality of funct7 and shamt
    slli_f73 = 0b000000001,
    srli_f73 = 0b000000101,
    srai_f73 = 0b010000101
  };
  enum reg_funct73 {
    add_f = 0b0000000000,
    sub_f = 0b0100000000,
    sll_f = 0b0000000001,
    slt_f = 0b0000000010,
    sltu_f = 0b0000000011,
    xor_f = 0b0000000100,
    srl_f = 0b0000000101,
    sra_f = 0b0100000101,
    or_f = 0b0000000110,
    and_f = 0b0000000111
  };
  enum fen_funct3 {
    fence_f = 0b000,
    fence_i_f = 0b001
  };
  enum sys_rs2 {
    ecall_f = 0b000,
    ebreak_f = 0b001
  };
  enum load_funct3 {
    lb_f = 0b000,
    lh_f = 0b001,
    lw_f = 0b010,
    ld_f = 0b011,
    lbu_f = 0b100,
    lhu_f = 0b101,
    lwu_f = 0b110,
  };
  enum store_funct3 {
    sb_f = 0b000,
    sh_f = 0b001,
    sw_f = 0b010,
    sd_f = 0b011
  };
  enum branch_funct3 {
    beq_f = 0b000,
    bne_f = 0b001,
    blt_f = 0b100,
    bge_f = 0b101,
    bltu_f = 0b110,
    bgeu_f = 0b111
  };
  enum imm64_funct3 {
    addiw_f3 = 0b000,
    slliw_f3 = 0b001,
    srliw_f3 = 0b101,
    sraiw_f3 = 0b101, 
  };
  enum imm64_funct7 {
    srliw_f7 = 0b0000000,
    sraiw_f7 = 0b0100000
  };
  enum reg64_funct73 {
    addw_f = 0b0000000000,
    subw_f = 0b0100000000,
    sllw_f = 0b0000000001,
    srlw_f = 0b0000000101,
    sraw_f = 0b0100000101
  };
}

#endif