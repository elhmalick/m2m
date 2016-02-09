/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Author: Olivier Gruber (olivier dot gruber at acm dot org)
*/

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

#define COM1 ((uint16_t)0x3f8)
#define COM2 ((uint16_t)0x2f8)
#define screenWidth 80
#define rows 25
#define column 80
#define coinBasDroite 0xB8000+(screenWidth*2)*2


static __inline __attribute__((always_inline, no_instrument_function))
uint8_t inb(uint16_t port) {
  uint8_t data;
  __asm __volatile("inb %w1,%0" : "=a" (data) : "d" (port));
  return data;
}

static __inline __attribute__((always_inline, no_instrument_function))
void outb(uint16_t port, uint8_t data) {
  __asm __volatile("outb %0,%w1" : : "a" (data), "d" (port));
}


static void serial_init(uint16_t port) {

  outb(port+1,0x00);    // Disable all interrupts
  outb(port+3,0x80);    // Enable DLAB (set baud rate divisor)

  outb(port+0,0x01);  // Set divisor to 1 (lo byte) 115200 baud
  outb(port+1,0x00);  //                0 (hi byte)

  outb(port+3,0x03);    // 8 bits, no parity, one stop bit
  outb(port+2,/*0xC7*/ 0x00);    // Enable FIFO, clear them, with 14-byte threshold
  outb(port+4,/*0x0B*/ 0x08);    // IRQs enabled, RTS/DSR set

  // outb(port+1,0x0d); // enable all intrs but writes
}



static
void serial_write_char(uint16_t port, char c) {
  while ((inb(port + 5) & 0x20) == 0);
  outb(port,c);
}

static
void serial_write_string(uint16_t port, const unsigned char *s) {
  while(*s != '\0') {
    serial_write_char(port,*s);
    s++;
  }
}

char serial_read(uint16_t port) {
   while ((inb(port + 5) & 1) == 0);
   return inb(port);
}

/*
 * See:
 *   http://wiki.osdev.org/Printing_To_Screen
 *   http://wiki.osdev.org/Text_UI
 *
 * Note this example will always write to the top
 * line of the screen.
 * Note it assumes a color screen 0xB8000.
 * It also assumes the screen is in text mode,
 * and page-0 is displayed.
 * Screen is 80x25 (80 characters, 25 lines).
 *
 * For more info:
 *   http://wiki.osdev.org/VGA_Resources
 */
 //volatile char *video = (volatile char*)0xB8000;
 
 char *video = (char *)0xB8000;
 
 void write_string( int colour, const char *string ) {
    
    while( *string != 0 ) {
        if(*string=='\n'){
           video += screenWidth*2 - ((int)(video-0xB8000) % (screenWidth*2));
           *string++;
       } else{
          *video++ = *string++;
          *video++ = colour;
        }    
    }
}

#define coinBasDroite 0xB8000 + 2*column*2
void scroll(){

    char *tmp;
    for(tmp=(char *)0xB8000;tmp< 0xB8000 + 2*column;tmp++){
        *tmp=*(tmp+2*column);
    }
    video=video-2*column;

}


void flecheGauche(){
    if(video !=(char*)0xB8000 )
    video -= 2;
}


void flecheDroite(){
    if(video != coinBasDroite)
        video += 2;

}


void flecheHaut(){
    if(video > 0xB8000 + 2*column)
      video-=2*screenWidth;
    
}

#define ligne 3
void flecheBas(){
    if(video < 0xB8000 + 2*column*(ligne-1))
       video+=2*screenWidth;
      
  }


int i=0;
void kputchar(int c, void *arg) {
  serial_write_char(COM2,c);
}


void kmain(void) {

  //write_string(0x2a,"Console greetings!");

  serial_init(COM1);
  serial_write_string(COM1,"\n\rHello!\n\r\nThis is a simple echo console... please type something.\n\r");

  serial_write_char(COM1,'>');
  
  char s[2];
  while(1) {
    /*unsigned*/ char c;
    c=serial_read(COM1);
      switch(c){    
          case 13: // touche entrée
              c = '\r';
              serial_write_char(COM1,c);
              c = '\n';
              serial_write_char(COM1,c);
              serial_write_char(COM1,'>');
              s[0] = c;
              s[1] = 0;
              write_string(0x2a, s);
              break;
          case '\033': //entrée fleche de direction
                c = serial_read(COM1);
                
               // if(c=='['){
                    c = serial_read(COM1); // Je refais un read pour evacuer le '['
                    
                    if(c=='D')  // fleche gauche
                        //video-=2;
                        flecheGauche();
                    if(c=='C')   //67 fleche droite
                        //video+=2;
                        flecheDroite();
                    if(c=='B')  //65 fleche bas
                        //video+=2*screenWidth;
                         flecheBas(); 
                    if(c=='A')  //66 fleche haut
                        //video-=2*screenWidth;
                        flecheHaut();

                        
                 
                break;   
            case 127:
                video= video -2;//flecheGauche();
                break;
          default:
            if(video == coinBasDroite)
                scroll(video);
                
              serial_write_char(COM1,c);
              s[0] = c;
              s[1] = 0;
              write_string(0x2a, s); 
            }
     }

}
