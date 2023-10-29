`timescale 1ns / 1ps
module lab3_tb;

  wire clk_50M, clk_11M0592;

  reg push_btn;   // BTN5 按钮�??关，带消抖电路，按下时为 1
  reg reset_btn;  // BTN6 复位按钮，带消抖电路，按下时�?? 1

  reg [3:0] touch_btn; // BTN1~BTN4，按钮开关，按下时为 1
  reg [31:0] dip_sw;   // 32 位拨码开关，拨到“ON”时�?? 1

  wire [15:0] leds;  // 16 �?? LED，输出时 1 点亮
  wire [7:0] dpy0;   // 数码管低位信号，包括小数点，输出 1 点亮
  wire [7:0] dpy1;   // 数码管高位信号，包括小数点，输出 1 点亮

  // 实验 3 用到的指令格�??
  `define inst_rtype(rd, rs1, rs2, op) \
    {7'b0, rs2, rs1, 3'b0, rd, op, 3'b001}

  `define inst_itype(rd, imm, op) \
    {imm, 4'b0, rd, op, 3'b010}
  
  `define inst_poke(rd, imm) `inst_itype(rd, imm, 4'b0001)
  `define inst_peek(rd, imm) `inst_itype(rd, imm, 4'b0010)

  // opcode table
  typedef enum logic [3:0] {
    ADD = 4'b0001,
    SUB = 4'b0010,
    AND = 4'b0011,
    OR  = 4'b0100,
    XOR = 4'b0101,
    NOT = 4'b0110,
    SLL = 4'b0111,
    SRL = 4'b1000,
    SRA = 4'b1001,
    ROL = 4'b1010
  } opcode_t;

  logic is_rtype, is_itype, is_load, is_store, is_unknown;
  logic [15:0] imm;
  logic [4:0] rd, rs1, rs2;
  logic [3:0] opcode;
  logic [15:0] regs [0:31];
  logic [4:0] ran;
  
  task automatic read(input int i);
      #100;
      rd = i;   // only lower 5 bits
      dip_sw = `inst_peek(rd, 0);
      push_btn = 1;

      #100;
      push_btn = 0;

      #1000;
    endtask //automatic
    
    task automatic alu(input int i);
      #100;
      rd = 1;   // only lower 5 bits
      rs1 = 1;
      rs2 = 2;
//      rs1 = $urandom_range(0, 31);
//      rs2 = $urandom_range(0, 31);
      opcode = i;
      dip_sw = `inst_rtype(rd, rs1, rs2, opcode);
      push_btn = 1;

      #100;
      push_btn = 0;

      #1000;
    endtask //automatic

  initial begin
    // 在这里可以自定义测试输入序列，例如：
    dip_sw = 32'h0;
    touch_btn = 0;
    reset_btn = 0;
    push_btn = 0;

    

    #100;
    reset_btn = 1;
    #100;
    reset_btn = 0;
    #2000;  // 等待复位结束

    // 样例：使�?? POKE 指令为寄存器赋随机初�??
    for (int i = 1; i < 32; i = i + 1) begin
      #100;
      rd = i;   // only lower 5 bits
//      ran = $urandom_range(0, 65536);
      ran = 1;
      regs[rd] = ran;
      dip_sw = `inst_poke(rd, ran);
      push_btn = 1;

      #100;
      push_btn = 0;

      #1000;
    end
    


    
    for (int i = 1; i < 32; i = i + 1) begin
        read(i);
    end

    // TODO: 随机测试各种指令
    for (int i = 1; i < 11; i = i + 1) begin
        alu(i);
    end

    #10000 $finish;
  end

  // 待测试用户设�??
  lab3_top dut (
      .clk_50M(clk_50M),
      .clk_11M0592(clk_11M0592),
      .push_btn(push_btn),
      .reset_btn(reset_btn),
      .touch_btn(touch_btn),
      .dip_sw(dip_sw),
      .leds(leds),
      .dpy1(dpy1),
      .dpy0(dpy0),

      .txd(),
      .rxd(1'b1),
      .uart_rdn(),
      .uart_wrn(),
      .uart_dataready(1'b0),
      .uart_tbre(1'b0),
      .uart_tsre(1'b0),
      .base_ram_data(),
      .base_ram_addr(),
      .base_ram_ce_n(),
      .base_ram_oe_n(),
      .base_ram_we_n(),
      .base_ram_be_n(),
      .ext_ram_data(),
      .ext_ram_addr(),
      .ext_ram_ce_n(),
      .ext_ram_oe_n(),
      .ext_ram_we_n(),
      .ext_ram_be_n(),
      .flash_d(),
      .flash_a(),
      .flash_rp_n(),
      .flash_vpen(),
      .flash_oe_n(),
      .flash_ce_n(),
      .flash_byte_n(),
      .flash_we_n()
  );

  // 时钟�??
  clock osc (
      .clk_11M0592(clk_11M0592),
      .clk_50M    (clk_50M)
  );

endmodule
