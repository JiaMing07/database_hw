module stall_pipeline (
    input wire if_master_ready,
    input wire mem_master_ready,

    output reg pipeline_stall
);
    assign pipeline_stall = ~(if_master_ready & mem_master_ready);
    
endmodule