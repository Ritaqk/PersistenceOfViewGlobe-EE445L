// Visual.c
// Runs on TM4C123
// Control the timers to display the currently selected mode
// Rita Kambil
// 10/25/18

// Peripherals
#define SSI3_DR     ((volatile uint32_t *)0x4000B008)
#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "DMASPI.h"
#include "SK9822.h"
#include "Timer1.h"
#include "Visual.h"
#include "Hall.h"
#include "worldmap.h"
#include "ADC.h"
#include "LUT.h"
#include "fxp.h"

// Need to say one more than the actual number
// 97 LEDs needs update time of 0.18ms = 14400 cycles
// 145 LEDs needs update time of 0.26ms = 20800 cycles
#define NUMBER_OF_LEDS 112
#define BYTES_PER_COLUMN ((NUMBER_OF_LEDS + 2)*2) // misnomer, its actually halfwords

#define DMA_PERIOD 70           // Set to 70 for 20MHz, increase for lower freqs
#define TIMER_PERIOD 100000     // Absolute minimum for 20MHz 
#define SAMPLING_RATE 2344      // ADC Sampling rate for FFT - see spreadsheet

#define BRIGHTNESS 0xFFFFFFFF   // AND with this to adjust Brightness
#define BASE_COLOR 0x0000E700   // Default brghtness for generated colors, save battery

void cr4_fft_1024_stm32(Complex_t *pssOUT, Complex_t *pssIN, unsigned short Nbin);

// Important color Globals
uint8_t color_flag = 0;
uint32_t next_color = BASE_COLOR;
uint32_t r = 0x00FE0000;
uint32_t g = 0x00000000;
uint32_t b = 0x00000000;
uint8_t current_output_buffer; // A is 0, B is 1
uint32_t max_columns = 134;
uint32_t column = 0;      // index : p_image[column] -> [GRWB, GRWB, ..., GRWB]

// Ping pong memory buffers
uint32_t buffA[NUMBER_OF_LEDS + 2];
uint32_t buffB[NUMBER_OF_LEDS + 2];

// Ping pong FFT memory buffers
Complex_t Ax[1024], Ay[1024]; // A is 0
Complex_t Bx[1024], By[1024]; // B is 1
uint32_t powA, powB;
Complex_t* sample_buffer;        // Pointer to the current buffer to fill up (swap between Ax and Bx)
uint8_t ready_flag[2] = {0, 0};
uint32_t sample_index;
uint8_t current_sample_buffer;  // A is 0, B is 1
int32_t debug_real[511];
int32_t debug_imag[511];
int32_t debug_mag[511];
uint32_t spectrogram[511];

void ADC0Seq3_Handler(void) {
    ADC0_ISC_R = 0x08;          // acknowledge ADC sequence 3 completion
    sample_buffer[sample_index].real = ADC0_SSFIFO3_R;
    sample_buffer[sample_index].imag = 0;
    sample_index += 1;
    if (sample_index == 1024) {
        sample_index = 0;       // rollover
        if (current_sample_buffer) {
            // Buffer Bx  is full
            ready_flag[1] = 1;
            current_sample_buffer = 0;
            sample_buffer = Ax;         // Next interrupts will fill Ax
        }
        else {
            // Buffer Ax is full
            ready_flag[0] = 1;
            current_sample_buffer = 1;
            sample_buffer = Bx;         // Next interrupts will fill Bx
        }
    }
}

// Subtract mean from each value - in place, do this before FFT
int32_t _get_pow_and_normalize(Complex_t* x) {
    static uint32_t i = 0;
    int32_t ave = 0;
    int32_t pow = 0;
    for (i = 0; i < 1024; i += 1) { // Sum
        ave += x[i].real;
    }
    ave >>= 10;
    pow = 0;
    for (i = 0; i < 1024; i += 1) { // subtract ave
        x[i].real -= ave;
        pow += x[i].real * x[i].real;
    }
    pow >>= 12;
    pow = fx32_sqrt((pow << 3));
    return (pow >> 5); //normalize to under 255
}

// Fill with base color
void _clear_buffer(uint32_t* buff) {
    static uint32_t i = 0;
    buff[0] = 0x00000000;
    for (i = 1; i < NUMBER_OF_LEDS + 2; i += 1) {
        buff[i] = BASE_COLOR;
    }
    buff[NUMBER_OF_LEDS + 1] = 0xFFFFFFFF;
}

