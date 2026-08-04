// pa5 for led
// pc13 for push button

#define AHB2_BASE_ADDRESS (0x48000000UL)
#define GPIOA_BASE_ADDRESS (0x48000000UL)
#define GPIOC_BASE_ADDRESS (0x48000800UL)

#define RCC_BASE_ADDRESS (0X40021000UL)

#define RCC_AHB2_EN_R_OFFSET (0X4CUL)
#define RCC_AHB2_EN (*(volatile unsigned int*) (RCC_BASE_ADDRESS + RCC_AHB2_EN_R_OFFSET))

#define GPIO_A_RCC_EN (1U << 0)
#define GPIO_C_RCC_EN (1U << 2)

#define MODER_OFFSET (0X00UL)
#define GPIO_A_MODER (*(volatile unsigned int*) (GPIOA_BASE_ADDRESS + MODER_OFFSET))
#define GPIO_C_MODER (*(volatile unsigned int*) (GPIOC_BASE_ADDRESS + MODER_OFFSET))

#define ODR_OFFSET (0X14UL)
#define PA5_ODR (*(volatile unsigned int*) (GPIOA_BASE_ADDRESS + ODR_OFFSET))

#define IDR_OFFSET (0X10UL)
#define PC13_IDR (*(volatile unsigned int*) (GPIOC_BASE_ADDRESS + IDR_OFFSET))

#define PA5  (1U << 5)

int main(void){
	RCC_AHB2_EN |= GPIO_A_RCC_EN;
	RCC_AHB2_EN |= GPIO_C_RCC_EN;

	GPIO_A_MODER |= (1U << 10);
	GPIO_A_MODER &=~ (1U << 11);

	GPIO_C_MODER &=~ (1U << 26);
	GPIO_C_MODER &=~ (1U << 27);

	while(1){
		if ((PC13_IDR & (1U << 13)) == 0)  // Assuming active-low button
		{
			PA5_ODR &= ~PA5; // Turn OFF LED
		}
		else
		{
			PA5_ODR |= PA5;  // Turn ON LED
		}

	}

}
