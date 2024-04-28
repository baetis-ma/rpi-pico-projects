#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "i2s.pio.h"

#define NUMSAMP 1024
//#define NUMSAMP 512
#define SAMPLE_RATE 0.100
#define SAMPLE_FREQ 22050
#define PIO_SERIAL_CLKDIV  125000000/(8*32*SAMPLE_FREQ)
//#define PIO_SERIAL_CLKDIV  125000000/(4*32*SAMPLE_FREQ)

float output[NUMSAMP];
float input[NUMSAMP];
#include "./include/fft.h"
#include "./include/fft.c"
#include "./include/fft_calc.c"

//globals
int dma_chan;
uint32_t i2s_data[2*NUMSAMP];

bool  trig_wait = false;
void dma_int_handler() {
    dma_hw->ints0 = 1u << dma_chan; // Clear the interrupt
    dma_channel_set_write_addr(dma_chan, &i2s_data[0], true); //Reset write ptr and restart dma
    trig_wait = true;
}

int main() {
    stdio_init_all();
    int i2s_lr = 28;
    gpio_init(i2s_lr);
    gpio_set_dir(i2s_lr, GPIO_OUT);
    gpio_put(i2s_lr, 1);
    float next_time = 0.00;
    
    //setup pio output serialiser
    int i2s_ck = 7;
    int i2s_da = 2;
    int i2s_ws = 29;
    uint offset = pio_add_program(pio0, &i2s_program);
    i2s_program_init(pio0, 0, offset, i2s_ck, i2s_ws, i2s_da, PIO_SERIAL_CLKDIV);

    //setup dma channel
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_write_increment(&c, true); //increment pointer to memory array
    channel_config_set_read_increment(&c, false); //dont increment pio pointer (pio fifo addr fixed)
    channel_config_set_dreq(&c, DREQ_PIO0_RX0); //enable pio read dreq

    dma_channel_configure( dma_chan, &c,
        i2s_data ,         //dst
        &pio0_hw->rxf[0],  //src (from pio read pointer)
        2*NUMSAMP,         //fill array and trigger irq handler
        false              //Don't start yet
    );
    
    //setup irq and handler
    dma_channel_set_irq0_enabled(dma_chan, true);  //enable dma irq
    irq_set_exclusive_handler(DMA_IRQ_0, dma_int_handler); //set pico irq handler
    irq_set_enabled(DMA_IRQ_0, true);

    dma_int_handler(); // Manually call the handler once, to trigger the first transfer

    next_time = SAMPLE_RATE + (float)0.000001 * get_absolute_time(); 
    //absolute_time_t start;
    while (true) {
       if(next_time < (float)0.000001 * get_absolute_time()){
           trig_wait = false;
           while(!trig_wait){sleep_us(50);}

           for(int i=0; i<NUMSAMP; i++){
               i2s_data[1+2*i] = (i2s_data[1+2*i] << 2); 
               input[i] = ((float)(int)i2s_data[1+2*i])/(1<<26);
	           //hamming window
               //printf("%f ", input[i]);
	           float hammingwindow = 0.5435 + (1-0.5435) * (float)sin((2*3.14159*i)/NUMSAMP);
	           input[i] = hammingwindow * input[i];
	       } 
           //printf("\n");
	   
           fft_calc(input, output, NUMSAMP );

           for (int a = 1; a < NUMSAMP/2; a++){ printf ("%7.4f ", output[a]);} printf("\n");
           next_time = SAMPLE_RATE + next_time; 
       }
       sleep_us(100);
    }
}
