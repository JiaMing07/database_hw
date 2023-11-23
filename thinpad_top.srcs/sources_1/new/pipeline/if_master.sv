`include "../define.svh"

// read pc from IM
module if_master #(
    ADDR_WIDTH = 32,
    DATA_WIDTH = 32
) (
    input wire clk,
    input wire rst,

    input wire pipeline_stall,
    input wire stall_i,
    // branch
    input wire jump,
    input wire [ADDR_WIDTH-1:0] pc_jump,
    input wire [ADDR_WIDTH-1:0] pc_next,

    // wishbone master 请求 instr
    input wire wb_ack_i,
    input wire [31:0] wb_dat_i,

    output reg wb_cyc_o,
    output reg wb_stb_o,
    output reg [31:0] wb_adr_o,
    output reg [31:0] wb_dat_o,
    output reg [3:0] wb_sel_o,
    output reg wb_we_o,

    // if reg & signal
    output reg [31:0] inst,
    output reg [31:0] pc,
    output reg if_master_ready,

    output reg [31:0] reg_pc
);
    // 保存当前指令�? PC�?
    typedef enum logic [1:0] { 
        STATE_IDLE, 
        STATE_READ_DATA,
        STATE_READ_DONE
    } state_t;

    state_t state;

    logic [31:0] last_pc;

    always_ff @ (posedge clk or posedge rst) begin
        if(rst) begin
            state <= STATE_IDLE;
            wb_cyc_o <= 1'b0;
            wb_stb_o <= 1'b0;
            wb_adr_o <= 32'h0000_0000;
            wb_dat_o <= 32'h0000_0000;
            wb_we_o <= 1'b0;
            wb_sel_o <= 4'b0000;
            inst <= 32'h0000_0000;
            pc <= 32'h8000_0000;
            reg_pc <= 32'h8000_0000 - 4;
            if_master_ready <= 1'b1;
        end else begin
            case (state)
                STATE_IDLE: begin
                    if(!pipeline_stall) begin
                        wb_cyc_o <= 1'b1;
                        wb_stb_o <= 1'b1;
                        if(jump) begin
                            wb_adr_o <= pc_jump;
                            reg_pc <= pc_jump;
                        end else begin
                            wb_adr_o <= pc_next;
                            reg_pc <= pc_next;
                        end
                        last_pc <= reg_pc;
                        wb_sel_o <= 4'b1111;
                        wb_we_o <= 1'b0;
                        if_master_ready <= 1'b0;
                        state <= STATE_READ_DATA;
                    end
                end
                STATE_READ_DATA: begin
                    if(wb_ack_i) begin
                        state <= STATE_IDLE;
                        if_master_ready <= 1'b1;
                        wb_cyc_o <= 1'b0;
                        wb_stb_o <= 1'b0;
                        inst <= wb_dat_i;
                        pc <= reg_pc;
                        if(stall_i) begin
                            reg_pc <= last_pc;
                        end
                    end
                end
                // STATE_READ_DONE : begin
                //     state <= STATE_IDLE;
                //     if_master_ready <= 1'b1;
                // end
            endcase
        end
    end
endmodule