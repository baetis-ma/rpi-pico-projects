#include "highspeed.pio.h"
#define NUMSAMP 50000
#define CPUCLK  125.0            //in megahertz
bool  tlevel = 0;
char  labels[16][16] = {"pin28","pin29","bus"};
int   nlabels = 3;
int   col[128] = { 28, -1, 29, -1, 28, 29, -1 };
int   mask =(1ul << 28) | (1ul << 29);
int   numcoll = 7; 
int   trig_pin = 29;
int   samp_num = 2000, trig_addr;
int   pause = 0;
float update_rate = 0.5; 
float sample_rate = 0.1 * CPUCLK;    //1us
float trig_off = 0.1; 
void menu();
/////////////////////////////
/////////// core1 ///////////
/////////////////////////////
void core1_entry() {
    uint32_t buffer[NUMSAMP];
    bool onoff = 0;

    //setup pio 
    uint offset = pio_add_program(pio0, &highspeed_program);
    //highspeed_program_init(pio0, 0, offset, PIO_SERIAL_CLKDIV);

    //setup dma channel
    int dma_chan;
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_write_increment(&c, true); //increment pointer to memory array
    channel_config_set_read_increment(&c, false); //dont increment pio pointer (pio fifo addr fixed)
    channel_config_set_dreq(&c, DREQ_PIO0_RX0); //enable pio read dreq

    dma_channel_configure(dma_chan, &c,
        buffer,            //dst
        &pio0_hw->rxf[0],  //src (from pio read pointer)
        NUMSAMP,           //fill array
        false              //Don't start yet
    );
    
    uint32_t last = 0;
    while (true) {
       //change pio clock, activate dma transfer and wait for it to end
       highspeed_program_init(pio0, 0, offset, sample_rate);
       sleep_us(5);
       dma_channel_set_write_addr(dma_chan, &buffer[0], true); 
       dma_channel_wait_for_finish_blocking(dma_chan);

       //find trigger in collected samples
       trig_addr = 0;
       for(int n = 0; n < NUMSAMP; n++){
          if(trig_addr == 0 && n > (int)(trig_off*samp_num) && 
	          !!((1ul << trig_pin) & buffer[n]) == tlevel && !!((1ul << trig_pin) & last) == !tlevel && 
		  trig_pin >= 0)  trig_addr = n;
          //if(trig_addr == 0 && n > (int)(trig_off*samp_num) && !!((1ul << trig_pin) & buffer[n]) == 1 &&  
                  //!!((1ul << trig_pin) & last) == 0 && trig_pin >- 0)  trig_addr = n;
          last = buffer[n];
	      if (trig_addr > 0) break;
       }

       //header
       int off;
       if (pause == 0) { 
          printf("highspeed samp=%d rate=%.3fus trigpin=%d trigoff=%.2f header ",
              samp_num, sample_rate/CPUCLK, trig_pin, trig_off);
          for(int n = 0; n < nlabels; n++) printf(" %s",labels[n]); printf("\n");

          //output data

          if(trig_addr > 0)off = (int)(trig_off * samp_num); else off = 0;
          for(int n = 0; n < samp_num; n++){
             if(n == samp_num -1 || last != (mask & buffer[n + (trig_addr - off)])) {
                printf("%7d  ", n);
  	        for(int nn = 0; nn < numcoll; nn++){
                   if(col[nn]>=0)printf("%c", '0' + !!((1ul << col[nn]) & buffer[n+(trig_addr-off)]));
                   else printf(" "); 
                } 
	        printf("\n");
             } 
             last = mask & buffer[n + (trig_addr - off)];
          } 
          printf("e\n");
       }
       menu();
       sleep_ms((int)1000*update_rate);
       //toggle pin 25 with each sample completed
       gpio_put(25, onoff = !onoff);
    }
}

void menu(){
   #define ENDSTDIN    255
   #define CR           13
   char strg[128];
   char chr;
   int strglen, lp = 0, lcptr;
   chr = getchar_timeout_us(0);
   while(chr != ENDSTDIN) {
      strg[lp++] = chr;
      sleep_us(10000);
      //printf("0x%02x  %d %s\n", chr, lp, strg);
      if(chr == CR) {
         strg[lp] = EOF;  //terminate string
         strglen = lp;
         //printf("temp %d %s\n", lp, strg);

         lp = 0;
         if(strncmp(strg, "help", 4) == 0) {
            printf("# command list:\n");
            printf("# rate <d>           -  sample rate in microseconds - float > 0.02\r\n");
            printf("# update <d>         -  update rate in seconds - float\r\n");
            printf("# samples <d>        -  number of samples\r\n");
            printf("# trig <d>           -  trigger pin - -num no trigger\r\n");
            printf("# tlevel <d>         -  toggle trigger level rising/falling\r\n");
            printf("# toff <d>           -  trigger offset float from 0.0 to 1.0\r\n");
            printf("# col <d> <d> ...    -  output column pin assignment-bus sep by ,\r\n");
            printf("# label <s> <s> ...  -  label assigned to pin column groups\r\n");
            printf("# pause              -  toggle pause output\r\n");
         }
         if(strncmp(strg, "rate", 4) == 0) { 
            float temp;
            sscanf(strg, "rate %f", &temp);
            sample_rate = CPUCLK * temp;
         }
         if(strncmp(strg, "update", 6) == 0) { sscanf(strg, "update %f", &update_rate); }
         if(strncmp(strg, "samples", 7) == 0) { sscanf(strg, "samples %d", &samp_num); }
         if(strncmp(strg, "trig", 4) == 0) { sscanf(strg, "trig %d", &trig_pin); }
         if(strncmp(strg, "tlevel", 6) == 0) { tlevel = !tlevel; }
         if(strncmp(strg, "toff", 4) == 0) { sscanf(strg, "toff %f", &trig_off); }
         if(strncmp(strg, "pause", 5) == 0) { if(pause == 0) pause = 1; else pause = 0; }
         if(strncmp(strg, "col", 3) == 0) {
            numcoll = 0;
            for(int n = 0; n < 32; n++)col[n] = -1;
            lcptr = strlen("col") + 1;
            while (lcptr < strglen - 1) {
               if(strg[lcptr] >= '0' && strg[lcptr] <= '9'){
                  col[numcoll] = strg[lcptr] - '0';
                  ++lcptr;
                  if(strg[lcptr] >= '0' && strg[lcptr] <= '9'){
                     col[numcoll] = 10 * col[numcoll] + strg[lcptr] - '0';
                     ++lcptr;
                  }
                  numcoll = 2 + numcoll;
               } else
               if(strg[lcptr] == ','){ --numcoll; ++lcptr; }
               else ++lcptr;
            }
	    //for(int n =0; n < numcoll; n++)printf("%5d %d\n", n, col[n]);
	    mask = 0; for(int n = 0; n < numcoll; n++) mask = mask | (1ul << col[n]); 
         }
         if(strncmp(strg, "label", 5) == 0) {
            nlabels = 0;
            lcptr = strlen("label") + 1;
            while (lcptr < strglen -1) {
               sscanf(strg+lcptr, "%s", labels[nlabels]);
               lcptr = lcptr + strlen(labels[nlabels]) + 1;
               ++nlabels;
            }
         }
         break;
      }
      chr = getchar_timeout_us(0);
   }
}
