void fft_calc(float *input, float *output, int num)
{
    // Create fft plan and let it allocate arrays
    fft_config_t *fft_analysis = fft_init(num, FFT_COMPLEX, FFT_FORWARD, NULL, NULL);
    for (int k = 0 ; k < fft_analysis->size ; k++) {
      fft_analysis->input[2*k] = input[k];
      fft_analysis->input[2*k+1] = 0;
    }

    fft_execute(fft_analysis);
    for (int k = 0 ; k < fft_analysis->size/2 ; k++){
       output[k] = sqrt(fft_analysis->output[2*k]*fft_analysis->output[2*k]+
                        fft_analysis->output[2*k+1]*fft_analysis->output[2*k+1]);
    }
    //printf("\nraw fft output\n");
    //for (int k = 0 ; k < fft_analysis->size/2 ; k++) 
    //   printf("%4d %10.2f %10.2fj\n", k, fft_analysis->output[2*k], fft_analysis->output[2*k+1]); 
    fft_destroy(fft_analysis);
}
