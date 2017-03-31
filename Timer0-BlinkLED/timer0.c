/* 
 * PICDEM 2 PLUS DEMO BOARD
 * PIC18F4520
 * 
 * Timer 0 is set to trigger an interrupt every 100ms when
 * the clock is set to 10MHz (Fosc = 10 MHz) and toggle an
 * LED on RB0.
 * 
 * Clock sources can be played around by modifying OSCCON
 * and OSCTUNE register or OSC configuration bits.
 * The push button on RA4 will toggle the clock source
 * between primary oscillator, secondary oscillator and 
 * internal oscillator.
 */

#include <P18F4520.h>
#include <GenericTypeDefs.h>
#include <delays.h>

BOOL timer0_flag = FALSE;
void ISR(void);

void main(void) {
    UINT8 clock = 0;
    
    // Internal Oscillator with PLL (2.6.4 PLL IN INTOSC MODES))
    // OSCCONbits.IRCF = 0b111; // Internal Oscillator Frequency (FOSC = 8MHz)
    OSCCONbits.IRCF = 0b110; // Internal Oscillator Frequency (FOSC = 4MHz)
    OSCTUNEbits.PLLEN = 1; // Enable PLL for internal osc. (after setting FOSC > 4MHz)
    
    // RA4 as input
    TRISA |= 1<<4;
    
    // RBx as output
    TRISB = 0;
    LATB = 0;

    // Enable global and peripheral interrupts
    INTCONbits.GIEH = 1;
    INTCONbits.PEIE = 1;
    
    // Setup Timer 0
    T0CONbits.TMR0ON = 1; // Enables Timer0 
    T0CONbits.T08BIT = 0; // 0 = 16-bit timer/counter 
    T0CONbits.T0CS = 0; // internal clock source
    T0CONbits.PSA = 0; // 0 = Timer0 prescaler is assigned. 
    T0CONbits.T0PS = 0b000; // 1:2 prescale value
    INTCONbits.TMR0IE = 1; // Enable timer 0
    
    while (1) {
        if (PORTAbits.RA4 == 0) {
            // Change clock source (Primary / Internal)
            OSCCONbits.SCS = clock++ & 0b11; 
        }
        if (timer0_flag) {
            timer0_flag = FALSE;
            LATB ^= 1<<0; // Blink RB0 LED
        }
        LATB ^= 1<<3; // Blink RB0 LED
    }
}


//----------------------------------------------------------------------------
// High priority interrupt vector

#pragma code InterruptVectorHigh = 0x08
void InterruptVectorHigh (void) {
    _asm
    goto ISR //jump to interrupt routine
    _endasm
}

//----------------------------------------------------------------------------
// High priority interrupt routine

#pragma code
#pragma interrupt ISR

void ISR() {
    if (INTCONbits.TMR0IF) {
        /* Calculation for reset value of timer
         *   Overflow at 0xFFFF = 65535
         *   Number to count = 100/104.87 * 65535 = 62491
         *   0 to 65535 -> 104.87ms
         *   0 to 62491 -> 100ms
         *   3044 to 65535 -> 100ms
         */
        TMR0H = 3044 >> 8; // Reset timer
        TMR0L = 3044 & 0xFF; // Reset timer
        INTCONbits.TMR0IF = 0; //clear interrupt flag
        timer0_flag = TRUE;
    }
}