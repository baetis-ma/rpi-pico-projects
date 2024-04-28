#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "pico/multicore.h"
#include "hardware/structs/bus_ctrl.h"
#define CPUCLK  125.0            //in megahertz
//boards from seedi have lower speed prom
#define PICO_FLASH_SPI_CLKDIV 4

////////////////////////////////////////////
/////////// core1 logic analyzer ///////////
////////////////////////////////////////////

#include "logica_core1.h"

/////////////////////////////
/////////// core0 ///////////
/////////////////////////////

int main() {
    stdio_init_all();
    sleep_ms(3000);printf("waited 2 sec\n");

    multicore_launch_core1(core1_entry);

    //pins 28 and 29 test output pins
    gpio_set_function(28, GPIO_FUNC_PWM);
    gpio_set_function(29, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(28);
    pwm_set_wrap(slice_num, 12500);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 3125);
    pwm_set_chan_level(slice_num, PWM_CHAN_B, 6250);
    pwm_set_enabled(slice_num, true);
    //pinn 25 hooked to led
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);

    int cnt = 0;
    while(true) {
        sleep_ms(100);
        ++cnt;
        pwm_set_chan_level(slice_num, PWM_CHAN_A, 3125+100*(cnt%50));

    }
}
