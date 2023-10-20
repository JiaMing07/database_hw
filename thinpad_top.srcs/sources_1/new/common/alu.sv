module alu #(
parameter int N=16
)(
    input wire [15:0] rs1,
    input wire [15:0] rs2,
    input wire [3:0] opcode,
    output wire [15:0] ans
);
    logic [15:0] a;
    always_comb begin
        case (opcode)
            4'b0001: a = rs1 + rs2; // ADD
            4'b0010: a = rs1 - rs2; // SUB
            4'b0011: a = rs1 & rs2; // AND
            4'b0100: a = rs1 | rs2; // OR
            4'b0101: a = rs1 ^ rs2; // XOR
            4'b0110: a = ~rs1; //NOT
            4'b0111: a = rs1 << (rs2 & (N-1)); // SLL
            4'b1000: a = rs1 >> (rs2 & (N-1)); // SLR 
            4'b1001: a = $signed(rs1) >>> (rs2 & (N-1)); // SRA
            4'b1010: a = (rs1 << (rs2 & (N-1))) + (rs1 >> (N - (rs2 & (N-1)))); // ROL 循环左移 
            default: a = 16'b0;
        endcase
        
    end
    assign ans = a;

endmodule