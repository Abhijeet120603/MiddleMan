#include <stdio.h>
#include <stdint.h>

#define AHB2_base_address (0x48000000UL)
#define GPIOA_base_address (0x48000000UL)
#define AHB1_base_address (0x40020000UL)
#define RCC_base (0X40021000UL)

#define GPIOA_RCC_EN (1U<<0)
#define PIN5 (1U<<5)
#define LED_PIN PIN5

typedef struct{
	volatile uint32_t GPIO_MODER;
	volatile uint32_t FALTU[4];
	volatile uint32_t ODR;
}GPIO_TYPEDEF;

typedef struct{
	volatile uint32_t FALTU[14];
	volatile uint32_t RCC_AHB2_EN_R;
}RCC_TYPEDEF;

#define RCC ((RCC_TYPEDEF*)RCC_base)
#define GPIO ((GPIO_TYPEDEF*)GPIOA_base_address)

int main(void){
	/* 1 en clk access to gpio A
	 * 2 set pa5 as output*/

	RCC->RCC_AHB2_EN_R |= GPIOA_RCC_EN;

	GPIO->GPIO_MODER |= (1U<<10);
	GPIO->GPIO_MODER &=~ (1U<<11);

	while(1){
		/* 3 set pa5 high */

		GPIO->ODR ^= PIN5;
		for(int i=0;i<=100000;i++){}

	}
}
