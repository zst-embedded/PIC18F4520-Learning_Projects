/* 
 * PICDEM 2 PLUS DEMO BOARD
 * PIC18F4520
 * 
 * Simple Button Debounced Polling 
 * Push button on RA4 will toggle the LEDs on RB0:3
 */

#include <P18F4520.h>

void delay(int len) {
    int i;
    for (i = 0; i < len; i++) {
        Nop();
    }
}

void main(void) {
    int poll =0;
    TRISB = 0;
    TRISA |= 1<<4;

    while(1){
        /*
        // if button pressed
        // wait short while
        // check button again
        // wait until button released
        // toggle
        */
       if (PORTAbits.RA4 == 0){
            delay(7000);
            if (PORTAbits.RA4 == 0){
                while (PORTAbits.RA4 == 0);
                LATB ^= 0x0F;
            }
       }
    }    
}