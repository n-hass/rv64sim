#pragma once
namespace rv64 {
  // opcode 
  enum opcode {
    lui_op = 0x37,
    auipc_op = 0x17,
    imm_op = 0x13,
    reg_op = 0x33,
    fen_op = 0x0F,
    sys_op = 0x73,
    load_op = 0x03,
    store_op = 0x23,
    jal_op = 0x6F,
    jalr_op = 0x67,
    branch_op = 0x63,
    imm64_op = 0x1B,
    reg64_op = 0x3B,
  };

  /* funct codes.
     funct3 = strictly: funct3
     funct73 = strictly: (funct3 << 7) & funct7

     The individual enum attributes are named with f7/3 when the enum for a single 
     instruction type is split in two (e.g. imm_funct3 for 14:12 of the I-type instructions
      and imm_funct73 for 31:25+14:12 of the I-type instructions, with funct3 concatenated from the LSB of funct7)
  */
  enum imm_funct3 {
    addi_f3 = 0x00,
    slti_f3 = 0x02,
    sltiu_f3 = 0x03,
    xori_f3 = 0x04,
    ori_f3 = 0x06,
    andi_f3 = 0x07,
    slli_f3 = 0x01,
    srli_f3 = 0x05,
    srai_f3 = 0x05, // note is a duplicate of above, is differentiated by funct7 (most significant 7 bits)
  };
  // note: is 31:26 + 14:12, NOT full funct7
  enum imm_funct9 { // this must be split as these are imm instructions (I-format) that don't use 12-bit immediate values and instead emulate the functionality of funct7 and shamt
    slli_f73 = 0x01,
    srli_f73 = 0x05,
    srai_f73 = 0x85
  };
  enum reg_funct73 {
    add_f = 0x00,
    sub_f = 0x100,
    sll_f = 0x01,
    slt_f = 0x02,
    sltu_f = 0x03,
    xor_f = 0x04,
    srl_f = 0x05,
    sra_f = 0x105,
    or_f = 0x06,
    and_f = 0x07
  };
  enum fen_funct3 {
    fence_f = 0x00,
    fence_i_f = 0x01
  };

  enum sys_funct3 {
    priv_f = 0x00,
    csrrw_f = 0x01,
    csrrs_f = 0x02,
    csrrc_f = 0x03,
    csrrwi_f = 0x05,
    csrrsi_f = 0x06,
    csrrci_f = 0x07
  };

  enum priv_rs2 {
    ecall_rs2 = 0x00,
    ebreak_rs2 = 0x01,
    mret_rs2 = 0x02,
  };

  enum load_funct3 {
    lb_f = 0x00,
    lh_f = 0x01,
    lw_f = 0x02,
    ld_f = 0x03,
    lbu_f = 0x04,
    lhu_f = 0x05,
    lwu_f = 0x06,
  };
  enum store_funct3 {
    sb_f = 0x00,
    sh_f = 0x01,
    sw_f = 0x02,
    sd_f = 0x03
  };
  enum branch_funct3 {
    beq_f = 0x00,
    bne_f = 0x01,
    blt_f = 0x04,
    bge_f = 0x05,
    bltu_f = 0x06,
    bgeu_f = 0x07
  };
  enum imm64_funct3 {
    addiw_f3 = 0x00,
    slliw_f3 = 0x01,
    srliw_f3 = 0x05,
    sraiw_f3 = 0x05, 
  };
  enum imm64_funct7 {
    srliw_f7 = 0x00,
    sraiw_f7 = 0x20
  };
  enum reg64_funct73 {
    addw_f = 0x00,
    subw_f = 0x100,
    sllw_f = 0x01,
    srlw_f = 0x05,
    sraw_f = 0x105
  };

  enum except {
    pc_misaligned = 0,
    illegal_instruction = 2,
    breakpoint = 3,
    load_address_misaligned = 4,
    store_address_misaligned = 6,
    ecall_from_u = 8,
    ecall_from_m = 11
  };

  enum interrupt {
    user_software = 0,
    machine_software = 3,
    user_timer = 4,
    machine_timer = 7,
    user_external = 8,
    machine_external = 11
  };

  enum csr {
    mvendorid = 0xF11,
    marchid = 0xF12,
    mimpid = 0xF13,
    mhartid = 0xF14,
    mstatus = 0x300,
    misa = 0x301,
    mie = 0x304,
    mtvec = 0x305,
    mscratch = 0x340,
    mepc = 0x341,
    mcause = 0x342,
    mtval = 0x343,
    mip = 0x344
  };
}