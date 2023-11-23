`include "../define.svh"

module jump (
    input wire branch,
    input wire id_branch_or_4,
    input wire [31:0] pc_branch,
    input wire [31:0] id_pc,

    output reg jump,
    output reg [31:0] jump_pc
);
    logic jump_flag;
    assign jump_flag = branch ^ id_branch_or_4;
    always_comb begin
        if(jump_flag) begin
            jump = 1'b1;
            jump_pc = branch ? pc_branch : (id_pc + 4);
        end else begin
            jump = 1'b0;
            jump_pc = id_pc + 4;
        end
    end
endmodule