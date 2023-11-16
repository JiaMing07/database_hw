
`define FORWARD_SELECT_MEM 2'b10
`define FORWARD_SELECT_WB 2'b01
`define FORWARD_SELECT_EX 2'b00

// instructions
`define OPCODE_R 7'b0110011 // R-type add sub sll srl sra and or xor 
`define OPCODE_I 7'b0010011 // I-type addi slli srli srai andi ori xori
`define OPCODE_L 7'b0000011 // L-type lb 
`define OPCODE_S 7'b0100011 // S-type sb
`define OPCODE_SB 7'b1100011 // SB-type beq bne
`define OPCODE_LUI 7'b0110111 // lui
`define OPCODE_AUIPC 7'b0010111 // auipc  
`define OPCODE_JAL 7'b1101111 // jal
`define OPCODE_JALR 7'b1100111 // jalr
`define OPCODE_NOP 7'b0000000 // nop  

`define OP_ADD 4'b0001
`define OP_SUB 4'b0010
`define OP_AND 4'b0011
`define OP_OR 4'b0100
`define OP_XOR 4'b0101
`define OP_NOT 4'b0110
`define OP_SLL 4'b0111
`define OP_SLR 4'b1000
`define OP_SRA 4'b1001
`define OP_ROL 4'b1010
`define OP_SLT 4'b1011 // SLT 指令没有实现
`define OP_SEL_B 4'b1100
`define OP_ADD_4 4'b1101

`define ALU_SELECT_DATA_B 1'b0
`define ALU_SELECT_IMM 1'b1

`define NOP_INST 32'h0000_0013 

`define ALU_SEL_A 1'b0
`define ALU_SEL_PC 1'b1

`define ALU_SEL_B 1'b0
`define ALU_SEL_IMM 1'b1