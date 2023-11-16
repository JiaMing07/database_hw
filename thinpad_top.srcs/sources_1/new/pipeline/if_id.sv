`include "../define.svh"

module if_id(
    input wire clk,
    input wire rst,
    input wire pipeline_stall,
    input wire bubble_i,
    input wire stall_i,

    input wire [31:0] if_pc,
    input wire [31:0] if_inst,

    output reg [31:0] id_pc,
    output reg [31:0] id_inst
);

    always_ff @ (posedge clk) begin
        if (rst) begin
            id_pc <= '0;
            id_inst <= `NOP_INST;
        end else if (~pipeline_stall) begin
            if (bubble_i) begin
                id_pc <= '0;
                id_inst <= `NOP_INST;
            end else if (~stall_i) begin
                id_pc <= if_pc;
                id_inst <= if_inst;
            end
        end
    end

endmodule