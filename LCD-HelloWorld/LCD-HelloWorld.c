/* 
 * PICDEM 2 PLUS DEMO BOARD
 * PIC18F4520
 * 
 * This project is to create a library to use an LCD display from scratch.
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

// Define pin connections
#define LCD_TRIS (TRISD)
#define LCD_LAT_Vcc (LATDbits.LATD7) 
#define LCD_LAT_EN (LATDbits.LATD6) 
#define LCD_LAT_RW (LATDbits.LATD5)
#define LCD_LAT_RS (LATDbits.LATD4)
#define LCD_LAT_DATA(x) (LATD = (LATD & 0xF0) | (x & 0x0F) )

// Function prototype
void LCD_sendByte(UINT8 x);
void LCD_writeCmd(UINT8 x);
void LCD_writeChar(UINT8 x);
void LCD_setup();
void LCD_setCursor(UINT8 line, INT8 position);
void LCD_shiftAddress(BOOL shift, BOOL left);
void LCD_returnHome();
void LCD_clearDisplay();

// Define LCD delay
#define LCD_delay() Delay10TCYx(100)


void LCD_setup() {
    /* Initialize 4-bit mode */
    LCD_writeCmd(0x32);
     
    /* Function set:
     * (bit 5) 1
     * (bit 4) 1 = 8-bit mode, 0 = 4-bit mode
     * (bit 3) 1 = 2 line, 0 = 1 line
     * (bit 2) 1 = 5x10 font, 0 = 5x7 font
     */
    LCD_writeCmd(0x28); // 0b00101000 = 4 bit, 2 line, 5x7;
    
    /* Entry mode set:
     * (bit 2) 1
     * (bit 1) cursor move increment
     * (bit 0) 1 = accompanies data shift, 0 = cursor fixed position
     */
    LCD_writeCmd(0b0110);
    
    /* Display on/off control:
     * (bit 3) 1
     * (bit 2) Display on
     * (bit 1) cursor on
     * (bit 0) cursor blink
     */
    LCD_writeCmd(0b1100 /*| 0b10 | 0b1 */);
}

void LCD_writeCmd(UINT8 x) {
    LCD_LAT_RS = 0; // Select Cmd Register
    LCD_sendByte(x);
}

void LCD_writeChar(UINT8 x) {
    LCD_LAT_RS = 1; // Select Data Register
    LCD_sendByte(x);
}

void LCD_sendByte(UINT8 x) {
    LCD_LAT_RW = 0; // Write operation
    LCD_delay();
    
    LCD_LAT_EN = 1;
    LCD_LAT_DATA(x >> 4); // send 4 high order bits
    LCD_delay();
    LCD_LAT_EN = 0;
    LCD_delay();
    
    LCD_LAT_EN = 1;
    LCD_LAT_DATA(x); // send 4 low order bits
    LCD_delay();
    LCD_LAT_EN = 0;
    LCD_delay();
    
    LCD_LAT_RW = 1; // Mark end of write operation
    LCD_delay();
}

void LCD_setCursor(UINT8 line, INT8 position) {
    /* Set DDRAM address:
     * Line 0 starts at 0x00
     * Line 1 starts at 0x40
     */
    LCD_writeCmd(0x80 | (line*0x40 + position));
}

void LCD_shiftAddress(BOOL shift, BOOL left) {
    /* Cursor or display shift:
     * (bit 3) SC / 1 = display shift, 0 = cursor move
     * (bit 2) RL / 1 = right, 0 = left
     */
     LCD_writeCmd( 0x10 | (left ? 0 : 0b100) | (shift ? 0b1000 : 0) );
}

void LCD_returnHome() {
    /* Return home:
     * Returns display from being shifted to original position
     * Set cursor to position 0
     */
     LCD_writeCmd(0b10);
}

void LCD_clearDisplay() {
    /* Clear display */
    LCD_writeCmd(0b0001);
}


void LCD_puts(char * txt) {
    UINT8 i = 0;
    while (txt[i] != '\0') {
        LCD_writeChar(txt[i++]);
    }
}

void main(void) {
    LCD_TRIS = 0; // Set LCD port as output
    LCD_LAT_Vcc = 1; // Turn on LCD on the board
    
    Delay1KTCYx(100); // Delay before initialising display
    LCD_setup();
    
    LCD_clearDisplay();
    LCD_returnHome();

    while (1) {
        char text1[] = "Hello :)", text2[] = ":P World";
        UINT8 i = 0;
        for (i = 0; i < 25; i++) {
            LCD_clearDisplay();
            
            // Shift 1st line to right
            LCD_setCursor(0, i);
            LCD_puts(text1);

            // Shift 2nd line to left
            LCD_setCursor(1, 16 - i);
            LCD_puts(text2);
            
            Delay10KTCYx(30);
        }
    }
}




