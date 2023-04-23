#ifndef INSTRUCTION_DEFS_H
#define INSTRUCTION_DEFS_H
namespace rv64 {
  // opcode 
  enum opcode {
    nop_op = 0b0000000,
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

  // funct codes, equiv to (funct3 << 7) & funct7 or just funct3
  enum load_funct {
    lb_f = 0b000,
    lh_f = 0b001,
    lw_f = 0b010,
    ld_f = 0b011,
    lbu_f = 0b100,
    lhu_f = 0b101,
    lwu_f = 0b110,
  };
  enum fen_funct {
    fence_f = 0b000,
    fence_i_f = 0b001
  };
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
  enum imm_funct73 { // this must be split as these are imm instructions (I-format) that don't use 12-bit immediate values and instead emulate the functionality of funct7 and shamt
    slli_f73 = 0b001000000,
    srli_f73 = 0b101000000,
    srai_f73 = 0b101010000
  };
  enum reg_funct {
    add_f = 0b0000000000,
    sub_f = 0b0000100000,
    sll_f = 0b0010000000,
    slt_f = 0b0100000000,
    sltu_f = 0b0110000000,
    xor_f = 0b1000000000,
    srl_f = 0b1010000000,
    sra_f = 0b1010100000,
    or_f = 0b1100000000,
    and_f = 0b1110000000
  };
  enum branch_funct {
    beq_f = 0b000,
    bne_f = 0b001,
    blt_f = 0b100,
    bge_f = 0b101,
    bltu_f = 0b110,
    bgeu_f = 0b111
  };
  enum imm64_funct {
    addiw_f = 0b000,
    slliw_f = 0b001,
    slliw10_f = 0b0010000000,
    srliw_f = 0b101,
    srliw10_f = 0b1010000000,
    sraiw_f = 0b101,
    sraiw10_f = 0b1010100000
  };
  enum reg64_funct {
    addw_f = 0b0000000000,
    subw_f = 0b0000100000,
    sllw_f = 0b0010000000,
    srlw_f = 0b1010000000,
    sraw_f = 0b1010100000
  };
  enum store_funct {
    sb_f = 0b000,
    sh_f = 0b001,
    sw_f = 0b010,
    sd_f = 0b011
  };
  enum sys_funct {
    ecall_f = 0b000000000000,
    ebreak_f = 0b000000000001
  };
}

#endif