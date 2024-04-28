#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/multicore.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#define ARRAY_LEN 256
#include "dsp/chebbpf.h"
#include "pico/cyw43_arch.h"
#define USB_MODE 1

//boards from seedi have lower speed prom
//#define PICO_FLASH_SPI_CLKDIV 4

#define I2C_N          i2c0
#define I2C_SDA        21
#define I2C_SCK        20
#define MAX30102_ADDR  0x57
//#define I2C_POWER  17
#include "./include/i2c.h"

//MAX30100 Registers
#define FIFO_RD_PTR     0x02
#define FIFO_WR_PTR     0x04
#define FIFO_DATA       0x05
#define MODE_CONFIG     0x06
#define SPO2_CONFIG     0x07
#define LED_CONFIG      0x09
#define TEMP_INT        0x16
#define TEMP_FRAC       0x27
#define REV_ID0         0xFE
#define REV_ID1         0xFF

/////////////////////////////
/////////// core1 ///////////
/////////////////////////////
void core1_entry() {
    i2c_start();
    i2c_scan();
    //MAX30102 Configuration
    float temperature;
    uint8_t data[128];
    data[0] = MODE_CONFIG;
    data[1] = (0<<7) + (0<<6) + (0<<3) + 3; //mode reg - shutdown, reset, temp, mode (3=spo2)
    data[2] = (1<<6) + (1<<2) + 1; //spo2 reg - dac res, rate(1=100), pulse(1=118)
    i2c_write_blocking(I2C_N, 0x57, data, 3, false);
    data[0] = LED_CONFIG;
    data[1] = 0xc4;    //ir - red 
    i2c_write_blocking(I2C_N, 0x57, data, 2, false);

    //read product id
    data[0] = REV_ID0;
    i2c_write_blocking( I2C_N, 0x57, data, 1, true);
    i2c_read_blocking( I2C_N, 0x57, data, 2, false);
    printf("Produnct ID = %d %d\n", data[1], data[0]);

    //read temperature
    data[0] = MODE_CONFIG;
    data[1] = (0<<7) + (0<<6) + (1<<3) + 3; //mode reg - shutdown, reset, temp, mode (3=spo2)
    i2c_write_blocking(I2C_N, 0x57, data, 2, false);
    sleep_us(2);
    data[0] = TEMP_INT;
    i2c_write_blocking( I2C_N, 0x57, data, 1, true);
    i2c_read_blocking( I2C_N, 0x57, data, 2, false);
    temperature = 32 + 1.8 * data[0] + 1.8 * 0.0625 * data[1];
    printf("Temperature = %7.2f\n", temperature);

    float red_val[ARRAY_LEN], ir_val[ARRAY_LEN], red_filt[ARRAY_LEN], ir_filt[ARRAY_LEN];
    int array_ptr = 0;
    float peak_red_ac, peak_red_dc, peak_ir_ac, peak_ir_dc;
    float pulse, pulseavg = 76.5, irmax=0, irmaxtime, spO2, spO2avg = 95.3;
    float ptrpulse, ptrpulseavg = 76.5;
    float time_last, time, peak_time = .5, peak_timelast; 
    uint32_t peak_ptr, peak_ptr_last;
    int goingup=0, goingdown=0, pulse_detected = 0;

    absolute_time_t start = get_absolute_time();
    while (1) {
        //read fifo - first find len
        data[0] = FIFO_RD_PTR;
        i2c_write_blocking( I2C_N, 0x57, data, 1, true);
        i2c_read_blocking( I2C_N, 0x57, data, 3, false);
        size_t len = (data[0]%16) - data[2]%16;
        if ((int)len <= 0) len = len + 16; //if wrap

        time_last = time;
        time = 0.000001 * absolute_time_diff_us(start, get_absolute_time());
        //read block of data from fifo, time calc assumed time interval / len times n
        data[0] = FIFO_DATA;
        //READ DATA
        i2c_read_blocking( I2C_N, 0x57, data, 4*len, false);
        //parse, dsp, store in circular array
        multicore_fifo_push_blocking(10000000+1000*(int)(10*pulseavg)+(int)(10*spO2avg));
        for(int i=0; i<len;i++) {
            red_val[array_ptr%ARRAY_LEN] = ((float)256*data[4*i+0]+data[4*i+1])/(1<<16);
            red_filt[array_ptr%ARRAY_LEN] = red_bpf(red_val[array_ptr%ARRAY_LEN]);
            ir_val[array_ptr%ARRAY_LEN] = ((float)256*data[4*i+2]+data[4*i+3])/(1<<16);
            ir_filt[array_ptr%ARRAY_LEN] = ir_bpf(ir_val[array_ptr%ARRAY_LEN]);
            uint32_t irint = (uint32_t)((100000*ir_filt[array_ptr%ARRAY_LEN])+5000);
            if (irint < 0) irint = 0; if (irint > 10000) irint = 9999;
            //printf("%10.5f  %d\n", ir_filt[array_ptr%ARRAY_LEN], irint);
            multicore_fifo_push_blocking(irint);

            //PEAK DETECT when going up > 2 and goingdown == 2 
            if((-1*ir_filt[array_ptr%ARRAY_LEN])>irmax) {
                    ++goingup; goingdown = 0;
                    irmax = -1*ir_filt[array_ptr%ARRAY_LEN]; 
            } else  {
                    irmax = 0.99 * irmax;
                    ++goingdown;
                    if (goingup > 2 && goingdown == 2) {
                        peak_timelast = peak_time;
                        peak_ptr_last = peak_ptr;
                        peak_ptr = array_ptr;
                        peak_time = time_last + (i+1) * (time - time_last)/len;
                        peak_ir_ac = -1*ir_filt[array_ptr%ARRAY_LEN]; 
                        peak_ir_dc = ir_val[array_ptr%ARRAY_LEN]; 
                        peak_red_ac = -1*red_filt[array_ptr%ARRAY_LEN]; 
                        peak_red_dc = red_val[array_ptr%ARRAY_LEN]; 
                        pulse = 60/(peak_time - peak_timelast);
                        ptrpulse = 60/(0.01 * peak_ptr - 0.01 * peak_ptr_last);
                        spO2 = 103.0 - 17.0 * ((peak_ir_ac/peak_red_ac) / (peak_ir_dc/peak_red_dc)); 
                        if(peak_red_dc > 0.1 && pulse < 140 && pulse > 50) { 
                            pulseavg = 0.3 * pulse + 0.7 * pulseavg;
                            ptrpulseavg = 0.3 * ptrpulse + 0.7 * ptrpulseavg;
                            spO2avg = 0.3 * spO2 + 0.7 * spO2avg;
                        } 
                        goingup = 0;
                        //printf("pulseavg %7.1f %7.1f spo2 %7.1f      %f\n",  pulseavg, ptrpulseavg, spO2avg,peak_red_dc); 
                    } 
            }
            array_ptr++;
        }
        sleep_ms(80);    //above loop takes about 20msec repeat about 10hx avioding fifo overflow
    }
}


