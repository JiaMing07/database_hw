`include "../define.svh"
module mem_master #(
    DATA_WIDTH = 32,
    ADDR_WIDTH = 32
) (
    input wire clk,
    input wire rst,

    // wishbone master 请求 写入 or 读取
    input wire wb_ack_i,
    input wire [31:0] wb_dat_i,

    output reg wb_cyc_o,
    output reg wb_stb_o,
    output reg [31:0] wb_adr_o,
    output reg [31:0] wb_dat_o,
    output reg [3:0] wb_sel_o,
    output reg wb_we_o,
    
    // mem signal and reg
    input wire [DATA_WIDTH-1:0] data_write,
    input wire [ADDR_WIDTH-1:0] addr,

    input wire mem_write,
    input wire mem_read,
    input wire [3:0] wb_sel_i,

    output reg [31:0] data_read,
    output reg mem_master_ready
);
    typedef enum logic [1:0] {
        STATE_IDLE,
        STATE_READ_DATA,
        STATE_WRITE_DATA
    } state_t;
    state_t state;

    always_ff @( posedge clk or posedge rst ) begin 
        if(rst) begin
            state <= STATE_IDLE;
            wb_cyc_o <= 1'b0;
            wb_stb_o <= 1'b0;
            wb_adr_o <= 32'h0000_0000;
            wb_dat_o <= 32'h0000_0000;
            wb_sel_o <= 4'b0000;
            wb_we_o <= 1'b0;
            data_read <= 0;
            mem_master_ready <= 1'b1;
        end else begin
            case (state)
                STATE_IDLE : begin
                    if(mem_read) begin
                        state <= STATE_READ_DATA;
                        wb_cyc_o <= 1'b1;
                        wb_stb_o <= 1'b1;
                        wb_adr_o <= addr;
                        wb_sel_o <= (wb_sel_i << (addr & 2'b11));
                        wb_we_o <= 1'b0;
                        mem_master_ready <= 1'b0;
                    end else if(mem_write) begin
                        state <= STATE_WRITE_DATA;
                        wb_cyc_o <= 1'b1;
                        wb_stb_o <= 1'b1;
                        wb_adr_o <= addr;
                        wb_dat_o <= data_write;
                        wb_sel_o <= (wb_sel_i << (addr & 2'b11));
                        wb_we_o <= 1'b1;
                        mem_master_ready <= 1'b0;
                    end else begin
                        state <= STATE_IDLE;
                        wb_cyc_o <= 1'b0;
                        wb_stb_o <= 1'b0;
                        wb_we_o <= 1'b0;
                        mem_master_ready <= 1'b1;
                    end
                end
                STATE_READ_DATA : begin
                    if(wb_ack_i) begin
                        state <= STATE_IDLE;
                        wb_cyc_o <= 1'b0;
                        wb_stb_o <= 1'b0;
                        wb_we_o <= 1'b0;
                        mem_master_ready <= 1'b1;
                        if(wb_sel_o == 4'b0001) begin
                            data_read <= {24'b0, wb_dat_i[7:0]};
                        end else if(wb_sel_o == 4'b0010) begin
                            data_read <= {24'b0, wb_dat_i[15:8]};
                        end else if(wb_sel_o == 4'b0100) begin
                            data_read <= {24'b0, wb_dat_i[23:16]};
                        end else if(wb_sel_o == 4'b1000) begin
                            data_read <= {24'b0, wb_dat_i[31:24]};
                        end else if(wb_sel_o == 4'b1111) begin
                            data_read <= wb_dat_i;
                        end else begin
                            data_read <= 0;
                        end
                    end
                end
                STATE_WRITE_DATA : begin
                    if(wb_ack_i) begin
                        state <= STATE_IDLE;
                        wb_cyc_o <= 1'b0;
                        wb_stb_o <= 1'b0;
                        wb_we_o <= 1'b0;
                        mem_master_ready <= 1'b1;
                    end
                end
            endcase
        end
    end
endmodule