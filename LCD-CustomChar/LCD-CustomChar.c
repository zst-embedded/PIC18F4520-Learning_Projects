/* 
 * PICDEM 2 PLUS DEMO BOARD
 * PIC18F4520
 * 
 * This project is to create custom characters on an LCD display. This extends
 * from the LCD-HelloWorld project.
 *
 * On the board there is an 16x2 LCD display with the following connections:
 *     RD[0:3] -> Data Bus DB[4:7]
 *     RD4     -> RS (0 = Cmd, 1 = Data)
 *     RD5     -> RW (0 = Write, 1 = Read)
 *     RD6     -> En (Start data read-write)
 *     RD7     -> Vcc
 */

#include <p18f4520.h>
#include <GenericTypeDefs.h>
#include <delays.h>
#include "LCD-Lib.h"

const UINT8 arrow[] = {
    0b00100,
    0b01110,
    0b10101,
    0b00100,
    0b00100,
    0b10101,
    0b01110,
    0b00100
};

void main(void);
void createChar(UINT8 address, UINT8 line, UINT8 data);

void main(void) {
    UINT8 i;
    
    LCD_TRIS = 0;
    LCD_LAT_Vcc = 1; // Turn on LCD on the board
    
    Delay1KTCYx(100); // Delay before initialising display
    LCD_setup();
    
    LCD_clearDisplay();
    LCD_returnHome();
    
    /* Create double sided arrow */
    for (i = 0; i < 8; i++) {
        createChar(0, i, arrow[i]);
    }
    
    while (1) {
        char text1[] = " Custom Chars ";
        LCD_clearDisplay();
        LCD_writeChar(0x00); // Print arrow at CGRAM address 0x00
        LCD_puts(text1);
        LCD_writeChar(0x08); // Print same arrow (bit 3 has no effect)
        Delay10KTCYx(100);
    }
}


void createChar(UINT8 address, UINT8 line, UINT8 data) {
    /* Set CGRAM addess:
     * (bit 6) 1
     * (bit 5:3) character code address in DDRAM
     * (bit 2:0) line data address of character
     */
    LCD_writeCmd(1 << 6 | address << 3 | line);
    LCD_writeChar(data);
    
    /* Character code in DDRAM is 0x0000xxxx
     * Bit 7 to 4 is zero. Bit 3 has no effect.
     * 0x08 or 0x00 to select CGRAM character
     * pattern at address 0x00.
     */
}




