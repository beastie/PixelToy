/* PixelToyFFT.h */

#define FFT_BUFFER_SIZE_LOG 9

#define FFT_BUFFER_SIZE (1 << FFT_BUFFER_SIZE_LOG)

typedef short int sound_sample;

typedef struct _struct_fft_state fft_state;

fft_state *fft_init (void);
void fft_perform (const sound_sample *input, float *output, fft_state *state);
void fft_close (fft_state *state);
static void fft_prepare(const sound_sample *input, float * re, float * im);
static void fft_calculate(float * re, float * im);
static void fft_output(const float *re, const float *im, float *output);
static int reverseBits(unsigned int initial);
