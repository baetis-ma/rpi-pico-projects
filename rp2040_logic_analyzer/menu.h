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
