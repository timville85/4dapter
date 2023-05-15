/**
 * N64 To USB adapter
 * by Michele Perla (the.mickmad@gmail.com)
 * https://github.com/MickMad/N64-To-USB
 * 
 * Gamecube controller to Nintendo 64 adapter
 * by Andrew Brown
 * Rewritten for N64 to HID by Peter Den Hartog
 */

#include "N64_Controller.h"
#include "Arduino.h"

N64Controller::N64Controller()
{
  
}

void N64Controller::N64_init()
{
  //N64 Setup
  digitalWrite(N64_PIN, LOW);  
  pinMode(N64_PIN, INPUT);
}

void N64Controller::translate_N64_data()
{
    memset(&N64_status, 0, sizeof(N64_status));
    
    // line 1
    // bits: A, B, Z, Start, Dup, Ddown, Dleft, Dright
    // line 2
    // bits: Reset, 0, L, R, Cup, Cdown, Cleft, Cright
    // line 3
    // bits: joystick x value
    // These are 8 bit values centered at 0x80 (128)
    // line 4
    // bits: joystick 4 value
    // These are 8 bit values centered at 0x80 (128)
    
    for (int i=0; i<8; i++) 
    {
        N64_status.data1 |= N64_raw_dump[i] ? (0x80 >> i) : 0;
        N64_status.data2 |= N64_raw_dump[8+i] ? (0x80 >> i) : 0;
        N64_status.stick_x |= N64_raw_dump[16+i] ? (0x80 >> i) : 0;
        N64_status.stick_y |= N64_raw_dump[24+i] ? (0x80 >> i) : 0;
    }
}


/**
 * This sends the given byte sequence to the controller
 * length must be at least 1
 * Oh, it destroys the buffer passed in as it writes it
 */
void N64Controller::N64_send_data_request(unsigned char *buffer, char length)
{
    char bits;
    bool bit;
    unsigned char timeout;
    char bitcount = 32;
    char *bitbin = N64_raw_dump;


  outer_loop:
    {
        bits = 8;
        
        inner_loop:
        {
            // Starting a bit, set the line low
            N64_LOW;

            if (*buffer >> 7) //Bit is a 1
            {
                __builtin_avr_delay_cycles(5);
                N64_HIGH;
                __builtin_avr_delay_cycles(40);
            } 
            else  //Bit is a 0
            {
                 __builtin_avr_delay_cycles(40);
                N64_HIGH;
            }
            
            --bits;
            if (bits != 0) 
            {
                __builtin_avr_delay_cycles(8);
                *buffer <<= 1;
                goto inner_loop;
            }
        }
        
        --length;
        if (length != 0) 
        {
            ++buffer;
            goto outer_loop;
        } 
    }

    // send a single stop (1) bit
    __builtin_avr_delay_cycles(8);
    N64_LOW;

    // wait 1 us, 16 cycles, then raise the line 
    __builtin_avr_delay_cycles(16);
    N64_HIGH;

    //////////////////////
    // listen for the expected 8 bytes of data back from the controller and
    // blast it out to the N64_raw_dump array, one bit per byte for extra speed.

    timeout = 0x3f;
    while (!N64_QUERY) 
    {
        if (!--timeout)
            return;
    }

read_loop:
   
    // wait for line to go low
    timeout = 0x3f;
    while (N64_QUERY) 
    {
        if (!--timeout)
            return;
    }

    //Wait 2us before reading data
    __builtin_avr_delay_cycles(32);
   
    *bitbin = N64_QUERY;
    ++bitbin;
    --bitcount;
    
    if (bitcount == 0)
        return;

    // wait for line to go high again
    // it may already be high, so this should just drop through
    timeout = 0x3f;
    while (!N64_QUERY) 
    {
        if (!--timeout)
            return;
    }
    
    goto read_loop;
}

void N64Controller::getN64Packet()
{
    unsigned char N64Command[] = {0x01};
    noInterrupts();
    N64_send_data_request(N64Command, 1);
    interrupts();
    translate_N64_data();
}
