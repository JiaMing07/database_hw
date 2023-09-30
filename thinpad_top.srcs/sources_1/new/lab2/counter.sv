module counter (
    // 时钟
    input wire clk,
    // 复位信号
    input wire reset,
    // 触发信号
    input wire trigger,
    // 计数
    output reg[3:0] count
);
    logic [3:0] count_reg;

    // 注意此时的敏感信号列表
    always_ff @ (posedge clk or posedge reset) begin
        if(reset) begin
            count_reg <= 4'd0;
        end else begin
            if (trigger && count_reg != 4'hf)   // 增加此处
                count_reg <= count_reg + 4'd1;  // 暂时忽略计数溢出
        end
    end
    assign count = count_reg;
    
endmodule