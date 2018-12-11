// Lab11.c
// Runs on TM4C123
// Initialization and control loop for LED globe
// Rita Kambil
// 10/26/18

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "../inc/tm4c123gh6pm.h"
#include "PLL.h"
#include "DMASPI.h"
#include "SK9822.h"
#include "Timer1.h"
#include "Timer2.h"
#include "Timer3.h"
#include "UART.h"
#include "esp8266.h"
#include "Visual.h"
#include "fxp.h"
#include "SysTick.h"
#include "Hall.h"

#define SSI3_DR ((volatile uint32_t *)0x4000B008)
#define COLOR_MODE 1
#define GLOBE_MODE 2
#define MUSIC_MODE 3
#define SPECTRO_MODE 4
#define OFF_MODE   0

void EnableInterrupts(void);
void PortF_Init(void);
void Crossfade(void); // LED Strip test
void Blink(void);
void DisableInterrupts(void);   // Defined in startup.s
void WaitForInterrupt(void);    // Defined in startup.s

// These 6 variables contain the most recent Blynk to TM4C123 message
// Blynk to TM4C123 uses VP0 to VP15
char serial_buf[64];
char Pin_Number[2]   = "99";       // Initialize to invalid pin number
char Pin_Integer[8]  = "0000";     //
char Pin_Float[8]    = "0.0000";   //
uint32_t pin_num;
uint32_t pin_int;

uint8_t last_mode = 0;
uint8_t current_mode = COLOR_MODE;




// Blink the on-board LED.
#define PF0       (*((volatile uint32_t *)0x40025004))
#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))

// -------------------------   Blynk_to_TM4C  -----------------------------------
// This routine receives the Blynk Virtual Pin data via the ESP8266 and parses the
// data and feeds the commands to the TM4C.
void Blynk_to_TM4C(void) {
    int j; char data;
    // Check to see if a there is data in the RXD buffer
    if (ESP8266_GetMessage(serial_buf)) { // returns false if no message
        // Read the data from the UART5
#ifdef DEBUG1
        j = 0;
        do {
            data = serial_buf[j];
            UART_OutChar(data);        // Debug only
            j++;
        } while (data != '\n');
        UART_OutChar('\r');
#endif

        // Rip the 3 fields out of the CSV data. The sequence of data from the 8266 is:
        // Pin #, Integer Value, Float Value.
        strcpy(Pin_Number, strtok(serial_buf, ","));
        strcpy(Pin_Integer, strtok(NULL, ","));       // Integer value that is determined by the Blynk App
        strcpy(Pin_Float, strtok(NULL, ","));         // Not used
        pin_num = atoi(Pin_Number);     // Need to convert ASCII to integer
        pin_int = atoi(Pin_Integer);

        // ---------------------------- VP #1 ----------------------------------------
        // Mode Selector
        if (pin_num == 1) {
            switch (pin_int) {
            case COLOR_MODE:
                Visual_Crossfade(1000000);
                current_mode = COLOR_MODE;
                break;
            case GLOBE_MODE:
                Visual_Globe();
                current_mode = GLOBE_MODE;
                break;
            case MUSIC_MODE:
                Visual_Music();
                current_mode = MUSIC_MODE;
                break;
            case SPECTRO_MODE:
                Visual_Spectrogram();
                current_mode = SPECTRO_MODE;
                break;
            }
        }
        // Toggle LEDs on and off, return to last mode
        if (pin_num == 0 && pin_int == 1) {
            if (current_mode == OFF_MODE) {
                switch (last_mode) {
                case COLOR_MODE:
                    Visual_Crossfade(1000000);
                    current_mode = COLOR_MODE;
                    break;
                case GLOBE_MODE:
                    Visual_Globe();
                    current_mode = GLOBE_MODE;
                    break;
                case MUSIC_MODE:
                    Visual_Music();
                    current_mode = MUSIC_MODE;
                    break;
                case SPECTRO_MODE:
                    Visual_Spectrogram();
                    current_mode = SPECTRO_MODE;
                    break;
                }
            }
            // Turn off leds
            else {
                last_mode = current_mode; // Save last mode
                Visual_Off();
                current_mode = OFF_MODE;
            }
        }
#ifdef DEBUG1
        UART_OutString(" Pin_Number = ");
        UART_OutString(Pin_Number);
        UART_OutString("   Pin_Integer = ");
        UART_OutString(Pin_Integer);
        UART_OutString("   Pin_Float = ");
        UART_OutString(Pin_Float);
        UART_OutString("\n\r");
#endif
    }
}

// ----------------------------------- TM4C_to_Blynk ------------------------------
// Send data to the Blynk App
// It uses Virtual Pin numbers between 70 and 99
// so that the ESP8266 knows to forward the data to the Blynk App
void TM4C_to_Blynk(uint32_t pin, uint32_t value) {
    if ((pin < 70) || (pin > 99)) {
        return; // ignore illegal requests
    }
    // your account will be temporarily halted if you send too much data
    ESP8266_OutUDec(pin);        // Send the Virtual Pin #
    ESP8266_OutChar(',');
    ESP8266_OutUDec(value);      // Send the current value
    ESP8266_OutChar(',');
    ESP8266_OutString("0.0\n");  // Null value not used in this example
}

//Want to send current Speed to Blynk
void SendInformation(void) {
    uint32_t static freq;
    freq = Hall_Frequency();
    //period = 50;
    TM4C_to_Blynk(77, freq);  // VP77
}


int main(void) {
    // volatile uint32_t delay;
    PLL_Init(Bus80MHz);  // now running at 80 MHz
    SysTick_Init();
    PortF_Init();

#ifdef DEBUG1
    UART_Init(5);         // Enable Debug Serial Port
    UART_OutString("\n\rEE445L Lab 4D\n\rBlynk example");
#endif

     ESP8266_Init();       // Enable ESP8266 Serial Port (UART2)
     ESP8266_Reset();      // Reset the WiFi module
     ESP8266_SetupWiFi();  // Setup communications to Blynk Server
     Timer2_Init(&Blynk_to_TM4C, 40000000);
    // check for receive data from Blynk App every 10ms
    Visual_Init();
    //Visual_Crossfade(10000000);
    //Visual_Globe();
    Visual_Music();
    //Visual_Spectrogram();
    //Visual_Off();
     Timer3_Init(&SendInformation, 80000000);
    // Send data back to Blynk App every 1 second
    EnableInterrupts();

    while (1) {
        PF1 ^= 0x02;
    }
}

void PortF_Init(void) {
    // Initialize PF1 and PF2 for Debug
    SYSCTL_RCGCGPIO_R |= 0x20;       // activate port F
    while ((SYSCTL_PRGPIO_R & 0x0020) == 0) {}; // ready?
    GPIO_PORTF_DIR_R |= 0x0E;        // make PF3-1 output (PF3-1 built-in LEDs)
    GPIO_PORTF_AFSEL_R &= ~0x0E;     // disable alt funct on PF3-1
    GPIO_PORTF_DEN_R |= 0x0E;        // enable digital I/O on PF3-1
    // configure PF3-1 as GPIO
    GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R & 0xFFFFF0FF) + 0x00000000;
    GPIO_PORTF_AMSEL_R = 0;          // disable analog functionality on PFe analog functionality on PF
}
