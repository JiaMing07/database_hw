`include "../define.svh"

module branch_compare #(
    DATA_WIDTH = 32,
    ADDR_WIDTH = 32
) (
    input wire [DATA_WIDTH-1:0] data_a,
    input wire [DATA_WIDTH-1:0] data_b,
    input wire [DATA_WIDTH-1:0] pc,
    input wire [DATA_WIDTH-1:0] imm,
    input wire [DATA_WIDTH-1:0] inst,

    output reg branch,
    output reg [DATA_WIDTH-1:0] pc_branch
);
    logic [6:0] opcode;
    assign opcode = inst[6:0];

    always_comb begin
        case (opcode)
            `OPCODE_SB: begin
                case (inst[14:12])
                    3'b000: begin // beq
                        branch = (data_a == data_b);
                    end
                    3'b001: begin // bne
                        branch = (data_a != data_b);
                    end
                    default: begin
                        branch = 1'b0;
                    end
                endcase
            end
            `OPCODE_JAL, `OPCODE_JALR: begin
                branch = 1'b1;
            end
            default: begin
                branch = 1'b0;
            end
        endcase
    end

    assign pc_branch = (opcode == `OPCODE_JALR) ? ((data_a + imm) & 32'hffff_fffe) : (pc + imm);
endmodule