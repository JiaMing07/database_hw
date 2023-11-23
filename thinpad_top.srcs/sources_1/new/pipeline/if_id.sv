`include "../define.svh"

module if_id(
    input wire clk,
    input wire rst,
    input wire pipeline_stall,
    input wire bubble_i,
    input wire stall_i,

    input wire [31:0] if_pc,
    input wire [31:0] if_inst,
    input wire if_branch_or_4,

    output reg [31:0] id_pc,
    output reg [31:0] id_inst,
    output reg id_branch_or_4
);

    always_ff @ (posedge clk) begin
        if (rst) begin
            id_pc <= '0;
            id_inst <= `NOP_INST;
            id_branch_or_4 <= '0;
        end else if (~pipeline_stall) begin
            if (bubble_i) begin
                id_pc <= '0;
                id_inst <= `NOP_INST;
                id_branch_or_4 <= '0;
            end else if (~stall_i) begin
                id_pc <= if_pc;
                id_inst <= if_inst;
                id_branch_or_4 <= if_branch_or_4;
            end
        end
    end

endmodule