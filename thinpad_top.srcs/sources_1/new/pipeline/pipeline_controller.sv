`include "../define.svh"
module pipeline_controller #(
    parameter DATA_WIDTH = 32,
    parameter ADDR_WIDTH = 32
) (
    input wire [31:0] inst,

    output reg [3:0] alu_op,
    output reg alu_sel_pc_a,
    output reg alu_sel_imm_b,

    output reg mem_read,
    output reg mem_write,
    output reg [3:0] wb_sel, 

    output reg reg_write,
    output reg mem_to_reg
);
    logic [6:0] opcode;
    assign opcode= inst[6:0];

    logic [4:0] rs1;
    assign rs1 = inst[19:15];

    logic  [4:0] rs2;
    assign rs2 = inst[24:20];

    logic [2:0] func3;
    assign func3 = inst[14:12];

    logic [6:0] func7;
    assign func7 = inst[31:25];

    // alu
    always_comb begin 
        case (opcode)
            `OPCODE_R: begin
                case (func3)
                    3'b000: begin
                        if(func7 == 7'b0000000) begin
                            alu_op = `OP_ADD;
                        end else begin
                            alu_op = `OP_SUB;
                        end
                    end 
                    3'b001: begin
                        alu_op = `OP_SLL;
                    end
                    3'b010, 3'b011: begin
                        alu_op = `OP_SLT;
                    end
                    3'b100: begin
                        alu_op = `OP_XOR;
                    end
                    3'b101: begin
                        if(func7 == 7'b0000000) begin
                            alu_op = `OP_SLR;
                        end else begin
                            alu_op = `OP_SRA;
                        end
                    end
                    3'b110: begin
                        alu_op = `OP_OR;
                    end
                    3'b111: begin
                        alu_op = `OP_AND;
                    end
                    default: alu_op = `OP_ADD;
                endcase
            end 
            `OPCODE_I: begin
                case (func3)
                    3'b000: alu_op = `OP_ADD;
                    3'b010, 3'b011: alu_op=`OP_SLT;
                    3'b100: alu_op = `OP_XOR;
                    3'b110: alu_op = `OP_OR;
                    3'b111: alu_op = `OP_AND; 
                    default: alu_op = `OP_ADD;
                endcase
            end
            `OPCODE_LUI: alu_op = `OP_SEL_B;
            `OPCODE_JAL, `OPCODE_JALR: alu_op = `OP_ADD_4; 
            default: begin
                alu_op = 4'b0001;
            end
        endcase
    end

    assign alu_sel_pc_a = (opcode == `OPCODE_AUIPC || opcode == `OPCODE_JAL || opcode == `OPCODE_JALR) ? `ALU_SEL_PC : `ALU_SEL_A;
    assign alu_sel_imm_b = (opcode == `OPCODE_R) ? `ALU_SEL_B : `ALU_SEL_IMM;

    assign mem_read = (opcode == `OPCODE_L)?(1'b1):(1'b0);
    assign mem_write = (opcode == `OPCODE_S)?(1'b1):(1'b0);

    always_comb begin
        case (func3)
            3'b000: wb_sel = 4'b0001;
            3'b100: wb_sel = 4'b0001;
            3'b001: wb_sel = 4'b0011;
            3'b101: wb_sel = 4'b0011;
            3'b010: wb_sel = 4'b1111;
            default: wb_sel = 4'b0000;
        endcase
    end

    assign reg_write = ((opcode == `OPCODE_S) || (opcode == `OPCODE_SB)) ? 1'b0 : 1'b1;
    assign mem_to_reg = (opcode == `OPCODE_L);

endmodule