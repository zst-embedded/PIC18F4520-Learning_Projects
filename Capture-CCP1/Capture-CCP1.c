/* 
 * PICDEM 2 PLUS DEMO BOARD
 * PIC18F4520
 * 
 * This project is to make use of the Capture mode in CCP1 to take
 * a square wave as an input and display the frequency of the wave
 * on an LCD screen. 
 *
 * A square wave is pumped in on pin RC2. A 10k resisor must be placed
 * in series with the input function generator signal. The jumper J9
 * on the board must be removed as it is connected to the buzzer
 *
 * On the board there is an 16x2 LCD display with the following connections:
 *     RD[0:3] -> Data Bus DB[4:7]
 *     RD4     -> RS (0 = Cmd, 1 = Data)
 *     RD5     -> RW (0 = Write, 1 = Read)
 *     RD6     -> En (Start data read-write)
 *     RD7     -> Vcc
 *
 * The clock is set to HS mode with a 10MHz crystal
 *     Fosc / 4 = 2.5MHz
 */

#include <p18f4520.h>
#include <GenericTypeDefs.h>
#include <delays.h>
#include "LCD-Lib.h"
#include <stdio.h>

#define CLK_CYCLE_FREQ (2.5e6) // Fosc / 4 = 2.5MHz
#define CLK_CYCLE_TIME (400.0e-9) // Fosc / 4 -> -> Tcy = 400ns

/*  CCP1CONbits.CCP1M
    0100 = Capture mode, every falling edge
    0101 = Capture mode, every rising edge
    0110 = Capture mode, every 4th rising edge
    0111 = Capture mode, every 16th rising edge */
#define CCP_EDGE_CONFIG (0b111)
#define CCP_EDGE_VALUE  (16.0)

// Function Prototype
void main(void);
void InterruptHandlerHigh(void);

UINT16 CCP1_Value = 0;

void main(void) {
    // LED on RB0
    TRISBbits.TRISB0 = 0;
    
    /***************************************************************************/
    // Setup RC2/CCP1 and Timer1
    /***************************************************************************/
    TRISCbits.TRISC2 = 1; // RC2 as input
    
    T3CONbits.T3CCP1 = 0; // Use Timer 1 for all CCP
    T3CONbits.T3CCP2 = 0; // Use Timer 1 for all CCP
    
    CCP1CONbits.CCP1M = CCP_EDGE_CONFIG; // Set to CCP Capture mode
    
    /* Enable global interrupts */
    INTCONbits.GIEH = 1;
    INTCONbits.GIEL = 1; 
    
    /* CCP1 Interrupt */
    PIE1bits.CCP1IE = 1; // Enable CCP1 interrupt
    PIR1bits.CCP1IF = 0; // Clear flag
    
    /* Timer1 */
    T1CONbits.T1CKPS = 0b00; // 1:1 Prescale value
    T1CONbits.TMR1ON = 1; // 1 = Enables Timer1 
    
    /***************************************************************************/
    // Setup LCD
    /***************************************************************************/
    LCD_TRIS = 0;
    LCD_LAT_Vcc = 1; // Turn on LCD on the board
    
    Delay1KTCYx(100); // Delay before initialising display
    LCD_setup();
    
    LCD_clearDisplay();
    LCD_returnHome();
    
    while (1) {
        char buf[20], buf1[20];
        float CCP_cycles = CCP1_Value / CCP_EDGE_VALUE;
        
        // Period in ms (CCP cycles time * 10^3 to get period in millisec)
        float period = CCP_cycles * CLK_CYCLE_TIME * 1e3;
        UINT32 period_int = period;
        UINT32 period_frac = (period - period_int) * 10000;
        
        // Frequency in Hz
        float freq = CLK_CYCLE_FREQ / CCP_cycles;
        UINT32 freq_int = freq;
        UINT32 freq_frac = (freq - freq_int) * 100;
        
        //sprintf (buf, "CCP1 = %u     ", CCP1_Value); // Print CCP1 register value in decimal
        //sprintf (buf, "CCP1 = %#06x", CCP1_Value); // Print CCP1 register value in hexadecimal
        sprintf (buf, "f = %lu.%02lu Hz     ", freq_int, freq_frac);
        sprintf (buf1, "t = %lu.%04lu ms     ", period_int, period_frac);
        
        LCD_setCursor(0, 0);
        LCD_puts(buf);
        LCD_setCursor(1, 0);
        LCD_puts(buf1);
        
        Delay10KTCYx(10);
    }
}


//----------------------------------------------------------------------------
// High priority interrupt vector

#pragma code InterruptVectorHigh = 0x08
void InterruptVectorHigh() {
    _asm
    goto InterruptHandlerHigh //jump to interrupt routine
    _endasm
}

//----------------------------------------------------------------------------
// High priority interrupt routine

#pragma code
#pragma interrupt InterruptHandlerHigh

void InterruptHandlerHigh() {
    if (PIR1bits.CCP1IF) {                 
        CCP1_Value = CCPR1; // Save CCP value
        TMR1L = 0;
        TMR1H = 0; // Reset timer value used for CCP
        PIR1bits.CCP1IF = 0; //clear interrupt flag
        LATB ^= 1; // (DEBUG) Toggle RB0 LED
    }
}

//----------------------------------------------------------------------------