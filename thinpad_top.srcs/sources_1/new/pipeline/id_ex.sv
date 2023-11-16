`include "../define.svh"

module id_ex(
    input wire clk,
    input wire rst,
    input wire pipeline_stall,
    input wire id_ex_bubble,
    input wire id_ex_stall,

    // pc & inst
    input wire [31:0] id_pc,
    input wire [31:0] id_inst,

    // regfile signal
    input wire id_reg_write,
    input wire id_mem_to_reg,

    // read data
    input wire [31:0] id_data_a,
    input wire [31:0] id_data_b,

    // memory signal
    input wire id_mem_write,
    input wire id_mem_read,
    input wire [3:0] id_wb_sel,

    // alu signal
    input wire [3:0] id_alu_op,
    input wire id_alu_sel_pc_a,
    input wire id_alu_sel_imm_b,

    input wire [31:0] id_imm,

    // output
    // pc & inst
    output reg [31:0] ex_pc,
    output reg [31:0] ex_inst,

    // regfile signal
    output reg ex_reg_write,
    output reg ex_mem_to_reg,

    // regfile waddr
    output reg [4:0] ex_waddr,

    // regfile read data
    output reg [31:0] ex_data_a,
    output reg [31:0] ex_data_b,

    // memory signal
    output reg ex_mem_write,
    output reg ex_mem_read,
    output reg [3:0] ex_wb_sel,

    // alu signal
    output reg [3:0] ex_alu_op,
    output reg ex_alu_sel_imm_b,
    output reg ex_alu_sel_pc_a,

    output reg [4:0] ex_rs1,
    output reg [4:0] ex_rs2,
    output reg [31:0] ex_imm
);

    always_ff @(posedge clk) begin
        if (rst) begin
            ex_pc <= 0;
            ex_inst <= 0;

            ex_reg_write <= 1'b0;
            ex_mem_to_reg <= 1'b0;

            ex_waddr <= 0;
            ex_data_a <= 0;
            ex_data_b <= 0;

            ex_mem_write <= 1'b0;
            ex_mem_read <= 1'b0;
            ex_wb_sel <= 0;

            ex_alu_op <= 0;
            ex_alu_sel_pc_a <= 1'b0;
            ex_alu_sel_imm_b <= 1'b0;

            ex_rs1 <= 0;
            ex_rs2 <= 0;
            ex_imm <= 0;
        end else if (!pipeline_stall) begin
            if (id_ex_bubble) begin
                ex_pc <= '0;
                ex_inst <= `NOP_INST;

                ex_reg_write <= 1'b0;
                ex_mem_to_reg <= 1'b0;

                ex_waddr <= 0;
                ex_data_a <= 0;
                ex_data_b <= 0;

                ex_mem_write <= 1'b0;
                ex_mem_read <= 1'b0;
                ex_wb_sel <= 0;

                ex_alu_op <= 0;
                ex_alu_sel_pc_a <= 1'b0;
                ex_alu_sel_imm_b <= 1'b0;

                ex_rs1 <= 0;
                ex_rs2 <= 0;
                ex_imm <= 0;
            end else if (!id_ex_stall)begin
                ex_pc <= id_pc;
                ex_inst <= id_inst;

                ex_reg_write <= id_reg_write;
                ex_mem_to_reg <= id_mem_to_reg;
                ex_waddr <= id_inst[11:7];
                ex_data_a <= id_data_a;
                ex_data_b <= id_data_b;

                ex_mem_write <= id_mem_write;
                ex_mem_read <= id_mem_read;
                ex_wb_sel <= id_wb_sel;

                ex_alu_op <= id_alu_op;
                ex_alu_sel_pc_a <= id_alu_sel_pc_a;
                ex_alu_sel_imm_b <= id_alu_sel_imm_b;

                ex_rs1 <= id_inst[19:15];
                ex_rs2 <= id_inst[24:20];
                ex_imm <= id_imm;
            end
        end
    end

endmodule