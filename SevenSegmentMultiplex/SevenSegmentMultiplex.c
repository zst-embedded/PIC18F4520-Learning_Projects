/* 
 * PICDEM 2 PLUS DEMO BOARD
 * PIC18F4520
 * 
 * This uses 4-digit multiplexed seven segment display.
 * The display counts up from 0000 to 9999
 *
 * Segments A-G are on RD[0:6] and are active LOW.
 * Common selector pins are on RE[0:1] and are active HIGH.
 *
 * Timer 0 is used to multiplex the display by switching to
 * the next digit every interrupt.
 * 
 */

#include <p18f4520.h>
#include <GenericTypeDefs.h>
#include <delays.h>

const UINT8 SEVEN_SEGMENT[] = {
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7D, // 6
    0x07, // 7
    0x7F, // 8
    0x67, // 9
};

UINT8 mux_digits[4] = {0, 0, 0, 0};
UINT8 mux_selector = 0;

void main(void);
void mux_UpdateDisplay(UINT8);
void mux_SetDigits(UINT16);
void InterruptHandlerHigh(void);

void main(void) {
    // Setup seven segment pins as output
    TRISD = 0x00;
    TRISEbits.TRISE0 = 0;
    TRISEbits.TRISE1 = 0;
    
    // Setup Timer 0 for mux
    T0CONbits.TMR0ON = 1; // 1 = Enables Timer0
    T0CONbits.T08BIT = 1; // 1 = Timer0 is configured as an 8-bit timer/counter 
    T0CONbits.T0CS = 0; // 0 = Internal instruction cycle clock (CLKO) 
    T0CONbits.PSA = 1; // 0 = Timer0 prescaler is not assigned
    
    // Enable Timer 0 interrupt
    INTCONbits.GIEH = 1;
    INTCONbits.GIEL = 1;
    INTCONbits.TMR0IE = 1; // 1 = Enables the TMR0 overflow interrupt 
    INTCONbits.TMR0IF = 0; // Clear Timer 0 Interrupt Flag
    
    while (1) {
        UINT16 i;
        for (i = 0; i < 10000; i++) {
            mux_SetDigits(i);
            Delay10KTCYx(25);
        }
    }
}

void mux_SetDigits(UINT16 input) {
    mux_digits[0] = input % 10;
    input /= 10;
    mux_digits[1] = input % 10;
    input /= 10;
    mux_digits[2] = input % 10;
    input /= 10;
    mux_digits[3] = input % 10;
    input /= 10;
}

void mux_UpdateDisplay(UINT8 display) {
    LATEbits.LATE0 = display & 0b01;
    LATEbits.LATE1 = (display >> 1) & 0b01;
    // Look up each digit and get its 7-segment decoded value
    // Invert (~) because it is active LOW
    LATD = ~SEVEN_SEGMENT[mux_digits[display]]; 
}

#pragma code InterruptVectorHigh = 0x08
void InterruptVectorHigh (void) {
    _asm
        goto InterruptHandlerHigh
    _endasm
}

#pragma code
#pragma interrupt InterruptHandlerHigh
void InterruptHandlerHigh(void) {
    if (INTCONbits.TMR0IF) {
        // Multiplex display every timer interrupt
        mux_UpdateDisplay(mux_selector++);
        if (mux_selector >= 4) {
            mux_selector = 0;
        }
        TMR0L = 8; // Reset timer value
        // 10MHz Crystal at HS mode
        // Fosc/4 = 2.5Mhz
        // 3us / (1/2.5MHz) = 7.5
        INTCONbits.TMR0IF = 0; // Clear Timer 0 Interrupt Flag
    }
}