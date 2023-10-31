module lab5_master #(
    parameter ADDR_WIDTH = 32,
    parameter DATA_WIDTH = 32
) (
    input wire clk_i,
    input wire rst_i,

    // TODO: 添加�???要的控制信号，例如按键开关？
    input wire [31:0] addr_i,

    // wishbone master
    output reg wb_cyc_o,
    output reg wb_stb_o,
    input wire wb_ack_i,
    output reg [ADDR_WIDTH-1:0] wb_adr_o,
    output reg [DATA_WIDTH-1:0] wb_dat_o,
    input wire [DATA_WIDTH-1:0] wb_dat_i,
    output reg [DATA_WIDTH/8-1:0] wb_sel_o,
    output reg wb_we_o
    
);

  // TODO: 实现实验 5 的内�???+串口 Master
  typedef enum logic [4:0] {
        STATE_IDLE = 0,
        STATE_READ_WAIT_ACTION = 1,
        STATE_READ_WAIT_CHECK = 2,
        STATE_READ_DATA_ACTION = 3,
        STATE_READ_DATA_DONE = 4,
        STATE_WRITE_SRAM_ACTION = 5,
        STATE_WRITE_SRAM_DONE = 6,
        STATE_WRITE_WAIT_ACTION = 7,
        STATE_WRITE_WAIT_CHECK = 8,
        STATE_WRITE_DATA_ACTION = 9,
        STATE_WRITE_DATA_DONE = 10
    } state_t;

    state_t state;
    logic [31:0] addr;
    logic [DATA_WIDTH-1:0] data;
    logic [3:0] cnt;

    always_ff@(posedge clk_i or posedge rst_i) begin 
        if(rst_i)begin 
            state <= STATE_IDLE;
            
            wb_stb_o <= 0;
            wb_adr_o <= 0;
            wb_dat_o <= 0;
            wb_sel_o <= 0;
            wb_we_o <= 0;

            addr <= addr_i;
            cnt <= '0;
            
        end else begin 
            case(state)
                STATE_IDLE: begin 
                    if(cnt < 10) begin 
                        // 进入准备读取状�??
                        state <= STATE_READ_WAIT_ACTION;

                        // 下一�??? state
                        // master �??? slave 发出请求（读 0x10000005�???
                        wb_cyc_o <= 1'b1;
                        wb_stb_o <= 1'b1;

                        wb_adr_o <= 32'h1000_0005;
                        wb_we_o <= 1'b0;
                        wb_sel_o <= 4'b0010;
                    end
                end
                STATE_READ_WAIT_ACTION: begin 
                    // 如果 slave 完成请求（读到了 0x10000005），�???查串口是否收到数�???
                    if(wb_ack_i == 1) begin 
                        state <= STATE_READ_WAIT_CHECK;
                        wb_cyc_o <= 0;
                        wb_stb_o <= 0;
                    end
                end
                STATE_READ_WAIT_CHECK: begin 
                    wb_cyc_o <= 1;
                    wb_stb_o <= 1;
                    wb_we_o <= 0;
                    if(wb_dat_i[8] == 1) begin 
                        // 如果串口收到数据，开始读串口
                        state <= STATE_READ_DATA_ACTION;

                        wb_adr_o <= 32'h1000_0000;
                        wb_sel_o <= 4'b0001;
                    end else begin 
                        // 串口没有收到数据，继续等�???
                        state <= STATE_READ_WAIT_ACTION;

                        wb_adr_o <= 32'h1000_0005;
                        wb_sel_o <= 4'b0010;
                    end
                end
                STATE_READ_DATA_ACTION: begin 
                    if(wb_ack_i == 1)begin 
                        // 读取串口完成，再下一个状�??? master 不发出请求，把读到的数据写入 reg
                        state <= STATE_READ_DATA_DONE;

                        wb_cyc_o <= 0;
                        wb_stb_o <= 0;
                        wb_dat_o <= wb_dat_i;
                        data <= wb_dat_i;
                    end
                end
                STATE_READ_DATA_DONE: begin 
                    // 下一状�?�写 sram，设�??? master 发出请求（cyc, stb），地址�??? addr + cnt * 4，根�???
                    state <= STATE_WRITE_SRAM_ACTION;

                    wb_cyc_o <= 1;
                    wb_stb_o <= 1;
                    wb_we_o <= 1;
                    wb_adr_o <= addr + cnt * 4;
                    wb_sel_o <= (4'b0001 << ((addr + 4 * cnt) & 2'b11));
                    wb_dat_o <= (data << (((addr + 4 * cnt) & 2'b11) << 3));
                end
                STATE_WRITE_SRAM_ACTION: begin 
                    if(wb_ack_i) begin 
                        state <= STATE_WRITE_SRAM_DONE;

                        wb_cyc_o <= 0;
                        wb_stb_o <= 0;
                    end
                end
                STATE_WRITE_SRAM_DONE: begin 
                    state <= STATE_WRITE_WAIT_ACTION;

                    wb_cyc_o <= 1;
                    wb_stb_o <= 1;
                    wb_we_o <= 0;
                    wb_sel_o <= 4'b0010;
                    wb_adr_o <= 32'h1000_0005;
                    
                end
                STATE_WRITE_WAIT_ACTION: begin 
                    if(wb_ack_i == 1) begin 
                        state <= STATE_WRITE_WAIT_CHECK;
                        wb_cyc_o <= 0;
                        wb_stb_o <= 0;
                        wb_dat_o <= data;
                        
                    end
                end
                STATE_WRITE_WAIT_CHECK: begin 
                    wb_cyc_o <= 1;
                    wb_stb_o <= 1;
                    if(wb_dat_i[13] == 1) begin 
                        state <= STATE_WRITE_DATA_ACTION;

                        wb_we_o <= 1;
                        wb_adr_o <= 32'h1000_0000;
                        wb_sel_o <= 4'b0001;
                    end else begin 
                        state <= STATE_WRITE_WAIT_ACTION;

                        wb_we_o <= 0;
                        wb_adr_o <= 32'h1000_0005;
                        wb_sel_o <= 4'b0010;
                    end
                end
                STATE_WRITE_DATA_ACTION: begin 
                    if (wb_ack_i) begin
                        state <= STATE_WRITE_DATA_DONE;
                        wb_cyc_o <= 0;
                        wb_stb_o <= 0;
                        
                    end
                end

                STATE_WRITE_DATA_DONE: begin 
                    state <= STATE_IDLE;
                    cnt <= cnt+1;
                    
                end
            endcase
        end
    end

endmodule
