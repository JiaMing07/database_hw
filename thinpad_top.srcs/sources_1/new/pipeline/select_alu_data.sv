`include "../define.svh"
// 实际上是一个 3 路选择器
// FORWARD_SELECT_MEM 2'b10
// FORWARD_SELECT_WB 2'b01
// FORWARD_SELECT_EX 2'b00
module select_data (
    input wire [1:0] forward,
    input wire [31:0] ex_data,
    input wire [31:0] mem_alu_res,
    input wire [31:0] wb_wdata,
    output reg [31:0] data
);
    always_comb begin 
        case (forward)
            `FORWARD_SELECT_EX: data = ex_data; //2'b00
            `FORWARD_SELECT_WB: data = wb_wdata; //2'b01
            `FORWARD_SELECT_MEM: data = mem_alu_res; //2'b10
            default: data = 0;
        endcase
    end
endmodule