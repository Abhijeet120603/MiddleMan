#include "stm32g4xx.h"

#define GPIOA_EN (1U<<0)
#define PIN5 (1U<<5)
#define GPIOC_EN (1U<<2)
#define PIN13 (1U<<13)

int main(void) {
    // Initialize things here (if needed)
	RCC->AHB2ENR |= GPIOA_EN;
	RCC->AHB2ENR |= GPIOC_EN;
	GPIOA->MODER |= (1U<<10);
	GPIOA->MODER &=~ (1U<<11);
	GPIOC->MODER &=~ (1U<<26);
	GPIOC->MODER &=~ (1U<<27);
    while (1) {
        // Main loop
    	if (GPIOC->IDR & PIN13){
    	GPIOA->BSRR |= PIN5;
    	}
    	else{
    	GPIOA->BSRR |= (1U<<21);
    	}
    }
}
