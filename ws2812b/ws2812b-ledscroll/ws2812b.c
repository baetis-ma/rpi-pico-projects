#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812b.pio.h"
#include "books.h"
#include "fonttable.h"
#include "tables.h"

#define NUM_PIXELS 256

#define OUT_PIN 16
#define TEXT river

int main()
{
  stdio_init_all();
  //sleep_ms(2000);

  PIO pio = pio0;
  if(set_sys_clock_khz(128000, false)) printf("clock set to %ld\n", clock_get_hz(clk_sys));
  uint offset = pio_add_program(pio, &ws2812b_program);
  uint sm = pio_claim_unused_sm(pio, true);
  ws2812b_program_init(pio, sm, offset, OUT_PIN);

  int pointer = 0, start = 0, cnt = 0;
  int textcol;
  int dotcolor = 0x04101000, backcolor = 0x01010000;
  float pct;
  absolute_time_t otime = get_absolute_time();;

  printf("starting\n");
  while (true) {
        //this whole block prints out status and resets end of string
        if(++cnt%(10000) == 0) { 
            pct = 100.0*(pointer/40)/strlen(TEXT);
            if(pct > 100) { pointer = 0; start = 0; otime = get_absolute_time(); }
            float htime = (float) (0.000001 * ((get_absolute_time()-otime)/3600));
            printf("%6.1f%c  ", pct - 0.1, 0x25);
            printf(" %2dh%02dm /", (int)htime, (int)(60 * (htime - (int)htime)));
            printf(" %2dh%02dm    ", (int)(htime*((100.0-pct)/pct)),
                    (int)(60*(htime*((100.0-pct)/pct) - (int)((htime*((100.0-pct)/pct))))), pointer);
            int dots = 20;
            for(int n = 0; n < dots; n++) if((100*n)/dots  < (pct-(50/dots))) printf("*"); else printf("-"); 
            printf("\n");
        }
        for(int a = 0; a < 5; a++) {
            textcol = pointer/40;
            textcol = textcol%strlen(TEXT);
            for(int b = 0; b < 8; b++) {
                //if(fonttable[((pointer/8)%5) + 5*(TEXT[textcol]-0x20)] != 0 )
                    if(fonttable[(((pointer/8)%5) + 5*((TEXT[textcol]%0x80)-0x20))] & (1 << (7-b))) { 
                        pio_sm_put_blocking(pio0, 0, dotcolor); 
                    } else  pio_sm_put_blocking(pio0, 0, backcolor); 
            }
            if(((pointer/8)%5) == 4) {
                //add space around around active columns in font of each char 
                if(fonttable[4 + 5*(TEXT[textcol]-0x20)] != 0)
                    for(int w = 0; w < 8; w++)  pio_sm_put_blocking(pio0, 0, backcolor); 
                if(fonttable[4 + 5*(TEXT[1+textcol]-0x20)] != 0)
                    for(int w = 0; w < 8; w++)  pio_sm_put_blocking(pio0, 0, backcolor); 
                //when display full, index start point and wait
                if((pointer - (8 * start))>NUM_PIXELS) { 
                    pointer = 8 * ++start; 
                    sleep_ms(30); 
                }
            }
            pointer = pointer + 8;
        }
  }
  return 0;
}
