module data_mux_2 #(
    DATA_WIDTH = 32,
    SEL_WIDTH = 1
) (
    input wire [DATA_WIDTH-1:0] data_1,
    input wire [DATA_WIDTH-1:0] data_2,
    input wire select,
    output wire [DATA_WIDTH-1:0] data_o
);
    // when select == 1, select data_1, else select data_2
    assign data_o = select ? data_1 : data_2;
endmodule