/**
 * N64 To USB adapter
 * by Michele Perla (the.mickmad@gmail.com)
 * https://github.com/MickMad/N64-To-USB
 * 
 * Gamecube controller to Nintendo 64 adapter
 * by Andrew Brown
 * Rewritten for N64 to HID by Peter Den Hartog
 */

#ifndef N64Controller_h
#define N64Controller_h

#define N64_PIN     10
#define N64_PIN_DIR DDRB

// these two macros set arduino pin 10 to input or output, which with an
// external 1K pull-up resistor to the 3.3V rail, is like pulling it high or
// low.  These operations translate to 1 op code, which takes 2 cycles
#define N64_HIGH     DDRB &= ~B01000000
#define N64_LOW      DDRB |=  B01000000
#define N64_QUERY   (PINB &   B01000000)

// 8 bytes of data that we get from the controller
typedef struct state
{
    char stick_x;
    char stick_y;
    
    // bits: 0, 0, 0, start, y, x, b, a
    unsigned char data1;
    
    // bits: 1, L, R, Z, Dup, Ddown, Dright, Dleft
    unsigned char data2;
} N64_status_packet;

class N64Controller 
{
  public:  
    N64Controller();
    void N64_init();
    void translate_N64_data();
    void N64_get_data_from_controller();
    void N64_send_data_request(unsigned char *buffer, char length);
    void print_N64_status();
    void getN64Packet();
    N64_status_packet N64_status;

  private:
    char N64_raw_dump[33]; // 1 received bit per byte
};

#endif
