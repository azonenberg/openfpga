module top(a, b, o);

output wire o;
input a;
input b;

assign o = a ^ b;

endmodule
