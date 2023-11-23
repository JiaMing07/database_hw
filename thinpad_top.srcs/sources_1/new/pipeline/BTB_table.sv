`include "../define.svh"

module btb_table (
    input wire clk,
    input wire rst,
    input wire [31:0] pc_write,
    input wire state_write,
    input wire [31:0] pc_branch_write,
    input wire we,
    input wire [31:0] pc_read,
    
    output reg [31:0] pc_next,
    output reg branch_or_4_o
);
    logic state_bit;
    logic [31:0] pc_compare;
    logic [31:0] pc_branch;
    logic [31:0] pc_4;

    always_ff @(posedge clk or posedge rst) begin
        if(rst) begin
            state_bit <= 1'b0;
            pc_compare <= '0;
            pc_branch <= '0;
            pc_4 <= '0;
        end else begin
            if (we) begin
                pc_compare <= pc_write;
                state_bit <= state_write;
                pc_branch <= pc_branch_write;
                pc_4 <= pc_write + 4;
            end
        end
    end
    
    always_comb begin
        if (pc_read == pc_compare) begin
            pc_next = state_bit ? pc_branch : pc_4;
            branch_or_4_o = state_bit ? 1'b1 : 1'b0;
        end else begin
            pc_next = pc_read + 4;
            branch_or_4_o = 1'b0;
        end
    end
endmodule