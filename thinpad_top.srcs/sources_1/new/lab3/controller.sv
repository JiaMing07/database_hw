module controller (
    input wire clk,
    input wire reset,

    // 连接寄存器堆模块的信�?
    output reg  [4:0]  rf_raddr_a,
    input  wire [15:0] rf_rdata_a,
    output reg  [4:0]  rf_raddr_b,
    input  wire [15:0] rf_rdata_b,
    output reg  [4:0]  rf_waddr,
    output reg  [15:0] rf_wdata,
    output reg  rf_we,

    // 连接 ALU 模块的信�?
    output reg  [15:0] alu_a,
    output reg  [15:0] alu_b,
    output reg  [ 3:0] alu_op,
    input  wire [15:0] alu_y,

    // 控制信号
    input  wire        step,    // 用户按键状态脉�?
    input  wire [31:0] dip_sw,  // 32 位拨码开关状�?
    output reg  [15:0] leds
);

    logic [31:0] inst_reg;  // 指令寄存�?

    // 组合逻辑，解析指令中的常用部分，依赖于有效的 inst_reg �?
    logic is_rtype, is_itype, is_peek, is_poke;
    logic [15:0] imm;
    logic [4:0] rd, rs1, rs2;
    logic [3:0] opcode;

    always_comb begin
        is_rtype = (inst_reg[2:0] == 3'b001);
        is_itype = (inst_reg[2:0] == 3'b010);
        is_peek = is_itype && (inst_reg[6:3] == 4'b0010);
        is_poke = is_itype && (inst_reg[6:3] == 4'b0001);

        imm = inst_reg[31:16];
        rd = inst_reg[11:7];
        rs1 = inst_reg[19:15];
        rs2 = inst_reg[24:20];
        opcode = inst_reg[6:3];
    end

    // 使用枚举定义状态列表，数据类型 logic [3:0]
    typedef enum logic [3:0] {
        ST_INIT,
        ST_DECODE,
        ST_CALC,
        ST_READ_REG,
        ST_WRITE_REG
    } state_t;

    // 状态机当前状态寄存器
    state_t current_state;
    state_t next_state;

    // 状态机逻辑
    // 1. 状态转移
    always_ff @ (posedge clk) begin 
        if(reset) begin 
            current_state <= ST_INIT;
        end else begin 
            current_state <= next_state;
        end
    end

    // 2. 状态转移条件
    always_comb begin
        case(current_state)
            ST_INIT: begin
                if (step) begin
                    next_state <= ST_DECODE;
                end else begin 
                    next_state <= ST_INIT;
                end
            end
            ST_DECODE: begin 
                if (is_rtype) begin 
                    next_state <= ST_CALC;
                end else if(is_peek) begin 
                    next_state <= ST_READ_REG;
                end else if(is_poke) begin 
                    next_state <= ST_WRITE_REG;
                end else begin 
                    next_state <= ST_INIT;
                end
            end
            ST_CALC : begin 
                next_state <= ST_WRITE_REG;
            end
            ST_READ_REG : begin 
                next_state <= ST_INIT;
            end
            ST_WRITE_REG : begin 
                next_state <= ST_INIT;
            end
            default: next_state = ST_INIT;
        endcase
    end

    // 3. 描述状态输入
    always_ff @ (posedge clk) begin 
        if(reset) begin 
            inst_reg <= 32'b0;
            rf_raddr_a <= 0;
            rf_raddr_b <= 0;
            rf_waddr <= 0;
            rf_wdata <= 0;
            rf_we <= 0;
            alu_a <= 0;
            alu_b <= 0;
            alu_op <= 0;
            leds <= 0;
        end else begin 
            case(current_state)
                ST_INIT : begin
                    inst_reg <= dip_sw;
                end
                ST_DECODE : begin 
                    if (is_rtype) begin 
                        // 若为算术计算指令，则读入两个操作数
                        rf_raddr_a <= rs1;
                        rf_raddr_b <= rs2;
                        rf_we <= 1'b0;
                    end else if (is_peek) begin 
                        // 若为读指令，则读入寄存器内的数
                        rf_raddr_a <= rd;
                        rf_we <= 1'b0;
                    end
                end
                ST_CALC : begin 
                    // 将数据传递给 ALU
                    alu_a <= rf_rdata_a;
                    alu_b <= rf_rdata_b;
                    alu_op <= opcode;
                    rf_we <= 1'b0;
                end
                ST_READ_REG : begin 
                    // 将读到的数据显示到LED上
                    leds <= rf_rdata_a;
                    rf_we <= 1'b0;
                end
                ST_WRITE_REG : begin 
                    // 如果是 R-type 就将计算后的结果存进寄存器中；否则就将 imm 存到寄存器中
                    if (is_rtype) begin 
                        rf_wdata <= alu_y;
                    end else begin 
                        rf_wdata <= imm;
                    end
                    rf_waddr <= rd;
                    rf_we <= 1'b1;
                end
                default : begin 
                    leds <= 15'b0;
                    rf_we <= 1'b0;
                end
            endcase
        end
    end
  
endmodule