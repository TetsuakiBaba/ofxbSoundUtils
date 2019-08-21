/**********************************************************************

  fft.cpp

  
  This class is a C++ wrapper for original code 
  written by Dominic Mazzoni in September 2000

  This file contains a few FFT routines, including a real-FFT
  routine that is almost twice as fast as a normal complex FFT,
  and a power spectrum routine which is more convenient when
  you know you don't care about phase information.  It now also
  contains a few basic windowing functions.

  Some of this code was based on a free implementation of an FFT
  by Don Cross, available on the web at:

    http://www.intersrv.com/~dcross/fft.html

  The basic algorithm for his code was based on Numerical Recipes
  in Fortran.  I optimized his code further by reducing array
  accesses, caching the bit reversal table, and eliminating
  float-to-double conversions, and I added the routines to
  calculate a real FFT and a real power spectrum.

  Note: all of these routines use single-precision floats.
  I have found that in practice, floats work well until you
  get above 8192 samples.  If you need to do a larger FFT,
  you need to use doubles.

**********************************************************************/

#include "bFFT.h"	
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

int **bFFT_gFFTBitTable = NULL;
const int MaxFastBits = 16;

int bFFT_IsPowerOfTwo(int x)
{
   if (x < 2)
      return false;

   if (x & (x - 1))             /* Thanks to 'byang' for this cute trick! */
      return false;

   return true;
}

int bFFT_NumberOfBitsNeeded(int PowerOfTwo)
{
   int i;

   if (PowerOfTwo < 2) {
      fprintf(stderr, "Error: FFT called with size %d\n", PowerOfTwo);
      exit(1);
   }

   for (i = 0;; i++)
      if (PowerOfTwo & (1 << i))
         return i;
}

int bFFT_ReverseBits(int index, int NumBits)
{
   int i, rev;

   for (i = rev = 0; i < NumBits; i++) {
      rev = (rev << 1) | (index & 1);
      index >>= 1;
   }

   return rev;
}

void bFFT_InitFFT()
{
   bFFT_gFFTBitTable = new int *[MaxFastBits];

   int len = 2;
   for (int b = 1; b <= MaxFastBits; b++) {

      bFFT_gFFTBitTable[b - 1] = new int[len];

      for (int i = 0; i < len; i++)
         bFFT_gFFTBitTable[b - 1][i] = bFFT_ReverseBits(i, b);

      len <<= 1;
   }
}

inline int bFFT_FastReverseBits(int i, int NumBits)
{
   if (NumBits <= MaxFastBits)
      return bFFT_gFFTBitTable[NumBits - 1][i];
   else
      return bFFT_ReverseBits(i, NumBits);
}

/*
 * Complex Fast Fourier Transform
 */

