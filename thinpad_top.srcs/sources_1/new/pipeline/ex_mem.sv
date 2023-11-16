`include "../define.svh"

module ex_mem(
    input wire clk,
    input wire rst,
    input wire pipeline_stall,

    input wire [31:0] alu_res,
    input wire [31:0] ex_wdata_wishbone,

    // regfile signal
    input wire ex_reg_write,
    input wire [4:0] ex_waddr,
    input wire ex_mem_to_reg,

    // memory signal
    input wire ex_mem_write,
    input wire ex_mem_read,
    input wire [3:0] ex_wb_sel,

    // memory write data and addr
    output reg [31:0] mem_alu_res,
    output reg [31:0] mem_wdata_d,

    // regfile signal
    output reg mem_reg_write,
    output reg mem_mem_to_reg,
    output reg [4:0] mem_waddr,

    // memory signal
    output reg mem_mem_write,
    output reg mem_mem_read,
    output reg [3:0] mem_wb_sel
);

    always_ff @ (posedge clk) begin
        if (rst) begin
            mem_alu_res <= 0;
            mem_wdata_d <= 0;

            mem_reg_write <= 1'b0;
            mem_mem_to_reg <= 1'b0;
            mem_waddr <= 5'b00000;

            mem_mem_write <= 1'b0;
            mem_mem_read <= 1'b0;
            mem_wb_sel <= 4'b0000;
        end else if (!pipeline_stall) begin
            mem_alu_res <= alu_res;
            mem_wdata_d <= ex_wdata_wishbone;

            mem_reg_write <= ex_reg_write;
            mem_mem_to_reg <= ex_mem_to_reg;
            mem_waddr <= ex_waddr;

            mem_mem_write <= ex_mem_write;
            mem_mem_read <= ex_mem_read;
            mem_wb_sel <= ex_wb_sel;
        end
    end

endmodule