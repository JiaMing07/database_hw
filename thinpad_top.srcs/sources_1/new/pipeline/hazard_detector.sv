`include "../define.svh"
module hazard_detector (
    input wire [31:0] id_inst,
    
    input wire ex_mem_read,
    input wire ex_reg_write, 
    input wire [4:0] ex_waddr,

    input wire flush_o, //flush_o = branch

    output reg if_id_stall,
    output reg id_ex_stall,

    output reg if_id_bubble,
    output reg id_ex_bubble
);
    logic stall_o;
    hazard u_hazard(
        .inst(id_inst),
        .ex_mem_read(ex_mem_read),
        .ex_reg_write(ex_reg_write),
        .ex_waddr(ex_waddr),
        .stall_o(stall_o)
    );

    stall_bubble u_stall_bubble(
        .stall_o(stall_o),
        .flush_o(flush_o),
        .if_id_stall(if_id_stall),
        .id_ex_stall(id_ex_stall),
        .if_id_bubble(if_id_bubble),
        .id_ex_bubble(id_ex_bubble)
    );
    
endmodule