//#include <complex.h>
#include <fftw3.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "otfs.h"
#include "common/config/config_userapi.h"

void PHY_otfs_mod(short *input, int N, int M, short *output){
    fftw_complex *in_Time, *out_Time;
    fftw_complex (*temp)[M];
    temp = (fftw_complex(*)[M])fftw_malloc(M *sizeof(fftw_complex) * N);
    in_Time = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * N);
    out_Time = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * N);
    for(int j = 0; j<M ; j++){  //对每列做IFFT到时域
        fftw_plan p;
        p = fftw_plan_dft_1d(N, in_Time, out_Time, FFTW_BACKWARD, FFTW_ESTIMATE);
        for(int i = 0; i<N ; i++){
            in_Time[i][0] = input[(i * M + j)<<1];
            in_Time[i][1] = input[((i * M + j)<<1)+1];
        }
        fftw_execute(p);
        for(int i = 0; i<N; i++){
		memcpy((void*)temp[i][j],(void*)out_Time[i], sizeof(fftw_complex));
        }
        fftw_destroy_plan(p);
    }
    fftw_free(in_Time); fftw_free(out_Time);
    fftw_complex *in_Freq, *out_Freq;
    in_Freq = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * M);
    out_Freq = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * M);
    for(int i = 0; i<N ; i++){   //对每行做FFT到频域
        fftw_plan p;
        p = fftw_plan_dft_1d(M, in_Freq, out_Freq, FFTW_FORWARD, FFTW_ESTIMATE);
        for(int j = 0; j<M; j++){
        	memcpy((void*)in_Freq[j],(void*)temp[i][j], sizeof(fftw_complex));
	}
        fftw_execute(p);
        for(int j = 0; j<M; j++){
            output[(i * M + j)<<1] =(short)(out_Freq[j][0]/M*N);
            output[((i * M + j)<<1)+1] =(short)(out_Freq[j][1]/M*N);
        }
        fftw_destroy_plan(p);
    }
    fftw_free(in_Freq); fftw_free(out_Freq); //时延域到频域的转换完成
    fftw_free(temp);
}

void PHY_otfs_demod(short *input, int N, int M, short *output){
    //先对N部分的时域做FFT到多普勒域
    fftw_complex *in_Time, *out_Time;
    fftw_complex (*temp)[M];
    temp = (fftw_complex(*)[M])fftw_malloc(M *sizeof(fftw_complex) * N);
    in_Time = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * N);
    out_Time = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * N);
    for(int j = 0; j<M; j++){
        fftw_plan p;
        p = fftw_plan_dft_1d(N, in_Time, out_Time, FFTW_FORWARD, FFTW_ESTIMATE);
        for(int i = 0; i<N; i++){
            in_Time[i][0] = input [(i * M +j)<<1];
            in_Time[i][1] = input [((i * M +j)<<1) + 1];
        }
        fftw_execute(p);
        for(int i = 0; i<N; i++){
		memcpy((void*)temp[i][j],(void*)out_Time[i], sizeof(fftw_complex));
        }
        fftw_destroy_plan(p);
    }
    fftw_free(in_Time); fftw_free(out_Time);
//对M部分的频域做FFT到时延域
    fftw_complex *in_Freq, *out_Freq;
    in_Freq = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * M);
    out_Freq = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * M);
    for(int i = 0; i<N ; i++){ 
        fftw_plan p;
        p = fftw_plan_dft_1d(M, in_Freq, out_Freq, FFTW_BACKWARD, FFTW_ESTIMATE);
        for(int j = 0; j<M; j++){
		memcpy((void*)in_Freq[j],(void*)temp[i][j], sizeof(fftw_complex));
        }
        fftw_execute(p);
        for(int j = 0; j<M; j++){
            output[(i * M + j)<<1] = out_Freq[j][0]*M/N;
            output[((i * M + j)<<1)+1] = out_Freq[j][1]*M/N;
        }
        fftw_destroy_plan(p);
    }
    fftw_free(in_Freq); fftw_free(out_Freq); //SFFT完成
    fftw_free(temp);
}
