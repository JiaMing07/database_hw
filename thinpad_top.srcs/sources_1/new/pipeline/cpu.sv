`include "../define.svh"
module cpu #(
    parameter DATA_WIDTH = 32, 
    parameter ADDR_WIDTH = 32,
    parameter SELECT_WIDTH = (DATA_WIDTH/8)
)(
    input wire clk,
    input wire rst,
    // IF wb master
    output  reg [ADDR_WIDTH-1:0]   wbm0_adr_o,    
    output  reg [DATA_WIDTH-1:0]   wbm0_dat_o,    
    input wire [DATA_WIDTH-1:0]   wbm0_dat_i,    
    output  reg                    wbm0_we_o,     
    output  reg [SELECT_WIDTH-1:0] wbm0_sel_o,   
    output  reg                    wbm0_stb_o, 
    input wire                    wbm0_ack_i, 
    output  reg                    wbm0_cyc_o, 

    // MEM wb master
    output  reg [ADDR_WIDTH-1:0]   wbm1_adr_o, 
    output  reg [DATA_WIDTH-1:0]   wbm1_dat_o, 
    input wire [DATA_WIDTH-1:0]   wbm1_dat_i, 
    output  reg                    wbm1_we_o, 
    output  reg [SELECT_WIDTH-1:0] wbm1_sel_o,
    output  reg                    wbm1_stb_o, 
    input wire                     wbm1_ack_i, 
    output  reg                    wbm1_cyc_o 
);

    // pipeline stall
    logic if_master_ready;
    logic mem_master_ready;
    logic pipeline_stall;
    stall_pipeline u_stall_pipeline(
        .if_master_ready(if_master_ready),
        .mem_master_ready(mem_master_ready),
        .pipeline_stall(pipeline_stall)
    );

    //stall 
    logic if_id_stall;
    logic id_ex_stall;

    // flush 
    logic flush_o;

    // bubble 
    logic if_id_bubble;
    logic id_ex_bubble;

    // branch 
    logic [31:0] pc_branch;
    logic branch;

    assign flush_o = branch;

    // alu data
    logic [31:0] data_a;
    logic [31:0] data_b;
    logic [31:0] alu_res;

    // IF
    logic [31:0] if_pc;
    logic [31:0] if_inst;

    // ID
    logic [31:0] id_pc;
    logic [31:0] id_inst;
    logic [31:0] id_imm;

    // id regfile signal
    logic id_reg_write;
    logic id_mem_to_reg;

    //id regfile data
    logic [31:0] id_data_a;
    logic [31:0] id_data_b;

    // id memory signal
    logic id_mem_write;
    logic id_mem_read;
    logic [3:0] id_wb_sel; // wishbone select

    // id alu signal
    logic [3:0] id_alu_op;
    logic id_alu_sel_pc_a;
    logic id_alu_sel_imm_b;

    // EX
    logic [31:0] ex_pc;
    logic [31:0] ex_inst;
    logic [31:0] ex_imm;

    //ex regfile signal
    logic ex_reg_write;
    logic ex_mem_to_reg;

    // ex regfile read data
    logic [31:0] ex_data_a;
    logic [31:0] ex_data_b;
    
    // ex memory signal
    logic ex_mem_read;
    logic ex_mem_write;
    logic [3:0] ex_wb_sel; // wishbone select

    // ex alu signal
    logic [3:0] ex_alu_op;
    logic ex_alu_sel_pc_a;
    logic ex_alu_sel_imm_b;

    // ex inst data
    logic [4:0] ex_rs1;
    logic [4:0] ex_rs2;
    logic [4:0] ex_waddr; // regfile waddr

    // foward unit
    logic [1:0] ex_forward_a;
    logic [1:0] ex_forward_b;
    // MEM
    // MEM regfile signal
    logic mem_reg_write;
    logic mem_mem_to_reg;
    
    // MEM memory signal
    logic mem_mem_write;
    logic mem_mem_read; 
    logic [3:0] mem_wb_sel;

    logic [31:0] mem_alu_res;
    logic [31:0] mem_wdata_d;
    logic [31:0] mem_read_data;
    logic [4:0] mem_waddr; // regfile waddr
    logic [31:0] mem_wdata;

    //wb 
    logic wb_reg_write;
    logic [4:0] wb_waddr; // regfile waddr

    // wb regfile data
    logic [31:0] wb_wdata;

    /*===================== IF begin =========================*/

    if_master u_if_master(
        .clk(clk),
        .rst(rst),
        .pipeline_stall(pipeline_stall),
        .stall_i(if_id_stall),
        .branch(branch),
        .pc_branch(pc_branch),
        .wb_ack_i(wbm0_ack_i),
        .wb_dat_i(wbm0_dat_i),
        .wb_cyc_o(wbm0_cyc_o),
        .wb_adr_o(wbm0_adr_o),
        .wb_dat_o(wbm0_dat_o),
        .wb_sel_o(wbm0_sel_o),
        .wb_stb_o(wbm0_stb_o),
        .wb_we_o(wbm0_we_o),
        .inst(if_inst),
        .pc(if_pc),
        .if_master_ready(if_master_ready)
    );

    /*===================== IF end =========================*/

    if_id u_if_id(
        .clk(clk),
        .rst(rst),
        .pipeline_stall(pipeline_stall),
        .bubble_i(if_id_bubble),
        .stall_i(if_id_stall),
        .if_pc(if_pc),
        .if_inst(if_inst),
        .id_pc(id_pc),
        .id_inst(id_inst)
    );

    /*===================== ID begin =========================*/

    imm_gen u_imm_gen(
        .inst(id_inst),
        .imm(id_imm)
    );

    logic [4:0] id_rs1;
    logic [4:0] id_rs2;

    assign id_rs1 = id_inst[19:15];
    assign id_rs2 = id_inst[24:20];

    pipeline_controller u_pipeline_controller(
        .inst(id_inst),
        .alu_op(id_alu_op),
        .alu_sel_pc_a(id_alu_sel_pc_a),
        .alu_sel_imm_b(id_alu_sel_imm_b),
        .mem_read(id_mem_read),
        .mem_write(id_mem_write),
        .wb_sel(id_wb_sel),
        .reg_write(id_reg_write),
        .mem_to_reg(id_mem_to_reg)
    );

    hazard_detector u_hazard_detextor(
        .id_inst(id_inst),
        .ex_mem_read(ex_mem_read),
        .ex_reg_write(ex_reg_write),
        .ex_waddr(ex_waddr),
        .flush_o(flush_o),
        .if_id_stall(if_id_stall),
        .id_ex_stall(id_ex_stall),
        .if_id_bubble(if_id_bubble),
        .id_ex_bubble(id_ex_bubble)
    );

    register_file u_register_file(
        .clk(clk),
        .reset(rst),
        .waddr(wb_waddr),
        .wdata(wb_wdata),
        .we(wb_reg_write),
        .raddr_a(id_inst[19:15]),
        .rdata_a(id_data_a),
        .raddr_b(id_inst[24:20]),
        .rdata_b(id_data_b)
    );

    logic [1:0] id_forward_a, id_forward_b;
    forward id_branch_forward(
        .ex_rs1(id_rs1),
        .ex_rs2(id_rs2),
        .mem_rd(mem_waddr),
        .mem_reg_write(mem_reg_write),
        .wb_rd(wb_waddr),
        .wb_reg_write(wb_reg_write),
        .forward_a(id_forward_a),
        .forward_b(id_forward_b)
    );

    logic [31:0] id_forward_data_a;
    logic [31:0] id_forward_data_b;

    select_data id_select_data_a(
        .forward(id_forward_a),
        .ex_data(id_data_a),
        .mem_alu_res(mem_alu_res),
        .wb_wdata(wb_wdata),
        .data(id_forward_data_a)
    );

    select_data id_select_data_b(
        .forward(id_forward_b),
        .ex_data(id_data_b),
        .mem_alu_res(mem_alu_res),
        .wb_wdata(wb_wdata),
        .data(id_forward_data_b)
    );

    branch_compare u_branch_compare(
        .data_a(id_forward_data_a),
        .data_b(id_forward_data_b),
        .pc(id_pc),
        .imm(id_imm),
        .inst(id_inst),
        .branch(branch),
        .pc_branch(pc_branch)
    );

    /*===================== ID end =========================*/

    id_ex u_id_ex(
        .clk(clk),
        .rst(rst),
        .pipeline_stall(pipeline_stall),
        .id_ex_stall(id_ex_stall),
        .id_ex_bubble(id_ex_bubble),

        .id_pc(id_pc),
        .id_inst(id_inst),
        .id_reg_write(id_reg_write),
        .id_mem_to_reg(id_mem_to_reg),
        .id_data_a(id_data_a),
        .id_data_b(id_data_b),
        .id_mem_write(id_mem_write),
        .id_mem_read(id_mem_read),
        .id_wb_sel(id_wb_sel),
        .id_alu_op(id_alu_op),
        .id_alu_sel_pc_a(id_alu_sel_pc_a),
        .id_alu_sel_imm_b(id_alu_sel_imm_b),
        .id_imm(id_imm),

        .ex_pc(ex_pc),
        .ex_inst(ex_inst),
        .ex_reg_write(ex_reg_write),
        .ex_mem_to_reg(ex_mem_to_reg),
        .ex_waddr(ex_waddr),
        .ex_data_a(ex_data_a),
        .ex_data_b(ex_data_b),
        .ex_mem_write(ex_mem_write),
        .ex_mem_read(ex_mem_read),
        .ex_wb_sel(ex_wb_sel),
        .ex_alu_op(ex_alu_op),
        .ex_alu_sel_pc_a(ex_alu_sel_pc_a),
        .ex_alu_sel_imm_b(ex_alu_sel_imm_b),
        .ex_rs1(ex_rs1),
        .ex_rs2(ex_rs2),
        .ex_imm(ex_imm)
    );

    /*===================== EX begin =========================*/

    forward u_forward(
        .ex_rs1(ex_rs1),
        .ex_rs2(ex_rs2),
        .mem_rd(mem_waddr),
        .mem_reg_write(mem_reg_write),
        .wb_rd(wb_waddr),
        .wb_reg_write(wb_reg_write),
        .forward_a(ex_forward_a),
        .forward_b(ex_forward_b)
    );

    select_data select_data_a(
        .forward(ex_forward_a),
        .ex_data(ex_data_a),
        .mem_alu_res(mem_alu_res),
        .wb_wdata(wb_wdata),
        .data(data_a)
    );

    select_data select_data_b(
        .forward(ex_forward_b),
        .ex_data(ex_data_b),
        .mem_alu_res(mem_alu_res),
        .wb_wdata(wb_wdata),
        .data(data_b)
    );

    // branch_compare u_branch_compare(
    //     .data_a(data_a),
    //     .data_b(data_b),
    //     .pc(ex_pc),
    //     .imm(ex_imm),
    //     .inst(ex_inst),
    //     .branch(branch),
    //     .pc_branch(pc_branch)
    // );

    logic [31:0] alu_data_a;
    logic [31:0] alu_data_b;

    // ALU_SEL_A = 1'b0 = ALU_SEL_B
    data_mux_2 sel_alu_data_a(
        .data_1(ex_pc),
        .data_2(data_a),
        .select(ex_alu_sel_pc_a),
        .data_o(alu_data_a)
    );

    data_mux_2 sel_alu_data_b(
        .data_1(ex_imm),
        .data_2(data_b),
        .select(ex_alu_sel_imm_b),
        .data_o(alu_data_b)
    );

    alu u_alu(
        .rs1(alu_data_a),
        .rs2(alu_data_b),
        .opcode(ex_alu_op),
        .ans(alu_res)
    );

    /*===================== EX end =========================*/

    ex_mem u_ex_mem(
        .clk(clk),
        .rst(rst),
        .pipeline_stall(pipeline_stall),
        .alu_res(alu_res),
        .ex_wdata_wishbone(data_b),
        .ex_reg_write(ex_reg_write),
        .ex_waddr(ex_waddr),
        .ex_mem_to_reg(ex_mem_to_reg),
        .ex_mem_write(ex_mem_write),
        .ex_mem_read(ex_mem_read),
        .ex_wb_sel(ex_wb_sel),

        .mem_alu_res(mem_alu_res),
        .mem_wdata_d(mem_wdata_d),
        .mem_reg_write(mem_reg_write),
        .mem_mem_to_reg(mem_mem_to_reg),
        .mem_waddr(mem_waddr),
        .mem_mem_read(mem_mem_read),
        .mem_mem_write(mem_mem_write),
        .mem_wb_sel(mem_wb_sel)
    );

    /*===================== MEM begin =========================*/

    mem_master u_mem_master(
        .clk(clk),
        .rst(rst),
        .wb_ack_i(wbm1_ack_i),
        .wb_dat_i(wbm1_dat_i),
        .wb_cyc_o(wbm1_cyc_o),
        .wb_stb_o(wbm1_stb_o),
        .wb_adr_o(wbm1_adr_o),
        .wb_dat_o(wbm1_dat_o),
        .wb_sel_o(wbm1_sel_o),
        .wb_we_o(wbm1_we_o),
        .data_write(mem_wdata_d),
        .addr(mem_alu_res),
        .mem_write(mem_mem_write),
        // .mem_write(ex_mem_write),
        .mem_read(mem_mem_read),
        // .mem_read(ex_mem_read),
        .wb_sel_i(mem_wb_sel),
        .data_read(mem_read_data),
        .mem_master_ready(mem_master_ready)
    );

    
    data_mux_2 sel_mem_wdata(
        .data_1(mem_read_data),
        .data_2(mem_alu_res),
        .select(mem_mem_to_reg),
        .data_o(mem_wdata)
    );

    /*===================== MEM end =========================*/

    mem_wb u_mem_wb(
        .clk(clk),
        .rst(rst),
        .pipeline_stall(pipeline_stall),
        .mem_reg_write(mem_reg_write),
        .mem_waddr(mem_waddr),
        .mem_wdata(mem_wdata),
        .wb_reg_write(wb_reg_write),
        .wb_waddr(wb_waddr),
        .wb_wdata(wb_wdata)
    );

    /*===================== WB begin =========================*/
    // regfile 
    /*===================== WB end =========================*/

endmodule