module trigger (
    input wire clk,
    input wire reset,
    input wire btn,
    output wire trig
);
    logic last_button;
    logic trigger_reg;

    always_ff @ (posedge clk or posedge reset) begin 
        if(reset)begin 
            last_button <= 1'b0;
            trigger_reg <= 1'b0;
        end else begin 
            if(btn) begin 
                // 当 btn == 1 时，可能会出现上升沿，与 last_button 对比进行判断
                if(last_button == 1'b0) begin 
                    last_button <= 1'b1;
                    trigger_reg <= 1'b1;
                end else begin 
                    last_button <= 1'b1;
                    trigger_reg <= 1'b0;
                end 
            end else begin 
                    last_button <= 1'b0;
                    trigger_reg <= 1'b0;
            end
        end
    end

    assign trig = trigger_reg;
    
endmodule