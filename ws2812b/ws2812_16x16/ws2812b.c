#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812b.pio.h"
#include "turbo_clormap.c"

#define NUM_PIXELS 256

#define OUT_PIN 29
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

  time_t t;
  srand((unsigned) time(&t));
  absolute_time_t otime = get_absolute_time();

  printf("starting %d\n", __RAND_MAX);
  int cnt = 0;
  int dist;
  int dim = 16;
  int res = 0;
  float x, y, randx = 0, randy = 0;
  while (true) {
        ++cnt;
        for(int xx = -8; xx < 8; xx++) {
            for(int yy = -8; yy < 8; yy++) {
               randx = randx + ((rand()%50001)-25000)/10000000.0;
               if(randx > 12.00) { randx = 6.00; srand((unsigned) time(&t)); randx = 0;++res; }
               if(randx < -12.00) { randx = -6.00; srand((unsigned) time(&t)); randx = 0;++res; }
               randy = randy + ((rand()%50001)-25000)/10000000.0;
               if(randy > 12.00) { randy = 6.00; srand((unsigned) time(&t)); randy = 0;++res; }
               if(randy < -12.00) { randy = -6.00; srand((unsigned) time(&t)); randy = 0;++res; }
               x = xx + randx;
               if((xx%2) == 0) y = -1 + (-1 *  yy) + randy;
                          else y = yy + randy;
               float dist = sqrt(x*x+y*y);
               if (dist == 0) dist = 0.0000000001;
               float sincp = sin(dist/2.0)/(dist/2.0);
               int col = (int)((256/1.22)*(0.22+sincp));
               //int col = (int)50*((sqrt(x*x+y*y))) ;
               if(col > 256) col = 256;
               int color = ((turbo[3*col+0]/dim)<<24)+ 
                           ((turbo[3*col+1]/dim)<<16)+ 
                           ((turbo[3*col+2]/dim)<<8); 
               pio_sm_put_blocking(pio0, 0, color);

            }
        }
        sleep_ms(2);
        if(cnt%100 == 0) printf("%10.1fsec   %5.1f  %5.1f %3d  %d\n", 0.000001* get_absolute_time(),
                                   randx, randy, res, cnt);
  }

  return 0;
}
