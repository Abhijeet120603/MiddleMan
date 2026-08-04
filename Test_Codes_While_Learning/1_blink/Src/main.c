#define AHB2_base_address (0x48000000UL)
#define GPIOA_base_address (0x48000000UL)
#define AHB1_base_address (0x40020000UL)
#define RCC_base (0X40021000UL)

#define AHB2_EN_R_OFFSET (0x4cUL)
#define AHB2_RCC_EN (*(volatile unsigned int*) (RCC_base + AHB2_EN_R_OFFSET))

#define GPIOA_RCC_EN (1U<<0)

#define GPIOA_MODER_OFFSET (0X00UL)
#define GPIOA_MODER (*(volatile unsigned int*) (GPIOA_base_address + GPIOA_MODER_OFFSET))

#define GPIOA_OD_OFFSET (0X14UL)
#define GPIOA_OD (*(volatile unsigned int*) (GPIOA_base_address + GPIOA_OD_OFFSET))
#define PIN5 (1U<<5)

#define LED_PIN PIN5

int main(void){
	/* 1 en clk access to gpio A
	 * 2 set pa5 as output*/

	AHB2_RCC_EN |= GPIOA_RCC_EN;
	GPIOA_MODER |= (1U<<10);
	GPIOA_MODER &=~ (1U<<11);

	while(1){
		/* 3 set pa5 high */
//		GPIOA_OD &= ~(PIN5); // to make all bits 0
//		GPIOA_OD |= PIN5; // to make led on
		GPIOA_OD ^= PIN5; // toggle led
		for(int i=0;i<=100000;i++){}

	}
}
