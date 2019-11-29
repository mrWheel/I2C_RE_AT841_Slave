// ATMEL ATTINY841
//                                +--\/--+
//                           VCC 1|      |14 GND
//   ROT_A    --->     (D10) PB0 2|      |13 PA0 AREF (D0) <--- ROT_B
//   n.c.               (D9) PB1 3|      |12 PA1      (D1) ---> SEND_INT
//   n.c.              RESET PB3 4|      |11 PA2      (D2) <--- BUTTON
//   RED LED  <--- INT0 (D8) PB2 5|      |10 PA3      (D3)      n.c.
//   GRN LED  <---      (D7) PA7 6|      |9  PA4      (D4) <--> SCL      [SLK]
// [MOSI] SDA <-->      (D6) PA6 7|      |8  PA5      (D5) ---> BLUE LED [MISO]
//                                +------+
//                            ^               ^
//                            |               |
//          Clockwise Pinout  +---------------+ like Rev. D boards
