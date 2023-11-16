`include "../define.svh"

module imm_gen(
    input wire [31:0] inst,
    output reg [31:0] imm
);
    // jal 指令本身提供 20 位立即数，末尾添一位 0， 共 21 位；
    // SB 类型指令提供 12 位立即数，但首位弃去不用，末尾补一位 0
    // 其他类型指令提供 12 位立即数，全部使用
    // jal 符号扩展前需 11 位
    logic [10:0] sign_ext_jal;
    assign sign_ext_jal = inst[31] ? 11'h7ff : 11'h0;

    // 其他指令符号扩展需 20 位
    logic [19:0] sign_ext;
    assign sign_ext = inst[31] ? 20'hfffff : 20'h0;

    always_comb begin
        case (inst[6:0])
            `OPCODE_I, `OPCODE_L, `OPCODE_JALR:
                imm = {sign_ext, inst[31:20]};
            `OPCODE_S:
                imm = {sign_ext, inst[31:25], inst[11:7]};
            `OPCODE_SB:
                imm = {sign_ext, inst[7], inst[30:25], inst[11:8], 1'b0};
            `OPCODE_LUI, `OPCODE_AUIPC:
                imm = {inst[31:12], 12'b0};
            `OPCODE_JAL:
                imm = {sign_ext_jal, inst[31], inst[19:12], inst[20], inst[30:21], 1'b0};
            default:
                imm = 32'b0;
        endcase
    end

endmodule