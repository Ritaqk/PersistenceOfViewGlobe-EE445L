//Hall.h
#ifndef __HALL_H__
#define __HALL_H__


// Initialize Halleffect sensor on PB3 and PC4
void Hall_Init(void(*task)(void));

//Get current Spin Frequency
uint32_t Hall_Frequency(void);

#endif //__HALL_H__
