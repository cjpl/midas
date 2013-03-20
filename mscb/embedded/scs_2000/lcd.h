/********************************************************************\

  Header file for LCD functions for four line text LCD

\********************************************************************/

void lcd_setup();
void lcd_clear();
void lcd_line(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2);
void lcd_goto(char x, char y);
char putchar(char c);
void lcd_puts(char *str);
void lcd_cursor(unsigned char flag);

