#include <stdint.h>
#include "stm32g491xx.h"

#define SYS_FREQ 16000000
#define APB2_CLK 16000000

#define UART_BAUDRATE 115200

#define USART_TE_EN_CR1 (1U<<3)
#define USART_EN_CR1 (1U<<0)
#define SR_TXE (1U<<7)

static void usart_set_baudrate(USART_TypeDef *USARTx, uint32_t Periph_clk, uint32_t Baudrate);
static uint16_t COMPUTE_UART_BAUD(uint32_t Periph_clk, uint32_t Baudrate);

void uart2_tx_init(void);
void UART2_WRITE(int ch);

int main(void){
	uart2_tx_init();

	while(1){
		UART2_WRITE('3');

	}

}

void uart2_tx_init(void){
	/* ..............configure UART gpio pin.........*/
	/*configure clk access to gpioa
	 * set pa2 mode to alternate function
	 * sett pa2 alternte function type to uart_tx (af07) */

	RCC->AHB2ENR |= (1U<<0);

	GPIOA->MODER &=~ (1U<<18);
	GPIOA->MODER |= (1U<<19);

	GPIOA->AFR[1] |= (1U<<4);
	GPIOA->AFR[1] |= (1U<<5);
	GPIOA->AFR[1] |= (1U<<6);
	GPIOA->AFR[1] &=~ (1U<<7);


	/* ..............configure UART module.........*/
	/*enable clk access to uart2
	 * configure baudrate
	 * configure transfer direction
	 * enable uart module */
	RCC->APB2ENR |= (1U<<14); //uart2en
	usart_set_baudrate(USART1,APB2_CLK,UART_BAUDRATE);
	USART1->CR1 = USART_TE_EN_CR1;
	USART1->CR1 |= USART_EN_CR1;

}

void UART2_WRITE(int ch){
	/* Make sure the transmit data register is empty*/
	while(!(USART1->ISR & SR_TXE)){

	}
	/* Write to transmit data register*/
	USART1->TDR = (ch & 0xFF);
}

static void usart_set_baudrate(USART_TypeDef *USARTx, uint32_t Periph_clk, uint32_t Baudrate){
	USARTx->BRR = COMPUTE_UART_BAUD(Periph_clk, Baudrate);
}

static uint16_t COMPUTE_UART_BAUD(uint32_t Periph_clk, uint32_t Baudrate){
	return ((Periph_clk + (Baudrate/2U))/Baudrate);
}
