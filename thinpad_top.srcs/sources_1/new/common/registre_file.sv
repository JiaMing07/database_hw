module register_file(
    input wire clk,
    input wire reset,

    // IO write(waddr, wdata) reada(raddr_a, rdata_a) readb(raddr_b, rdata_b)
    // we 使能信号 判断读还是写
    input wire [4:0] waddr,
    input wire [15:0] wdata,
    input wire we,
    input wire [4:0] raddr_a,
    output reg [15:0] rdata_a,
    input wire [4:0] raddr_b,
    output reg [15:0] rdata_b
);
    logic [15:0] registers [0:31];

    always_ff @(posedge clk) begin
        if(reset) begin 
            for(int i = 0; i < 32; i = i + 1) begin 
                registers[i] <= 0;
            end
        end else begin 
            if(we && waddr != 0) begin 
                registers[waddr] <= wdata;
            end
        end
    end

    assign rdata_a = registers[raddr_a];
    assign rdata_b = registers[raddr_b];

endmodule