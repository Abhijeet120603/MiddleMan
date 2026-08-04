#include <stdio.h>
#include <stdint.h>
#include "stm32g491xx.h"
#include "uart.h"

char key;


int main(void){
	uart2_tx_rx_init();

	while(1){
		key = UART2_READ;

	}

}