void bFFT_FFT(int NumSamples,
         bool InverseTransform,
         float *RealIn, float *ImagIn, float *RealOut, float *ImagOut)
{
   int NumBits;                 /* Number of bits needed to store indices */
   int i, j, k, n;
   int BlockSize, BlockEnd;

   double angle_numerator = 2.0 * M_PI;
   float tr, ti;                /* temp real, temp imaginary */

   if (!bFFT_IsPowerOfTwo(NumSamples)) {
      fprintf(stderr, "%d is not a power of two\n", NumSamples);
      exit(1);
   }

   if (!bFFT_gFFTBitTable)
      bFFT_InitFFT();

   if (InverseTransform)
      angle_numerator = -angle_numerator;

   NumBits = bFFT_NumberOfBitsNeeded(NumSamples);

   /*
    **   Do simultaneous data copy and bit-reversal ordering into outputs...
    */

   for (i = 0; i < NumSamples; i++) {
      j = bFFT_FastReverseBits(i, NumBits);
      RealOut[j] = RealIn[i];
      ImagOut[j] = (ImagIn == NULL) ? 0.0 : ImagIn[i];
   }

   /*
    **   Do the FFT itself...
    */

   BlockEnd = 1;
   for (BlockSize = 2; BlockSize <= NumSamples; BlockSize <<= 1) {

      double delta_angle = angle_numerator / (double) BlockSize;

      float sm2 = sin(-2 * delta_angle);
      float sm1 = sin(-delta_angle);
      float cm2 = cos(-2 * delta_angle);
      float cm1 = cos(-delta_angle);
      float w = 2 * cm1;
      float ar0, ar1, ar2, ai0, ai1, ai2;

      for (i = 0; i < NumSamples; i += BlockSize) {
         ar2 = cm2;
         ar1 = cm1;

         ai2 = sm2;
         ai1 = sm1;

         for (j = i, n = 0; n < BlockEnd; j++, n++) {
            ar0 = w * ar1 - ar2;
            ar2 = ar1;
            ar1 = ar0;

            ai0 = w * ai1 - ai2;
            ai2 = ai1;
            ai1 = ai0;

            k = j + BlockEnd;
            tr = ar0 * RealOut[k] - ai0 * ImagOut[k];
            ti = ar0 * ImagOut[k] + ai0 * RealOut[k];

            RealOut[k] = RealOut[j] - tr;
            ImagOut[k] = ImagOut[j] - ti;

            RealOut[j] += tr;
            ImagOut[j] += ti;
         }
      }

      BlockEnd = BlockSize;
   }

   /*
      **   Need to normalize if inverse transform...
    */

   if (InverseTransform) {
      float denom = (float) NumSamples;

      for (i = 0; i < NumSamples; i++) {
         RealOut[i] /= denom;
         ImagOut[i] /= denom;
      }
   }
}

/*
 * Real Fast Fourier Transform
 *
 * This function was based on the code in Numerical Recipes in C.
 * In Num. Rec., the inner loop is based on a single 1-based array
 * of interleaved real and imaginary numbers.  Because we have two
 * separate zero-based arrays, our indices are quite different.
 * Here is the correspondence between Num. Rec. indices and our indices:
 *
 * i1  <->  real[i]
 * i2  <->  imag[i]
 * i3  <->  real[n/2-i]
 * i4  <->  imag[n/2-i]
 */

void bFFT_RealFFT(int NumSamples, float *RealIn, float *RealOut, float *ImagOut)
{
   int Half = NumSamples / 2;
   int i;

   float theta = M_PI / Half;

   float *tmpReal = new float[Half];
   float *tmpImag = new float[Half];

   for (i = 0; i < Half; i++) {
      tmpReal[i] = RealIn[2 * i];
      tmpImag[i] = RealIn[2 * i + 1];
   }

   bFFT_FFT(Half, 0, tmpReal, tmpImag, RealOut, ImagOut);

   float wtemp = float (sin(0.5 * theta));

   float wpr = -2.0 * wtemp * wtemp;
   float wpi = float (sin(theta));
   float wr = 1.0 + wpr;
   float wi = wpi;

   int i3;

   float h1r, h1i, h2r, h2i;

   for (i = 1; i < Half / 2; i++) {

      i3 = Half - i;

      h1r = 0.5 * (RealOut[i] + RealOut[i3]);
      h1i = 0.5 * (ImagOut[i] - ImagOut[i3]);
      h2r = 0.5 * (ImagOut[i] + ImagOut[i3]);
      h2i = -0.5 * (RealOut[i] - RealOut[i3]);

      RealOut[i] = h1r + wr * h2r - wi * h2i;
      ImagOut[i] = h1i + wr * h2i + wi * h2r;
      RealOut[i3] = h1r - wr * h2r + wi * h2i;
      ImagOut[i3] = -h1i + wr * h2i + wi * h2r;

      wr = (wtemp = wr) * wpr - wi * wpi + wr;
      wi = wi * wpr + wtemp * wpi + wi;
   }

   RealOut[0] = (h1r = RealOut[0]) + ImagOut[0];
   ImagOut[0] = h1r - ImagOut[0];

   delete[]tmpReal;
   delete[]tmpImag;
}

/*
 * PowerSpectrum
 *
 * This function computes the same as RealFFT, above, but
 * adds the squares of the real and imaginary part of each
 * coefficient, extracting the power and throwing away the
 * phase.
 *
 * For speed, it does not call RealFFT, but duplicates some
 * of its code.
 */

