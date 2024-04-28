//sets up interupt contollers in each core to receive
//string via core to core fifo

//static void core1_entry();

//static void core1_str_setup(char *outstring) {
//    void core1_sio_irq() {
//        int cnt = 0;
//        while (multicore_fifo_rvalid()){
//            while(1) {
//                outstring[cnt] = (char)multicore_fifo_pop_blocking();
//                if (outstring[cnt] == 0) {cnt = 0; break; }
//                ++cnt;
//            }
//        }
//        multicore_fifo_clear_irq();
//    }
//    multicore_fifo_clear_irq();
//    irq_set_exclusive_handler(SIO_IRQ_PROC1, core1_sio_irq);
//    irq_set_enabled(SIO_IRQ_PROC1, true);
//}

//static void core0_str_setup(char *outstring) {
//    void core0_sio_irq() {
//        int cnt = 0;
//        while (multicore_fifo_rvalid()){
//            while(1) {
//                outstring[cnt] = (char)multicore_fifo_pop_blocking();
//                if (outstring[cnt] == 0) {cnt = 0; break; }
//                ++cnt;
//            }
//        }
//        multicore_fifo_clear_irq();
//    }
//    multicore_launch_core1(core1_entry); 
//    irq_set_exclusive_handler(SIO_IRQ_PROC0, core0_sio_irq);
//    irq_set_enabled(SIO_IRQ_PROC0, true);
//}

static void core0_array_setup(uint32_t *result_array) {
    static uint32_t rd_ptr;
    static uint32_t tempint;
    void core0_sio_irq() {
        while (multicore_fifo_rvalid()){
            tempint = multicore_fifo_pop_blocking();
            if ((tempint/10000000) == 1) { 
                result_array[ARRAY_LEN+1] = tempint;
            }
            else {
            result_array[rd_ptr%256] = tempint;
            result_array[ARRAY_LEN+2] = rd_ptr;
            //if(result_array[rd_ptr%256] == 0) { break; }
            ++rd_ptr;

            }
        }
        multicore_fifo_clear_irq();
    }
    multicore_launch_core1(core1_entry); 
    irq_set_exclusive_handler(SIO_IRQ_PROC0, core0_sio_irq);
    irq_set_enabled(SIO_IRQ_PROC0, true);
        printf("\n");
}
