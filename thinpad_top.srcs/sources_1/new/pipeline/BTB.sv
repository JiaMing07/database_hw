`include "../define.svh"

module btb (
    input wire clk,
    input wire rst,
    input wire [31:0] pc_write,
    input wire [31:0] pc_branch_write,
    input wire state_bit,
    input wire we,
    input wire [31:0] pc_read,

    output reg [31:0] pc_next,
    output reg branch_or_4
);
    logic [3:0] state_bit_table;
    logic [3:0] we_table;
    logic [31:0] pc_next_table [0:3];
    logic [3:0] branch_or_4_table;

    logic [1:0] btb_idx_write;
    assign btb_idx_write = pc_write[3:2];

    logic [1:0] btb_idx_read;
    assign btb_idx_read = pc_read[3:2];

    always_comb begin
        if(we) begin
            case (btb_idx_write)
                2'b00 : begin
                    we_table = 4'b0001;
                    state_bit_table = {3'b000, state_bit};
                end
                2'b01 : begin
                    we_table = 4'b0010;
                    state_bit_table = {2'b00, state_bit, 1'b0};
                end
                2'b10 : begin
                    we_table = 4'b0100;
                    state_bit_table = {1'b0, state_bit, 2'b00};
                end
                2'b11 : begin
                    we_table = 4'b1000;
                    state_bit_table = {state_bit, 3'b000};
                end
            endcase
        end else begin
            we_table = 4'b0000;
        end
    end

    always_comb begin
        case (btb_idx_read)
            2'b00 : begin
                pc_next = pc_next_table[0];
                branch_or_4 = branch_or_4_table[0];
            end
            2'b01 : begin
                pc_next = pc_next_table[1];
                branch_or_4 = branch_or_4_table[1];
            end
            2'b10 : begin
                pc_next = pc_next_table[2];
                branch_or_4 = branch_or_4_table[2];
            end
            2'b11 : begin
                pc_next = pc_next_table[3];
                branch_or_4 = branch_or_4_table[3];
            end
        endcase
    end

    btb_table table_0(
        .clk(clk),
        .rst(rst),
        .pc_write(pc_write),
        .pc_branch_write(pc_branch_write),
        .state_write(state_bit_table[0]),
        .we(we_table[0]),
        .pc_next(pc_next_table[0]),
        .branch_or_4_o(branch_or_4_table[0]),
        .pc_read(pc_read)
    );

    btb_table table_1(
        .clk(clk),
        .rst(rst),
        .pc_write(pc_write),
        .pc_branch_write(pc_branch_write),
        .state_write(state_bit_table[1]),
        .we(we_table[1]),
        .pc_next(pc_next_table[1]),
        .branch_or_4_o(branch_or_4_table[1]),
        .pc_read(pc_read)
    );

    btb_table table_2(
        .clk(clk),
        .rst(rst),
        .pc_write(pc_write),
        .pc_branch_write(pc_branch_write),
        .state_write(state_bit_table[2]),
        .we(we_table[2]),
        .pc_next(pc_next_table[2]),
        .branch_or_4_o(branch_or_4_table[2]),
        .pc_read(pc_read)
    );

    btb_table table_3(
        .clk(clk),
        .rst(rst),
        .pc_write(pc_write),
        .pc_branch_write(pc_branch_write),
        .state_write(state_bit_table[3]),
        .we(we_table[3]),
        .pc_next(pc_next_table[3]),
        .branch_or_4_o(branch_or_4_table[3]),
        .pc_read(pc_read)
    );
endmodule