void bFFT_PowerSpectrum(int NumSamples, float *In, float *Out)
{
   int Half = NumSamples / 2;
   int i;

   float theta = M_PI / Half;

   float *tmpReal = new float[Half];
   float *tmpImag = new float[Half];
   float *RealOut = new float[Half];
   float *ImagOut = new float[Half];

   for (i = 0; i < Half; i++) {
      tmpReal[i] = In[2 * i];
      tmpImag[i] = In[2 * i + 1];
   }

   bFFT_FFT(Half, 0, tmpReal, tmpImag, RealOut, ImagOut);

   float wtemp = float (sin(0.5 * theta));

   float wpr = -2.0 * wtemp * wtemp;
   float wpi = float (sin(theta));
   float wr = 1.0 + wpr;
   float wi = wpi;

   int i3;

   float h1r, h1i, h2r, h2i, rt, it;
   //float total=0;

   for (i = 1; i < Half / 2; i++) {

      i3 = Half - i;

      h1r = 0.5 * (RealOut[i] + RealOut[i3]);
      h1i = 0.5 * (ImagOut[i] - ImagOut[i3]);
      h2r = 0.5 * (ImagOut[i] + ImagOut[i3]);
      h2i = -0.5 * (RealOut[i] - RealOut[i3]);

      rt = h1r + wr * h2r - wi * h2i; //printf("Realout%i = %f",i,rt);total+=fabs(rt);
      it = h1i + wr * h2i + wi * h2r; // printf("  Imageout%i = %f\n",i,it);

      Out[i] = rt * rt + it * it;

      rt = h1r - wr * h2r + wi * h2i;
      it = -h1i + wr * h2i + wi * h2r;

      Out[i3] = rt * rt + it * it;

      wr = (wtemp = wr) * wpr - wi * wpi + wr;
      wi = wi * wpr + wtemp * wpi + wi;
   }
   //printf("total = %f\n",total);
   rt = (h1r = RealOut[0]) + ImagOut[0];
   it = h1r - ImagOut[0];
   Out[0] = rt * rt + it * it;

   rt = RealOut[Half / 2];
   it = ImagOut[Half / 2];
   Out[Half / 2] = rt * rt + it * it;

   delete[]tmpReal;
   delete[]tmpImag;
   delete[]RealOut;
   delete[]ImagOut;
}

/*
 * Windowing Functions
 */

int bFFT_NumWindowFuncs()
{
   return 4;
}

char *bFFT_WindowFuncName(int whichFunction)
{
   switch (whichFunction) {
   default:
   case 0:
      return "Rectangular";
   case 1:
      return "Bartlett";
   case 2:
      return "Hamming";
   case 3:
      return "Hanning";
   }
}

void bFFT_WindowFunc(int whichFunction, int NumSamples, float *in)
{
   int i;

   if (whichFunction == 1) {
      // Bartlett (triangular) window
      for (i = 0; i < NumSamples / 2; i++) {
         in[i] *= (i / (float) (NumSamples / 2));
         in[i + (NumSamples / 2)] *=
             (1.0 - (i / (float) (NumSamples / 2)));
      }
   }

   if (whichFunction == 2) {
      // Hamming
      for (i = 0; i < NumSamples; i++)
         in[i] *= 0.54 - 0.46 * cos(2 * M_PI * i / (NumSamples - 1));
   }

   if (whichFunction == 3) {
      // Hanning
      for (i = 0; i < NumSamples; i++)
         in[i] *= 0.50 - 0.50 * cos(2 * M_PI * i / (NumSamples - 1));
   }
}

/* constructor */
bFFT::bFFT() {
}

/* destructor */
bFFT::~bFFT() {
    delete magnitude;
    delete phase;
    delete power;
}

void bFFT::setup(int _bufsize, int _sampling_rate)
{
    bufsize = _bufsize;
    sampling_rate = _sampling_rate;
    magnitude = new float[bufsize];
    phase = new float[bufsize];
    power = new float[bufsize];
    spectrum.resize(bufsize/2);
    sound = new float[bufsize];

}

