//PEAK DETECT when going up > 2 and goingdown == 2 
void peak_detect() {
            //    save all peak data and time, calc pulse and sp)2 and filter
            if((-1*ir_filt[array_ptr%ARRAY_LEN])>irmax) {
                    ++goingup; goingdown = 0;
                    irmax = -1*ir_filt[array_ptr%ARRAY_LEN]; 
            } else  {
                    irmax = 0.99 * irmax;
                    ++goingdown;
                    if (goingup > 2 && goingdown == 2) {
                        peak_timelast = peak_time;
                        peak_time = time_last + (i+1) * (time - time_last)/len;
                        peak_ir_ac = -1*ir_filt[array_ptr%ARRAY_LEN]; 
                        peak_ir_dc = ir_val[array_ptr%ARRAY_LEN]; 
             //           peak_red_ac = -1*red_filt[array_ptr%ARRAY_LEN]; 
                        peak_red_dc = red_val[array_ptr%ARRAY_LEN]; 
                        pulse = 60/(peak_time - peak_timelast);
                        spO2 = 103.0 - 17.0 * ((peak_ir_ac/peak_red_ac) / (peak_ir_dc/peak_red_dc)); 
                        if(peak_red_dc > 0.1 && pulse < 150 && pulse > 50) { 
                            pulseavg = 0.3 * pulse + 0.7 * pulseavg;
                            spO2avg = 0.3 * spO2 + 0.7 * spO2avg;
                            //printf("\n");
                            //printf(" %8.2fsec %12.1fbps %8.1f%%\n", peak_time, pulseavg, spO2avg);
                            //printf(" %8.2f %8.2f %8.2f %12.2f %8.2f\n", peak_time, pulse, pulseavg, spO2, spO2avg);
                            //printf(" %8.2f %8.2f %8.2f %12.2f\n", peak_time, pulse, pulseavg, spO2avg);
                            pulse_detected = 1;
                        } else  pulse_detected = 0;
                        goingup = 0;
                        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    } else cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            }

            if(pulse_detected == 1) printf("%10.3f %12.8f %12.8f 0\n", 
                time_last + (i+1) * (time - time_last)/len,
                red_filt[array_ptr%ARRAY_LEN],
                ir_filt[array_ptr%ARRAY_LEN]);
}
