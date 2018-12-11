// SK9822.c
// Runs on TM4C123
// Use SSI3 to communicate with LED strip
// Rita Kambil
// 10/25/18

// SSI3Clk to clock line  connected to PD0
// SSI3Tx to data line connected to PD3
// see Volume 2 Figure 7.19 for complete schematic
#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"


#define SSI_CR0_SCR_M           0x0000FF00  // SSI Serial Clock Rate
#define SSI_CR0_SPH             0x00000080  // SSI Serial Clock Phase
#define SSI_CR0_SPO             0x00000040  // SSI Serial Clock Polarity
#define SSI_CR0_FRF_M           0x00000030  // SSI Frame Format Select
#define SSI_CR0_FRF_MOTO        0x00000000  // Freescale SPI Frame Format
#define SSI_CR0_DSS_M           0x0000000F  // SSI Data Size Select
#define SSI_CR0_DSS_16          0x0000000F  // 16-bit data
#define SSI_CR1_MS              0x00000004  // SSI Master/Slave Select
#define SSI_CR1_SSE             0x00000002  // SSI Synchronous Serial Port
#define SSI_SR_RNE              0x00000004  // SSI Receive FIFO Not Empty
#define SSI_SR_TNF              0x00000002  // SSI Transmit FIFO Not Full
// Enable
#define SSI_CPSR_CPSDVSR_M      0x000000FF  // SSI Clock Prescale Divisor
#define SSI_DMACTL_TXDMAE       0x00000002  // Transmit DMA Enable
#define SSI_DMACTL_RXDMAE       0x00000001  // Receive DMA Enable


//********LED_Init*****************
// Initialize SK9822 LED strip
// inputs:  none
// outputs: none
// assumes: system clock rate of 80 MHz
void LED_Init(void) {
    volatile uint32_t delay;
    SYSCTL_RCGCSSI_R |= 0x08;             // activate SSI3
    SYSCTL_RCGCGPIO_R |= 0x08;            // activate port D
    delay = SYSCTL_RCGC2_R;               // allow time to finish activating

    GPIO_PORTD_AFSEL_R |= 0x09;           // enable alt funct on PD0, PD3
    GPIO_PORTD_DEN_R |= 0x09;             // enable digital I/O on PD0, PD3
    // Enable SSI on PD0, 3
    GPIO_PORTD_PCTL_R = (GPIO_PORTD_PCTL_R & 0xFFFF0FF0) + 0x00001001;
    GPIO_PORTD_AMSEL_R = 0;               // disable analog functionality on PD

    SSI3_CR1_R &= ~SSI_CR1_SSE;           // disable SSI
    SSI3_CR1_R &= ~SSI_CR1_MS;            // master mode
    // clock divider for 8 MHz SSIClk (assumes 80 MHz PLL)
    SSI3_CR0_R |=  0x00000000;          // SCR = 3 so denom = 4, 80MHz/4 = 20 MHz SSI Clock

    SSI3_CPSR_R = (SSI3_CPSR_R & ~SSI_CPSR_CPSDVSR_M) + 8 ; // 8 = 10MHz 4 = 20 MHz
    SSI3_CR0_R &= ~(SSI_CR0_SCR_M |       // SCR = 0 (8 Mbps data rate)
                    SSI_CR0_SPH |         // SPH = 0
                    SSI_CR0_SPO);         // SPO = 0
    // FRF = Freescale format
    SSI3_CR0_R = (SSI3_CR0_R & ~SSI_CR0_FRF_M) + SSI_CR0_FRF_MOTO;
    SSI3_CR0_R = (SSI3_CR0_R & ~SSI_CR0_DSS_M) + 0xF;   //16 bit dss
    SSI3_DR_R = 0;                     // load 'data' into transmit FIFO
    SSI3_CR1_R |= SSI_CR1_SSE;            // enable SSI
}
