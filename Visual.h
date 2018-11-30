// Visual.h
// Runs on TM4C123
// Control the timers to display the currently selected mode
// Rita Kambil
// 10/25/18

#ifndef __VISUAL_H__
#define __VISUAL_H__
// Color Definitions - GRWB because of endian dumbness
#define VISUAL_BLACK 0x0000FF00
#define VISUAL_WHITE 0xFFFFFFFF
#define VISUAL_RED   0x00FFFF00
#define VISUAL_BLUE  0x0000FFFF
#define VISUAL_GREEN 0xFF00FF00


typedef struct {
    int16_t real, imag;
} Complex_t;


//********Visual_Init*****************
// Initialize the LED strip
// inputs:  None
// outputs: None
void Visual_Init(void);

//********Visual_Crossfade**************
// Set to Crossfade Mode
// inputs:  Period for timer between color changes (Min 14400 @ 80MHz)
// outputs: None
void Visual_Crossfade(uint32_t period);

//********Visual_Globe**************
// Set to Globe mode
// inputs:  Make it Binary or Crossfade
// outputs: None
void Visual_Globe(uint8_t RGB);

//********Visual_Music**************
// Set to Music Visualizer mode, start ADC and do FFT
// inputs:  None
// outputs: None
void Visual_Music(void);


//********Visual_Stop************
// Stop whatever the current mode is outputting
// Use this function when not spinning to save battery
// inputs:  None
// outputs: None
void Visual_Off(void);

#endif //__VISUAL_H__
