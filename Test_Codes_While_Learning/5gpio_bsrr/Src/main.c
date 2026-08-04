#include "stm32g4xx.h"

#define GPIOA_EN (1U<<0)
#define PIN5 (1U<<5)

int main(void) {
    // Initialize things here (if needed)
	RCC->AHB2ENR |= GPIOA_EN;
	GPIOA->MODER |= (1U<<10);
	GPIOA->MODER &=~ (1U<<11);
    while (1) {
        // Main loop
    	GPIOA->BSRR |= PIN5;
    	for(int i=0;i<=100000;i++){}
    	GPIOA->BSRR |= (1U<<21);
    	for(int i=0;i<=100000;i++){}
    }
}
