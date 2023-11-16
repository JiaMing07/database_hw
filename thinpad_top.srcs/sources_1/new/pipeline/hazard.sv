`include "../define.svh"

module hazard(
    input wire [31:0] inst,
    
    input wire ex_mem_read,
    input wire ex_reg_write, 
    input wire [4:0] ex_waddr,

    output reg stall_o
);

    assign stall_o = ex_mem_read & (ex_waddr == inst[24:20] | ex_waddr == inst[19:15]);   

endmodule