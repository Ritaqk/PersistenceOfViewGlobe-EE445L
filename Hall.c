// Hall.c
// TM4C123

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"

#define NVIC_ST_CTRL_COUNT      0x00010000  // Count flag
#define NVIC_ST_CTRL_CLK_SRC    0x00000004  // Clock Source
#define NVIC_ST_CTRL_INTEN      0x00000002  // Interrupt enable
#define NVIC_ST_CTRL_ENABLE     0x00000001  // Counter mode
#define NVIC_ST_RELOAD_M        0x00FFFFFF  // Counter load value
#define PF1       (*((volatile uint32_t *)0x40025008))

void (*UserTask)(void);   // user function
uint32_t HallCount = 0;
uint32_t Freq_Hz = 0;

// Hall effet sensor wired to both PB3 and PC4
// Input cap for frequency measure on Timer3 PB3 T3CCP1
void Hall_Init(void(*task)(void)) {
    SYSCTL_RCGCWTIMER_R |= 0x01;  // activate WTimer0
    SYSCTL_RCGCGPIO_R |= 0x02;       // activate port C and B
    while ((SYSCTL_PRGPIO_R & 0x02) == 0) {}; // ready?
    UserTask = task;

    // PB3 init
    GPIO_PORTB_DIR_R &= ~0x08;        // make PB3 Input
    GPIO_PORTB_AFSEL_R |= 0x08;     // enable alt funct on PB3
    GPIO_PORTB_DEN_R |= 0x08;        // enable digital I/O on PB3
    GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R & 0xFFFF0FFF); // Normal GPIO
    GPIO_PORTB_IS_R &= ~0x08;     // (d) PB3 is edge-sensitive
    GPIO_PORTB_IBE_R &= ~0x08;    //     PB3 is not both edges
    GPIO_PORTB_IEV_R &= ~0x08;    //     PB3 falling edge event
    GPIO_PORTB_ICR_R = 0x08;      // (e) clear flag3
    GPIO_PORTB_IM_R |= 0x08;      // (f) arm interrupt on PB3
    NVIC_PRI0_R = (NVIC_PRI0_R & 0xFFFF00FF) | 0x00002000; // 8) priority 1
    // interrupts enabled in the main program after all devices initialized
    // vector number 17, interrupt number 1
    NVIC_EN0_R = 1 << 1;  // 9) enable IRQ 1 in NVIC

    WTIMER0_CTL_R = 0x00000000;     // 1) disable WTIMER0A during setup
    WTIMER0_CFG_R = 0x00000004;     // 2) configure for 32-bit split mode
    WTIMER0_TAMR_R = 0x00000002;    // 3) configure for periodic mode, default down-count settings
    WTIMER0_TAILR_R = 79999999;     // 1 Hz

    WTIMER0_TBMR_R = 0x00000003;     // Edge count, capture mode 011
    WTIMER0_TBILR_R = 0xFFFFFFFF;    // start value
    // bits 11-10 in CTL are 0 for rising edge

    WTIMER0_ICR_R = 0x00000001;    // 6) clear WTIMER0A timeout flag
    WTIMER0_IMR_R = 0x00000001;    // 7) arm timeout interrupt

    NVIC_PRI23_R = (NVIC_PRI23_R & 0xFF00FFFF) | 0x00200000; // 8) priority 1
    // interrupts enabled in the main program after all devices initialized
    // vector number 110, interrupt number 94
    NVIC_EN2_R = 1 << 30;  // 9) enable IRQ 35 in NVIC
    WTIMER0_CTL_R = 0x00000101;    // 10) enable WTIMER0A AND wTIMER0B
}

// Hz is same as RPS
uint32_t Hall_Frequency(void) {
    return Freq_Hz;
}

void WideTimer0A_Handler(void) {
    PF1 ^= 0x02;
    WTIMER0_ICR_R = 0x00000001;      // acknowledge timer0A timeout
    Freq_Hz = HallCount; // f = (pulses)/(fixed time)
    HallCount = 0;
}

void GPIOPortB_Handler(void) {
    GPIO_PORTB_ICR_R = 0x08;      // (e) clear flag3
    HallCount += 1;
    (*UserTask)();
}
