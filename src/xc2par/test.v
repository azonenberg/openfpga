module top(clk_, o);

output wire o;
input clk_;

BUFG clk_bufg (
    .I(clk_),
    .O(clk));

FTDCP toggle (
    .C(clk),
    .PRE(1'b0),
    .CLR(1'b0),
    .T(1'b1),
    .Q(o));
endmodule

// module FTCP (C, PRE, CLR, T, Q);
//     parameter INIT = 0;

//     input C, PRE, CLR, T;
//     output wire Q;
//     reg Q_;

//     initial begin
//         Q_ <= INIT;
//     end

//     always @(posedge C, posedge PRE, posedge CLR) begin
//         if (CLR == 1)
//             Q_ <= 0;
//         else if (PRE == 1)
//             Q_ <= 1;
//         else if (T == 1)
//             Q_ <= ~Q_;
//     end

//     assign Q = Q_;
// endmodule
