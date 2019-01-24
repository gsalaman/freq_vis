// First iteration.  FFT sampling is dependent on loop frequency, so it's not calibrated, 
//  and theres a bunch of aliasing (I worked out ~10 KHz sample rate)

#include <Adafruit_GFX.h>   // Core graphics library
#include <RGBmatrixPanel.h> // Hardware-specific library
#include <arduinoFFT.h>

// Similar to F(), but for PROGMEM string pointers rather than literals
#define F2(progmem_ptr) (const __FlashStringHelper *)progmem_ptr

#define CLK 11  // MUST be on PORTB! (Use pin 11 on Mega)
#define LAT 10
#define OE  9
#define A   A0
#define B   A1
#define C   A2
#define D   A3

/* Pin Mapping:
 *  Sig   Uno  Mega
 *  R0    2    24
 *  G0    3    25
 *  B0    4    26
 *  R1    5    27
 *  G1    6    28
 *  B1    7    29
 */

// Last parameter = 'true' enables double-buffering, for flicker-free,
// buttery smooth animation.  Note that NOTHING WILL SHOW ON THE DISPLAY
// until the first call to swapBuffers().  This is normal.
RGBmatrixPanel matrix(A, B, C,  D,  CLK, LAT, OE, true);
// Double-buffered mode consumes nearly all the RAM available on the
// Arduino Uno -- only a handful of free bytes remain.  Even the
// following string needs to go in PROGMEM:


// button unused.
#define BUTTON_PIN 12
int button_state;

#define ENVELOPE_PIN A5
#define AUDIO_PIN A4
int max_sound_level=64;
int min_sound_level=0;

#define GAIN_PIN A15
int gain=1;

// These are the raw samples from the audio input.
#define SAMPLE_SIZE 32
int sample[SAMPLE_SIZE] = {0};

// These are used to do the FFT.
double vReal[SAMPLE_SIZE];
double vImag[SAMPLE_SIZE];
arduinoFFT FFT = arduinoFFT();

// This is our FFT output
char data_avgs[SAMPLE_SIZE];
void setup() 
{
  Serial.begin(9600);
  
  matrix.begin();
  pinMode(BUTTON_PIN, INPUT_PULLUP);

}

void collect_samples( void )
{
  int i;
  for (i = 0; i < SAMPLE_SIZE; i++)
  {
    sample[i] = analogRead(AUDIO_PIN);
  }
}

#define SAMPLE_BIAS 512
#define GAIN        8

// Mapped sample should give a number between 0 and 31
int map_sample( int input )
{
  int mapped_sample;
  
  // Looks like our samples are quiet, so I'm gonna start with a very quiet mapping.

  // start by taking out DC bias.  This will make negative #s...
  mapped_sample = input - SAMPLE_BIAS;

  // Now make this a 0-31 number.  

  // add in gain.
  mapped_sample = mapped_sample / gain;
  
  // center on 16.
  mapped_sample = mapped_sample + 16;

  // and clip.
  if (mapped_sample > 31) mapped_sample = 31;
  if (mapped_sample < 0) mapped_sample = 0;

  return mapped_sample;
}

void read_gain( void )
{
   int raw_gain;

   raw_gain = analogRead(GAIN_PIN);
   gain = map(raw_gain, 0, 1023, 1, 32);
}

void show_samples( void )
{
  int x;
  int y;

  matrix.fillScreen(0);
  
  for (x=0; x < SAMPLE_SIZE; x++)
  {
    y=map_sample(sample[x]);
    matrix.drawPixel(x,y,matrix.Color333(0,0,1));
  }
}


void show_samples_lines( void )
{
  int x;
  int y;
  int last_x=0;
  int last_y=16;

  matrix.fillScreen(0);
  
  for (x=0; x < SAMPLE_SIZE; x++)
  {
    y=map_sample(sample[x]);
    matrix.drawLine(last_x,last_y,x,y,matrix.Color333(0,0,1));
    last_x = x;
    last_y = y;
  }
}

void doFFT( void )
{
  int i;
  int temp_sample;

  for (i=0; i < SAMPLE_SIZE; i++)
  {
    // Remove 512 DC bias
    temp_sample = sample[i] - SAMPLE_BIAS;

    // Load the sample into the complex number...some compression here.
    vReal[i] = temp_sample/8;
    vImag[i] = 0;
  }
  
  FFT.Windowing(vReal, SAMPLE_SIZE, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, SAMPLE_SIZE, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLE_SIZE);
}

void display_freq_raw( void )
{
  int i;
  int mag;

  matrix.fillScreen(0);
  
  for (i = 0; i < SAMPLE_SIZE; i++)
  {
    mag = constrain(vReal[i], 0, 40);
    mag = map(mag, 0, 40, 31, 0);

    matrix.drawLine(i,31,i,mag, matrix.Color333(1,0,0));
  }
  
}


void loop() 
{
  //read_gain();
  
  collect_samples();
  doFFT();
  display_freq_raw();
  
  //show_samples_lines();

  //Serial.println(gain);
  
  // Update display
  matrix.swapBuffers(true);

}
