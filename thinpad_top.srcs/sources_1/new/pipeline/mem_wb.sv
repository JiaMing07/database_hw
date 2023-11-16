`include "../define.svh"

module mem_wb(
    input wire clk,
    input wire rst,
    input wire pipeline_stall,

    input wire mem_reg_write,
    input wire [4:0] mem_waddr,
    input wire [31:0] mem_wdata,

    output reg wb_reg_write,
    output reg [4:0] wb_waddr,
    output reg [31:0] wb_wdata
);

    always_ff @ (posedge clk) begin
        if (rst) begin
            wb_reg_write <= 1'b0;
            wb_waddr <= 0;
            wb_wdata <= 0;
        end else if (!pipeline_stall) begin
            wb_reg_write <= mem_reg_write;
            wb_waddr <= mem_waddr;
            wb_wdata <= mem_wdata;
        end
    end

endmodule