// Make the music viz look cool
void _process_FFT(Complex_t* y, uint32_t* buff, uint32_t pow) {
    static uint32_t i = 0;
    static uint32_t j = 0;
    static uint32_t ind1 = 0;
    static uint32_t ind2 = 0;
    static uint32_t led_val = 0;

    _clear_buffer(buff);
    for (i = 1; i < 512; i += 1) {
        spectrogram[i] = (y[i].real * y[i].real) + (y[i].imag * y[i].imag);
    }

    buff[0] = 0x00000000;
    for (i = 0; i < NUMBER_NOTES - 1; i += 1) {
        ind1 = NOTEBINS[i];
        ind2 = NOTEBINS[i + 1];
        led_val = 0;
        // Average up all of the values in this range
        for (j = ind1; j < ind2; j += 1) { led_val += spectrogram[j]; }
        led_val /= (ind2 - ind1);
        led_val = fx32_log2((led_val << 3));
        //led_val += pow;
        if (led_val > 50) {
            buff[NUMBER_OF_LEDS / 2 + i] = BASE_COLOR + (led_val << 24) + pow;
            buff[NUMBER_OF_LEDS / 2 - i] = BASE_COLOR + (led_val << 24) + pow;
        }
        else {
            buff[NUMBER_OF_LEDS / 2 + i] = BASE_COLOR + (led_val << 16) + pow;
            buff[NUMBER_OF_LEDS / 2 - i] = BASE_COLOR + (led_val << 16) + pow;
        }
    }
    buff[NUMBER_OF_LEDS + 1] = 0xFFFFFFFF;
}

//for debug only
void _separate(Complex_t* x, uint8_t safemag) {
    uint32_t i = 0;
    uint32_t mag = 0;
    for (i = 0; i < 511; i += 1) {
        debug_real[i] = x[i].real;
        debug_imag[i] = x[i].imag;
        if (safemag) {
            mag = (x[i].real * x[i].real) + (x[i].imag * x[i].imag);
            debug_mag[i] = fx32_log2((mag << 1));
        }
    }
}

// Private helper function to modify the color globals for crossfade
void _calulate_new_color(void) {
    if (r > 0 && b == 0) {
        r -= 0x00010000;
        g += 0x01000000;
    }
    if (g > 0 && r == 0) {
        g -= 0x01000000;
        b += 0x00000001;
    }
    if (b > 0 && g == 0) {
        r += 0x00010000;
        b -= 0x00000001;
    }
    next_color = BASE_COLOR + r + g + b;
}

// Private helper function to fill buffer with next_color. Generates first and last.
void _fill_buffer_color(uint32_t* buff) {
    static uint32_t ic = 0;                         // no need to reallocate index every time
    buff[0] = 0x00000000;
    for (ic = 1; ic <= NUMBER_OF_LEDS; ic += 1) {
        buff[ic] = next_color;
    }
    buff[NUMBER_OF_LEDS + 1] = 0xFFFFFFFF;
}
// Private helper function to fill buffer with data pulled from ROM
// Assumes image data already has first and last codes built in
// Assumes the data is 8 bit binary data
void _fill_buffer_binary_column(uint32_t* buff) {
    static uint32_t i = 0;                          // no need to reallocate index every time
    static uint32_t val;
    buff[0] = 0x00000000;
    if (color_flag) {
        for (i = 0; i < (NUMBER_OF_LEDS); i += 1) {
            val = Map[column][i];
            if (val == 0xFF) {
                val = next_color;
            }
            else {
                val = BASE_COLOR;
            }
            buff[i + 1] = val;
        }
    }
    else {
        for (i = 0; i < (NUMBER_OF_LEDS); i += 1) {
            val = Map[column][i];
            if (val == 0xFF) {
                val = BRIGHTNESS;
            }
            else {
                val = BASE_COLOR;
            }
            buff[i + 1] = val;
        }
    }
    buff[NUMBER_OF_LEDS + 1] = 0xFFFFFFFF;
}

