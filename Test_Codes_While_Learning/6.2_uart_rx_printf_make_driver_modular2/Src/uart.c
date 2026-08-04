/*
 * uart.c
 *
 *  Created on: Aug 13, 2025
 *      Author: abhij
 */
#include "uart.h"

#define SYS_FREQ 16000000
#define APB2_CLK 16000000

#define UART_BAUDRATE 115200

#define USART_TE_EN_CR1 (1U<<3)
#define USART_EN_CR1 (1U<<0)
#define SR_TXE (1U<<7)

#define USART_RX_EN_CR1 (1U<<2)
#define SR_RXFNE (1U<<5)


static void usart_set_baudrate(USART_TypeDef *USARTx, uint32_t Periph_clk, uint32_t Baudrate);
static uint16_t COMPUTE_UART_BAUD(uint32_t Periph_clk, uint32_t Baudrate);


void UART2_WRITE(int ch);

int __io_putchar(int ch){
	UART2_WRITE(ch);
	return ch;
}



void uart2_tx_rx_init(void){
	/* ..............configure UART gpio pin.........*/
	/*configure clk access to gpioa
	 * set pa9 mode to alternate function
	 * sett pa9 alternte function type to uart_tx (af07) */

	RCC->AHB2ENR |= (1U<<0);

	GPIOA->MODER &=~ (1U<<18);
	GPIOA->MODER |= (1U<<19);

	GPIOA->AFR[1] |= (1U<<4);
	GPIOA->AFR[1] |= (1U<<5);
	GPIOA->AFR[1] |= (1U<<6);
	GPIOA->AFR[1] &=~ (1U<<7);

	//for rx enable pa10
	GPIOA->MODER &=~ (1U<<20);
	GPIOA->MODER |= (1U<<21);

	GPIOA->AFR[1] |= (1U<<8);
	GPIOA->AFR[1] |= (1U<<9);
	GPIOA->AFR[1] |= (1U<<10);
	GPIOA->AFR[1] &=~ (1U<<11);




	/* ..............configure UART module.........*/
	/*enable clk access to uart2
	 * configure baudrate
	 * configure transfer direction
	 * enable uart module */
	RCC->APB2ENR |= (1U<<14); //uart2en
	usart_set_baudrate(USART1,APB2_CLK,UART_BAUDRATE);
	USART1->CR1 = (USART_TE_EN_CR1 | USART_RX_EN_CR1);
	USART1->CR1 |= USART_EN_CR1;

}

char UART2_READ(void){
		//MAKE SURE READ DATA REGISTER IS NOT EMPTY
	while(!(USART1->ISR & SR_RXFNE)){}
	//READ DATA
	return USART1->RDR;

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
