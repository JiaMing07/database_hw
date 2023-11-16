`include "../define.svh"
module alu #(
parameter int N=16
)(
    input wire [31:0] rs1,
    input wire [31:0] rs2,
    input wire [3:0] opcode,
    output wire [31:0] ans
);
    logic [31:0] a;
    always_comb begin
        case (opcode)
            `OP_ADD: a = rs1 + rs2; // ADD
            `OP_SUB: a = rs1 - rs2; // SUB
            `OP_AND: a = rs1 & rs2; // AND
            `OP_OR: a = rs1 | rs2; // OR
            `OP_XOR: a = rs1 ^ rs2; // XOR
            `OP_NOT: a = ~rs1; //NOT
            `OP_SLL: a = rs1 << (rs2 & (N-1)); // SLL
            `OP_SLR: a = rs1 >> (rs2 & (N-1)); // SLR 
            `OP_SRA: a = $signed(rs1) >>> (rs2 & (N-1)); // SRA
            `OP_ROL: a = (rs1 << (rs2 & (N-1))) + (rs1 >> (N - (rs2 & (N-1)))); // ROL 循环左移 
            `OP_SEL_B: a = rs2; // SEL_B
            `OP_ADD_4: a = rs1 + 4; // A_ADD_4 处理 pc
            default: a = 16'b0;
        endcase
        
    end
    assign ans = a;

endmodule