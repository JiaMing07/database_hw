`include "../define.svh"

module btb_table (
    input wire clk,
    input wire rst,
    input wire [31:0] pc_now,
    input wire state_now,
    input wire [31:0] pc_branch_now,
    input wire we,
    input wire [31:0] pc_read,
    
    output wire [31:0] pc_next,
    output wire branch_or_4
);
    logic state_bit;
    logic [31:0] pc_compare;
    logic [31:0] pc_branch;
    logic [31:0] pc_4;

    always_ff @(posedge clk or posedge rst) begin
        if(clk) begin
            state_bit <= 1'b0;
            pc_compare <= '0;
            pc_branch <= '0;
            pc_4 <- '0;
        end else begin
            if (we) begin
                pc_compare <= pc_now;
                state_bit <= state_now;
                pc_branch <= pc_branch_now;
                pc_4 <= pc_now + 4;
            end
        end
    end
    
    always_comb begin
        if (pc_now == pc_compare) begin
            pc_next = state_bit ? pc_branch : pc_4;
            branch_or_4 = state_bit ? 1'b1 : 1'b0;
        end else begin
            pc_next = pc_now + 4;
            branch_or_4 = 1'b0;
        end
    end
endmodule