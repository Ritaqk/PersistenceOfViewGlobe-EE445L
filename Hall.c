// Hall.c
// TM4C123

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"

#define NVIC_ST_CTRL_COUNT      0x00010000  // Count flag
#define NVIC_ST_CTRL_CLK_SRC    0x00000004  // Clock Source
#define NVIC_ST_CTRL_INTEN      0x00000002  // Interrupt enable
#define NVIC_ST_CTRL_ENABLE     0x00000001  // Counter mode
#define NVIC_ST_RELOAD_M        0x00FFFFFF  // Counter load value

void (*UserTask)(void);   // user function
uint32_t Last = 0;
uint32_t Now;
uint32_t Time_diff;
uint32_t Period_sec;


void Hall_Init(void(*task)(void)) {
    // Initialize PF1 and PF2 for Debug
    SYSCTL_RCGCGPIO_R |= 0x08;       // activate port D
    while ((SYSCTL_PRGPIO_R & 0x08) == 0) {}; // ready?
    GPIO_PORTD_DIR_R &= ~0x01;        // make PD0 Input
    GPIO_PORTD_AFSEL_R &= ~0x01;     // disable alt funct on PD0
    GPIO_PORTD_DEN_R |= 0x01;        // enable digital I/O on PD0
    GPIO_PORTD_PCTL_R = (GPIO_PORTD_PCTL_R & 0xFFFFFFF0);
    GPIO_PORTD_AMSEL_R = 0;          // disable analog functionality on PD analog functionality on PD
    UserTask = task;
    GPIO_PORTD_IS_R &= ~0x01;     // (d) PD0 is edge-sensitive
    GPIO_PORTD_IBE_R &= ~0x01;    // PD0 is not both edges
    GPIO_PORTD_IEV_R |= 0x01;    // PD0 rising edge event
    GPIO_PORTD_ICR_R = 0x01;      // (e) clear flag0
    GPIO_PORTD_IM_R |= 0x01;      // (f) arm interrupt on PD0
    NVIC_PRI0_R = (NVIC_PRI0_R & 0x00FFFFFF) | 0x20000000; // (g) priority 1
    NVIC_EN0_R = 1 << 3;      // (h) enable interrupt 19 in NVIC
    Last = NVIC_ST_CURRENT_R;
}

uint32_t Hall_Period(void) {
    return Period_ns;
}

// Get the time between interrupts and calculate current speed
void GPIOPortD_Handler(void) {
    GPIO_PORTD_ICR_R = 0x01;      // acknowledge flag4
    Now = NVIC_ST_CURRENT_R;
    if (Now > Last) {
        Time_diff = last + (0x00FFFFFF - now); // Handle Overflow
    }
    else {
        Time_diff = last - now;
    }
    Period_ns = (25 * Time_diff)/2;
    (*UserTask)(); 
}