/////////////////////////////////////////////////
///////////// core0 and tcp stack ///////////////
/////////////////////////////////////////////////
//#include "pico/cyw43_arch.h"
char tcpresponse[128];
char outstring[2048];
uint32_t result_array[ARRAY_LEN+3];
uint32_t start_ptr = 0, end_ptr;
float pulse, spO2;
#include "./include/tcp_server.c"
#include "./include/core_fifo.c"

int main() {
    //int runload = 0;
    stdio_init_all(); //setup usb serial

    if(USB_MODE == 0) {
        //setup wifi
        if (cyw43_arch_init()) { printf("failed to initialise\n"); return 1; }
        cyw43_arch_enable_sta_mode();

        if (cyw43_arch_wifi_connect_timeout_ms("mutantbunny", "password", CYW43_AUTH_WPA2_AES_PSK, 30000)) {
            printf("Failed to connect to wi-fi.\n");
            return 1;
        } else { printf("\nConnected to wi-fi ip address : %s:80\n",
                        ip4addr_ntoa(netif_ip4_addr(netif_list))); }
    }

    //keeps messages coherant between processors also sets up intrupts not being used
    core0_array_setup(result_array);
    sleep_ms(100); // Wait for a bit for things to happen

    while (1) {
       pulse = 0.1 * ((result_array[ARRAY_LEN+1]/1000)%10000);
       spO2  = 0.1 * (result_array[ARRAY_LEN+1]%1000);
       end_ptr = result_array[ARRAY_LEN+2];

       //run tcp server
       if(USB_MODE == 0) run_tcp_server_start();
       else { 
           printf("0,%d,%d,%.1f,%.1f", start_ptr, end_ptr, pulse, spO2);
           //printf("start_ptr = %d end_ptr =  %d\n", start_ptr, end_ptr);
           while(start_ptr < end_ptr) {
               printf(",%d",result_array[start_ptr%ARRAY_LEN]);
               ++start_ptr;
           }
           printf("\n");
       }

       //printf("-----tcp rsp = %s\n", tcpresponse);
       sleep_ms(100);
   }
   cyw43_arch_deinit();
   return(0);
}

