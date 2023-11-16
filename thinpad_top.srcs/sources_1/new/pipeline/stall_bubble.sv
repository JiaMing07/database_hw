`include "../define.svh"
module stall_bubble (
    input wire stall_o,
    input wire flush_o, //flush_o

    output reg if_id_stall,
    output reg id_ex_stall,

    output reg if_id_bubble,
    output reg id_ex_bubble
);
    always_comb begin
        if (flush_o) begin
            if_id_stall = 1'b0;
            id_ex_stall = 1'b0;
            if_id_bubble = 1'b1;
            id_ex_bubble = 1'b1;
        end else if (stall_o) begin
            if_id_stall = 1'b1;
            id_ex_stall = 1'b0;
            if_id_bubble = 1'b0;
            id_ex_bubble = 1'b1;
        end else begin
            if_id_stall = 1'b0;
            id_ex_stall = 1'b0;
            if_id_bubble = 1'b0;
            id_ex_bubble = 1'b0;
        end
    end
endmodule