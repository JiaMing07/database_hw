`include "../define.svh"

module select_data (
    input wire [1:0] forward,
    input wire [31:0] ex_data,
    input wire [31:0] mem_alu_res,
    input wire [31:0] wb_wdata,
    output reg [31:0] data
);
    always_comb begin 
        case (forward)
            `FORWARD_SELECT_EX: data = ex_data;
            `FORWARD_SELECT_MEM: data = mem_alu_res;
            `FORWARD_SELECT_WB: data = wb_wdata; 
            default: data = 0;
        endcase
    end
endmodule