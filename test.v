module my_processor (
    input wire clk,
    input wire reset,
    input wire [7:0] data_in,
    input wire [3:0] addr,
    output reg [7:0] data_out,
    output reg ready,
    output reg valid
);

always @(posedge clk or posedge reset) begin
    if (reset) begin
        data_out <= 8'b0;
        ready <= 1'b0;
        valid <= 1'b0;
    end else begin
        valid <= 1'b1;
    end
end

endmodule