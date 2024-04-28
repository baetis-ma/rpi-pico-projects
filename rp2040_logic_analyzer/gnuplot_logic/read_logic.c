#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

void main()  {
   char labels[128][128];
   float sample_rate, trigoff;
   int numcol;
   int col_hex[128], numtrace;
   char header[128];
   char *tline;
   int vallast[128];
   int nlabels, lcptr;
   int vecnum = 0;
   int col_data[1000][128];
   int slope = 3;
   int sample_num, trigpin;

   size_t len;
   while((getline(&tline, &len, stdin)) != EOF ) {
      // extract paramters from packet head
      if(strncmp("highspeed", tline, 9) == 0) {
         sscanf(tline, "highspeed samp=%d rate=%fus trigpin=%d trigoff=%f header %128[^\n]", 
                 &sample_num, &sample_rate, &trigpin, &trigoff, header);
//printf("%d %.3f %d %.3f %s\n", sample_num, sample_rate, trigpin, trigoff, header);
         //extract labels from header
         nlabels = 0;
         lcptr = 0;
         while (sscanf(header+lcptr, "%s", labels[nlabels]) > 0) {
            lcptr = lcptr + strlen(labels[nlabels]) + 1;
//printf("labels[%d]=%s", nlabels, labels[nlabels]); printf("\n");
            ++nlabels;
         } 
      } else
      //extract data vectors from payload  
      if (tline[0] == '#') { printf("%s", tline); } else
      if (tline[0] != 'e') {
         int nn = 0, mul = 1, hcol = 0, first = 0;
	 numcol = 0;
         sscanf(tline, "%d", &col_data[vecnum][numcol++]);
         int l = strlen(tline);
         tline[l-1] = ' '; tline[l-2] = ' ';
         for(int n = 8; n < strlen(tline); n++) {
            if(tline[n] == '1') { col_data[vecnum][numcol] = col_data[vecnum][numcol] + mul; }
            if(tline[n] == ' ' && tline[n + 1] != ' ') {  first = 1; }
            if(first != 0) {
               if(tline[n] == ' ' && tline[n + 1] != ' ') { hcol = 1; mul = 1; } //space then dig 
               if(tline[n] != ' ' && tline[n + 1] != ' ') { hcol++; mul = 2 * mul;} //dig then dig 
               if(tline[n] != ' ' && tline[n + 1] == ' ') { col_hex[numcol] = hcol; ++numcol; } //dig then space
            }
            //printf(" %2d %2d %c %d %d    %d\n", l, n, tline[n], mul, hcol, numcol);
         }
//       //calculate number of traces
         numtrace = numcol - 1;
         for(int n = 1; n <= numcol; n++) if(col_hex[n] > 1) ++numtrace;
         //printf("numcol = %d  numtrace = %d\n", numcol, numtrace);
	 //printf("col_hex    ");for(int n = 0; n <= numcol; n++)printf("%d ", col_hex[n]);printf("\n");
	 //printf("col_data ");for(int n = 0; n < numcol; n++)printf("%d ", col_data[vecnum][n]);printf("\n");
         vecnum++;
      } 
      else if(tline[0] == 'e') {
         //gnuplot header
         printf("#!/usr/bin/gnuplot -p\n");
         printf("set terminal x11 noraise background rgb \'dark-olivegreen\'\n");
         //printf("set terminal wxt noraise size 1200, 400 background rgb \'dark-olivegreen\'\n");
         printf("set yrange [-0.5:%d]\n", 5*(numcol-1));
         printf("set xrange [0:%f]\n", 0.001 * sample_rate * sample_num);
         printf("set title \'Logic Analyzer  - samples=%d rate=%.3fus trigpin=%d\' tc rgb \'white\'\n", 
                         sample_num, sample_rate, trigpin);
         printf("unset label\n");
         printf("unset ytics\n");
         printf("unset key\n");
         printf("set style line 1 linecolor rgb 'orange' lw 2\n");
         printf("set label 1 \"*\" font \",20\" at %f,-.10 center tc rgb \'red\'\n", 
            0.001*trigoff*sample_rate*sample_num);
         printf("array xa[2000]\n");
         for(int i=0; i < numtrace; i++) printf("array y%da[2000]\n", i);

         //yaxis labels
         for(int i=0; i < numcol; i++){
            printf("set label \"%s\" at %f,%d tc \"white\" front rotate by 60 font \",30\"\n",
                labels[i], -0.0001, 5*(i+1) -4 );
         }

         //make labels for buses
         for(int i=1; i < vecnum - 1; i++){
            for(int ii=0; ii < numcol; ii++){
            //for(int ii=0; ii < nlabels; ii++){
               if(col_hex[ii+1] > 1 && col_data[i][ii+1] != col_data[i+1][ii+1])
                   printf("set label \"x%x\" at %f,%0.1f rotate by 60 tc \"orange\" font \",13\"\n",
                      col_data[i][ii+1], 0.001*(float)(sample_rate*col_data[i][0]+5), 5.0*ii+1);
            }
         }
   
         //gnuplot data
         for(int vec = 0; vec < vecnum; vec++) {
            int ptr = 0;
            printf("xa[%d]=%.3f; ", 2 * vec + 1, 0.001 * (sample_rate * col_data[vec][0] - slope));
            int trace = 0;
            for(int n = 1; n < numcol; n++) {
               if(col_hex[n] == 1) printf("y%da[%d]=%d; ", trace++, 2*vec+1, 5*(n-1)+4*col_data[vec-1][n]);
               else {
                  printf("y%da[%d]=%d; ", trace++, 2*vec + 1, 5*(n-1)+4*vallast[n]);
                  printf("y%da[%d]=%d; ", trace++, 2*vec + 1, 5*(n-1)+4*!(vallast[n]));
              }
            }
            printf("\n");
            printf("xa[%d]=%.3f; ", 2 * vec + 2, 0.001 * sample_rate * col_data[vec][0]);
            trace = 0;
            for(int n = 1; n < numcol; n++) {
               if(col_hex[n] == 1) printf("y%da[%d]=%d; ", trace++, 2*vec+2, 5*(n-1)+4*col_data[vec][n]);
                  else {
                  if(col_data[vec-1][n] != col_data[vec][n]) {
                     if(vallast[n] == 1) vallast[n] = 0; else vallast[n] = 1;
                  }
                  printf("y%da[%d]=%d; ", trace++, 2*vec+2, 5*(n-1)+4*vallast[n]);
                  printf("y%da[%d]=%d; ", trace++, 2*vec+2, 5*(n-1)+4*!(vallast[n]));
               }
            }
            printf("\n");
         }
         
         //gnuplot tail
         if(numtrace==1)printf("plot xa u 2:(y0a[$1]) w lines ls 1\n");
             else printf("plot xa u 2:(y0a[$1]) w lines ls 1, \\\n");
         for(int a=1; a < numtrace-1; a++)
            printf("     xa u 2:(y%da[$1]) w lines ls 1, \\\n", a);
         printf("     xa u 2:(y%da[$1]) w lines ls 1\n\n", numtrace-1);

         memset(col_data, 0, 1000*128*4);
         vecnum = 0;
         fflush(stdout);
      }
   }
}