// Pass this to timer ISR for music viz
void Music(void) {
    // If finished sampling a buffer, process and do out DMA on it
    if (ready_flag[0]) {    // Process Ax
        ready_flag[0] = 0;
        powA = _get_pow_and_normalize(Ax);
        cr4_fft_1024_stm32(Ay, Ax, 1024);   // complex FFT of 1024 values
        _process_FFT(Ay, buffA, powA);
        DMA_Start((uint16_t*) buffA, SSI3_DR, BYTES_PER_COLUMN);
    }
    if (ready_flag[1]) {                 // Process Bx
        ready_flag[1] = 0;
        powB = _get_pow_and_normalize(Bx);
        cr4_fft_1024_stm32(By, Bx, 1024);   // complex FFT of 1024 values
        _process_FFT(By, buffB, powB);
        DMA_Start((uint16_t*) buffB, SSI3_DR, BYTES_PER_COLUMN);
    }
}

// Private function to give to Timer/Hall effect ISR
// But for stationary strip debug, give it to Timer
void Crossfade(void) {
    _calulate_new_color();
    if (current_output_buffer) {
        DMA_Start((uint16_t*) buffB, SSI3_DR, BYTES_PER_COLUMN);
        _fill_buffer_color(buffA);
        current_output_buffer = 0;
    }
    else {
        DMA_Start((uint16_t*) buffA, SSI3_DR, BYTES_PER_COLUMN);
        _fill_buffer_color(buffB);
        current_output_buffer = 1;
    }
}

void Globe(void) {
    _calulate_new_color();
    if (current_output_buffer) {
        DMA_Start((uint16_t*) buffB, SSI3_DR, BYTES_PER_COLUMN);
        _fill_buffer_binary_column(buffA);
        current_output_buffer = 0;
    }
    else {
        DMA_Start((uint16_t*) buffA, SSI3_DR, BYTES_PER_COLUMN);
        _fill_buffer_binary_column(buffB);
        current_output_buffer = 1;
    }
    column += 1;
    if (column == max_columns) { column = 0;}   // loop back
}

void Empty(void) {
    if (current_output_buffer) {
        DMA_Start((uint16_t*) buffB, SSI3_DR, BYTES_PER_COLUMN);
        _clear_buffer(buffA);
        current_output_buffer = 0;
    }
    else {
        DMA_Start((uint16_t*) buffA, SSI3_DR, BYTES_PER_COLUMN);
        _clear_buffer(buffB);
        current_output_buffer = 1;
    }
}

// Valid for both images and crossfading
void HallEffectISR() {
    column = 0;
    _calulate_new_color();
}

//********Visual_Init*****************
// Public function to initialize the LED strip
// Initialize the ping pong buffers, color values, SPI, DMA, ADC
// Don't enable Timer1, ADC, or anything
void Visual_Init(void) {
    Timer1_Init(&Crossfade, TIMER_PERIOD);        // Default State
    LED_Init();
    //Hall_Init(&HallEffectISR);
    ADC_Init(1, 2344);           // Sampling Rate subject to change with RPS
    DMA_Init(DMA_PERIOD);        // How fast to send bytes to SSI buffer?
    _fill_buffer_color(buffA);   // Initialize the buffers
    _fill_buffer_color(buffB);
    current_output_buffer = 1;
}

// TODO: ADC Stop, ADC Start
void Visual_Music(void) {
    ADC_Start();
    Timer1_Change(&Music, 100000); // every 1.25 ms
}

void Visual_Crossfade(uint32_t period) {
    // Sets Timer function pointer to CrossFade
    // TODO: When we have hall effect sensor,
    // Attach crossfade function to HE sensor interrupt
    ADC_Stop();
    Timer1_Change(&Crossfade, period);
    //
}

void Visual_Globe(uint8_t RGB) {
    column = 0;
    ADC_Stop();
    max_columns = 134;
    _clear_buffer(buffA);
    current_output_buffer = 0;
    if (RGB) {
        color_flag = 1;
    }
    else {
        color_flag = 0;
    }
    Timer1_Change(&Globe, TIMER_PERIOD); // For test purposes, later do 14400
}

// Leave the buffers empty to keep the LEDs off
void Visual_Off(void) {
    ADC_Stop();
    Timer1_Change(&Empty, 8000000); // For test purposes, later do 14400
}
