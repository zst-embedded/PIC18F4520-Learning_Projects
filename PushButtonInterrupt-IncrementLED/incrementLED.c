/* 
 * PICDEM 2 PLUS DEMO BOARD
 * PIC18F4520
 * 
 * Interrupt-driven LED counter
 * Push button on RB0 will trigger an interrupt 
 * and increment a counter. The binary value is
 * shown with LEDs on RA0:7
 */

#include <p18f4520.h>
#include <GenericTypeDefs.h>

BOOL RB0_Pressed = FALSE;
void ISR(void);

void main(void) {
    UINT8 count = 0;
    
    // RB0 as input
    TRISB = 1<<0;
    
    // RA0:7 as output
    TRISA = 0;
    
    // Setup push button interrupt
    INTCONbits.GIEH = 1; // Enable global interrupt
    INTCONbits.INT0IE = 1; // INT0 enabled
    INTCON2bits.INTEDG0 = 0; // Interrupt on falling edge
    
    while (1) {
        if (RB0_Pressed) {
            RB0_Pressed = FALSE;
            //Increment LED counter
            count++;
        }
        // Update LEDs
        LATA = count;
    }
}


#pragma code InterruptVectorHigh = 0x08 
void InterruptVectorHigh(void) {
    _asm
    goto ISR
    _endasm
}
#pragma code
#pragma interrupt ISR
void ISR(void) {
    if (INTCONbits.INT0IF == 1) {
        INTCONbits.INT0IF = 0;
        RB0_Pressed = TRUE;
    }
}
