#include <math.h>
  //coefficients for chebyshev 4 pole band pass 7 to 0.7 hertx 100 samples/sec
  double A=0.008686, d1=3.642715, d2=-5.067112, d3=3.201052, d4=-0.776986, ep=2.000000;

  double red_w0, red_w1=0, red_w2=0, red_w3=0, red_w4=0;
  float red_bpf(double input) {
      red_w0 = d1*red_w1 + d2*red_w2 + d3*red_w3 + d4*red_w4 + input;
      input = A*(red_w0 - 2.0*red_w2 + red_w4);
      red_w4 = red_w3;
      red_w3 = red_w2;
      red_w2 = red_w1;
      red_w1 = red_w0;
      return((float)(ep*input));
  }

  double ir_w0, ir_w1, ir_w2, ir_w3, ir_w4;
  float ir_bpf(double input) {
      ir_w0 = d1*ir_w1 + d2*ir_w2 + d3*ir_w3 + d4*ir_w4 + input;
      input = A*(ir_w0 - 2.0*ir_w2 + ir_w4);
      ir_w4 = ir_w3;
      ir_w3 = ir_w2;
      ir_w2 = ir_w1;
      ir_w1 = ir_w0;
      return((float)(ep*input));
  }


  //peak extraction
  #define PEAK_NUM   5
  #define PEAK_WIN  64
  #define PEAK_SKIP  1
  float red_peak_adata[PEAK_WIN];
  float red_peak_tdata[PEAK_WIN];
  float red_peak_amp[PEAK_NUM];
  float red_peak_time[PEAK_NUM];
  float red_peak_amp_avg;
  float red_peak_amp_time;
  int red_peak_data_ptr = 0;

void red_peak(float input, float time) {
    int hit, scan, pass;
    float peak_amp, peak_time;
    red_peak_adata[red_peak_data_ptr] = input;
    red_peak_tdata[red_peak_data_ptr] = time;

    //printf("input -- t= %f red =%f\n", time, input);
    pass = 1; //scan through amplitude array to see in middle value id the largest
    for(int i = 0; i < PEAK_WIN; i = i + PEAK_SKIP) {
        if(red_peak_adata[(PEAK_WIN+red_peak_data_ptr - PEAK_WIN/2)%PEAK_WIN] < 
           red_peak_adata[(i+PEAK_WIN+red_peak_data_ptr - PEAK_WIN)%PEAK_WIN]){ pass = 0;break;}
    }  
    peak_amp = red_peak_adata[(PEAK_WIN+red_peak_data_ptr - PEAK_WIN/2)%PEAK_WIN];
    peak_time= red_peak_tdata[(PEAK_WIN+red_peak_data_ptr - PEAK_WIN/2)%PEAK_WIN];
    if (pass == 1) {
        printf("peak found at %.3f of %.6f    %d\n", peak_time, peak_amp,
			(PEAK_WIN+red_peak_data_ptr - PEAK_WIN/2)%PEAK_WIN);
    }
    red_peak_data_ptr = (++red_peak_data_ptr)%PEAK_WIN;
}
