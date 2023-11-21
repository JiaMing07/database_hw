`include "../define.svh"

// read or write data with wishbone in MEM
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
    typedef enum logic [2:0] {
        STATE_IDLE,
        STATE_READ_DATA,
        STATE_WRITE_DATA,
        STATE_READ_DONE,
        STATE_WRITE_DONE
    } state_t;
    state_t state;

    logic same;
    logic [DATA_WIDTH-1:0] reg_data_write;
    logic [ADDR_WIDTH-1:0] reg_addr;
    logic reg_mem_write;
    logic reg_mem_read;
    logic [3:0] reg_sel_i;

    // 不执行两次相同的命令，去除了也没有错
    assign same  = ((reg_data_write == data_write) && (reg_addr == addr) && (reg_mem_read == mem_read) && (reg_mem_write == mem_write) && (reg_sel_i == wb_sel_i));

    logic [1:0] cnt;
    
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
            reg_data_write <= 0;
            reg_addr <= 0;
            reg_mem_read <= 0;
            reg_mem_write <= 0;
            reg_sel_i <= 0;
            cnt <= 0;
        end else begin
            case (state)
                STATE_IDLE : begin
                    if(mem_read && ~same) begin
                        state <= STATE_READ_DATA;
                        reg_data_write <= data_write;
                        reg_addr <= addr;
                        reg_mem_read <= mem_read;
                        reg_mem_write <= mem_write;
                        reg_sel_i <= wb_sel_i;
                        wb_cyc_o <= 1'b1;
                        wb_stb_o <= 1'b1;
                        wb_adr_o <= addr;
                        wb_sel_o <= (wb_sel_i << (addr & 2'b11));
                        wb_we_o <= 1'b0;
                        mem_master_ready <= 1'b0;
                    end else if(mem_write && ~same) begin
                        state <= STATE_WRITE_DATA;
                        reg_data_write <= data_write;
                        reg_addr <= addr;
                        reg_mem_read <= mem_read;
                        reg_mem_write <= mem_write;
                        reg_sel_i <= wb_sel_i;
                        wb_cyc_o <= 1'b1;
                        wb_stb_o <= 1'b1;
                        wb_adr_o <= addr;
                        wb_dat_o <= data_write;
                        wb_sel_o <= (wb_sel_i << (addr & 2'b11));
                        wb_we_o <= 1'b1;
                        mem_master_ready <= 1'b0;
                    end else begin
                        reg_data_write <= data_write;
                        reg_addr <= addr;
                        reg_mem_read <= mem_read;
                        reg_mem_write <= mem_write;
                        reg_sel_i <= wb_sel_i;
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
                        mem_master_ready <= 1'b1;
                        wb_cyc_o <= 1'b0;
                        wb_stb_o <= 1'b0;
                        wb_we_o <= 1'b0;
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
                // STATE_READ_DONE : begin
                //     state <= STATE_IDLE;
                //     mem_master_ready <= 1'b1;
                // end
                // STATE_WRITE_DONE : begin
                //     state <= STATE_IDLE;
                //     mem_master_ready <= 1'b1;
                // end
            endcase
        end
    end
endmodule