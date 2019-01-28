// Take 3.  Move back to UNO.  Figured out ADC register mappings.  Just gonna do serial print for this take.
#include <Adafruit_GFX.h>   // Core graphics library
#include <RGBmatrixPanel.h> // Hardware-specific library
#include <arduinoFFT.h>


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
//RGBmatrixPanel matrix(A, B, C,  D,  CLK, LAT, OE, false);
// Double-buffered mode consumes nearly all the RAM available on the
// Arduino Uno -- only a handful of free bytes remain.  Even the
// following string needs to go in PROGMEM:

#define DISPLAY_COLUMNS 32

// Since I'm bit-banging the ADC register, I'm not using a define for Audio pin.  It's on 5.

// These are the raw samples from the audio input.
#define SAMPLE_SIZE 128
int sample[SAMPLE_SIZE] = {0};

// These are used to do the FFT.
double vReal[SAMPLE_SIZE];
double vImag[SAMPLE_SIZE];
arduinoFFT FFT = arduinoFFT();

// This is our FFT output
char data_avgs[DISPLAY_COLUMNS];

int gain=8;

void setup() 
{
  Serial.begin(9600);
  
  //matrix.begin();

  setupADC();

}

void setupADC( void )
{

    ADCSRA = 0b11100101;      // set ADC to free running mode and set pre-scalar to 32 (0xe5)
                              // pre-scalar 32 should give sample frequency of 40 KHz on the UNO
                              // ...which will reproduce samples up to 20 KHz

    //ADMUX = 0b01000101;       // use pin A5 and the internal 5v voltage reference
    ADMUX = 0b00000101;         // Ext 5v rev.

    delay(50);  //wait for voltages to stabalize. 
}

void collect_accurate_samples( void )
{
  //use ADC internals.

  int i;

  for (i=0; i<SAMPLE_SIZE; i++)
  {
    while(!(ADCSRA & 0x10));        // wait for ADC to complete current conversion ie ADIF bit set
    ADCSRA = 0b11110101 ;               // clear ADIF bit so that ADC can do next operation (0xf5)

    sample[i] = ADC;
  }
}

#define SAMPLE_BIAS 512

void doFFT( void )
{
  int i;
  int temp_sample;
  long int total=0;
  long int avg;

#if 0
  for (i=0; i < SAMPLE_SIZE; i++)
  {
    total = total + sample[i];
  }
  avg = total / SAMPLE_SIZE;
  Serial.print("Avg: ");
  Serial.println(avg);
#endif

  Serial.println("Mapped Samples:");
  for (i=0; i < SAMPLE_SIZE; i++)
  {
    // Remove DC bias
    //temp_sample = sample[i] - avg;
    temp_sample = sample[i] - SAMPLE_BIAS;

    Serial.println(temp_sample);

    // try and map this between 50 and -50
    // Load the sample into the complex number...some compression here.
    //vReal[i] = temp_sample/8;
    vReal[i] = temp_sample;
    vImag[i] = 0;
    
  }

  Serial.println("=====");
  
  FFT.Windowing(vReal, SAMPLE_SIZE, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, SAMPLE_SIZE, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLE_SIZE);
}


#if 0
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
#endif


void print_freq_mag( void )
{
  int i;
  int freq;

  // 20 KHz over 64 bins means about 320 Hz per bin.

  Serial.println("FREQ Results");
  for (i=0; i < SAMPLE_SIZE/2; i++)
  {
    freq = i*312;
    Serial.print(freq);
    Serial.print(" Hz = ");
    Serial.println(vReal[i]);
  }

  Serial.println("========");
}

void print_samples( void )
{
  int i;

  Serial.println("Sample Buffer: ");
  for (i=0; i<SAMPLE_SIZE; i++)
  {
    Serial.println(sample[i]);
  }
  Serial.println("===================");
}


void loop() 
{
  unsigned long start_time;
  unsigned long stop_time;
  unsigned long delta_t;
  unsigned long per_sample;

  start_time = micros();
  collect_accurate_samples();
  //collect_samples();
  stop_time = micros();
  
  print_samples();

  doFFT();
  
  delta_t = stop_time - start_time;
  per_sample = delta_t / SAMPLE_SIZE;
  
  Serial.print("Time to collect samples (us): ");
  Serial.println( (stop_time - start_time ) );
  Serial.print(per_sample);
  Serial.print(" us per sample (");
  Serial.print( 1000000 / per_sample);
  Serial.println(" Hz)");

  print_freq_mag();


  Serial.println("hit enter for next sampling");
  while (!Serial.available());
  while (Serial.available()) Serial.read();
  
}
