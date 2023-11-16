`include "../define.svh"
module forward #(
    REG_WIDTH = 5,
    FORWARD_SELECT = 2
) (
    input wire [REG_WIDTH-1:0] ex_rs1,
    input wire [REG_WIDTH-1:0] ex_rs2,

    // EX 冒险
    input wire [REG_WIDTH-1:0] mem_rd,
    input wire mem_reg_write,

    // MEM 冒险
    input wire [REG_WIDTH-1:0] wb_rd,
    input wire wb_reg_write,

    output reg [FORWARD_SELECT-1:0] forward_a,
    output reg [FORWARD_SELECT-1:0] forward_b
);
    logic mem_forward;
    logic wb_forward;
    assign mem_forward = mem_reg_write & (mem_rd != 0) & (mem_rd == ex_rs1);
    assign wb_forward = mem_reg_write && (mem_rd != 0) && (mem_rd == ex_rs2);
    always_comb begin 
        if (mem_reg_write & (mem_rd != 0) & (mem_rd == ex_rs1)) begin 
            forward_a = `FORWARD_SELECT_MEM; // select from EX/MEM 产生 EX 冒险
        end else begin
            if(wb_reg_write && (wb_rd != 0) && (wb_rd == ex_rs1)) begin 
                forward_a = `FORWARD_SELECT_WB; // select from MEM/WB 产生 MEM 冒险
            end else begin
                forward_a = `FORWARD_SELECT_EX;
            end
        end
    end

    always_comb begin
        if(mem_reg_write && (mem_rd != 0) && (mem_rd == ex_rs2)) begin 
            forward_b = `FORWARD_SELECT_MEM; // select from EX/MEM 产生 EX 冒险
        end else begin
            if(wb_reg_write && (wb_rd != 0) && (wb_rd == ex_rs2)) begin 
                forward_b = `FORWARD_SELECT_WB; // select from MEM/WB 产生 MEM 冒险
            end else begin
                forward_b = `FORWARD_SELECT_EX;
            end
        end
    end
endmodule