void bFFT::update(float *_input_sound)
{
    powerSpectrum(0,
                  (int)bufsize/2,
                   _input_sound,
                   bufsize,
                   &magnitude[0],
                   &phase[0],
                   &power[0],
                   &avg_power);
    memcpy(sound, _input_sound, bufsize);
}
/* Calculate the power spectrum */
void bFFT::powerSpectrum(int start, int half, float *data, int windowSize,float *magnitude,float *phase, float *power, float *avg_power) {
    int i;
    int windowFunc = 3;
    float total_power = 0.0f;
    
    /* processing variables*/
    float *in_real = new float[windowSize];
    float *in_img = new float[windowSize];
    float *out_real = new float[windowSize];
    float *out_img = new float[windowSize];
    
    for (i = 0; i < windowSize; i++) {
        in_real[i] = data[start + i];
    }
    
    bFFT_WindowFunc(windowFunc, windowSize, in_real);
    bFFT_RealFFT(windowSize, in_real, out_real, out_img);
    
    max_power = 0.0;
    for (i = 0; i < half; i++) {
        /* compute power */
        power[i] = out_real[i]*out_real[i] + out_img[i]*out_img[i];
        total_power += power[i];
        /* compute magnitude and phase */
        magnitude[i] = 2.0*sqrt(power[i]);
        phase[i] = atan2(out_img[i],out_real[i]);
        
        if( max_power < power[i] )max_power = power[i];
        spectrum[i].power = power[i];
        spectrum[i].db    = 10*log10(power[i]);
        spectrum[i].Hz = getFreqStep(sampling_rate, bufsize)*i;
    }
    /* calculate average power */
    *(avg_power) = total_power / (float) half;
    
    delete[]in_real;
    delete[]in_img;
    delete[]out_real;
    delete[]out_img;
}

void bFFT::inversePowerSpectrum(int start, int half, int windowSize, float *finalOut,float *magnitude,float *phase) {
	int i;
   int windowFunc = 3;
   
	/* processing variables*/
   float *in_real = new float[windowSize];
   float *in_img = new float[windowSize];
   float *out_real = new float[windowSize];
   float *out_img = new float[windowSize];
	
	/* get real and imag part */
	for (i = 0; i < half; i++) {	
		in_real[i] = magnitude[i]*cos(phase[i]);
		in_img[i]  = magnitude[i]*sin(phase[i]);
	}
	
	/* zero negative frequencies */
	for (i = half; i < windowSize; i++) {	
		in_real[i] = 0.0;
		in_img[i] = 0.0;
	}
	
	//FFT(windowSize, 1, in_real, in_img, out_real, out_img); // second parameter indicates inverse transform
	bFFT_WindowFunc(windowFunc, windowSize, out_real);
				
	for (i = 0; i < windowSize; i++) {
		finalOut[start + i] += out_real[i];
	}
			
   delete[]in_real;
   delete[]in_img;   
   delete[]out_real;
   delete[]out_img;
}

int bFFT::getFreqSpectrum(float freq, int half, float *magnitude)
{
}

float bFFT::getFreqStep(int sampling_rate, int buffer_size)
{
  return sampling_rate/(float)buffer_size;
}

float bFFT::getFreqStep()
{
    return sampling_rate/(float)bufsize;
}

float bFFT::getPower(float _hz)
{
    for( int i = 0 ; i < spectrum.size()-1; i++){
        if( spectrum[i].Hz <= _hz &&
           _hz < spectrum[i+1].Hz){
            return (spectrum[i].power+spectrum[i+1].power)/2.0;
        }
    }
    return -1;
}

double bFFT::getDFTPower(float _hz)
{
    double freq = _hz;
    double dw = freq*2*M_PI*(1.0/(float)sampling_rate);
    double w = 0.0;
    double sum_real = 0.0;
    double sum_imag = 0.0;
    for( int i = 0; i < bufsize; i++ ){
        sum_real = sum_real + cos(w)*sound[i];
        sum_imag = sum_imag + (-1)*sin(w)*sound[i];
        w = w + dw;
    }
    return (pow(sum_real,2)+pow(sum_imag,2));
}
