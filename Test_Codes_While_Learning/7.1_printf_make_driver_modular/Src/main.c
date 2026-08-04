#include <stdio.h>
#include <stdint.h>
#include "stm32g491xx.h"
#include "uart.h"


int main(void){
	uart2_tx_init();

	while(1){
		printf("hello \n\r");

	}

}


