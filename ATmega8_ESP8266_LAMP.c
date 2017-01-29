/*
 * ATmega8_ESP8266_LAMP.c
 *
 * Created: 8. 11. 2016 17:18:26
 *  Author: Lord Viktor
 */ 


/*
 * ATmega8A_ESP8266_14_7456MHz_5V.c
 *
 * Created: 9. 8. 2016 15:52:25
 *  Author: Lord Viktor
 */ 

#define F_CPU 14745600UL
#define FOSC 14745600// Clock Speed
#define BAUD 115200
#define MYUBRR FOSC/16/BAUD-1



#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/interrupt.h>



unsigned char counter=0,pole_rx[100],pole_pretriedene[100],Flag=0;

void USART_Init( unsigned int ubrr)
{
	/* Set baud rate */
	UBRRH = 0b00000000;
	UBRRL = 0b00000111;
	/* Enable receiver and transmitter */
	UCSRB = (1<<RXEN)|(1<<TXEN)|(1<<RXCIE);
	/* Set frame format: 8data, 2stop bit */
	UCSRC = (1<<URSEL)|(1<<USBS)|(3<<UCSZ0);
	UCSRA=0b01100000;
	sei();
	/*
	/ * Set baud rate * /
	UBRRH = (unsigned char)(ubrr>>8);
	UBRRL = (unsigned char)ubrr;
	/ * Enable receiver and transmitter * /
	UCSRB = (1<<RXEN)|(1<<TXEN)|(1<<RXCIE);
	/ * Set frame format: 8data, 2stop bit * /
	UCSRC = (1<<URSEL)|(1<<USBS)|(3<<UCSZ0);
	UCSRA|= (1<<U2X);
	sei();*/
}

void USART_Transmit( unsigned char data )
{
	/* Wait for empty transmit buffer */
	while ( !( UCSRA & (1<<UDRE)) );
	/* Put data into buffer, sends the data */
	UDR = data;
}

unsigned char USART_Receive( void )
{
	/* Wait for data to be received */
	while ( !(UCSRA & (1<<RXC)) );
	/* Get and return received data from buffer */
	return UDR;
}
/*
void USART_Flush( void )//vyprazdnenie registra prijatych dat z FIFO
{
	unsigned char dummy;
	while ( UCSRA & (1<<RXC) ) dummy = UDR;
}*/

ISR(USART_RXC_vect)
{	
	pole_rx[counter] = UDR;             //read UART register into value
	if(counter<=99)
	counter++;
	Flag=1;
}

void check_prijate_data (void)//pravdepodobne tu je problem v triedeni pola, treba to debugova5 vo visual studiu v konyolovej aplikacii
{
	unsigned char i=0,a=0;
	while(1)//kontroluje pole pokial sa nevyskytne \r alebo \n
	{
		if((pole_rx[i]=='\r')||(pole_rx[i]=='\n'))
		break;
		
		i++;
	}	
	
	while(1)//bude ingnorovat \r a \n pokial nenarazi na iny znak
	{
		if((pole_rx[i]!='\r')&&(pole_rx[i]!='\n'))
		break;
		
		i++;
	}
	
	while(1)//bude zapisovat do pole_triedene pokial nenarazi na \r alebo \n
	{
		if((pole_rx[i]=='\r')||(pole_rx[i]=='\n'))
		break;
		else
		{
			pole_pretriedene[a]=pole_rx[i];
			a++;
		}
		
		i++;
	}
	pole_pretriedene[99]='\0';
	counter=0;//po skonceni kontroly vynulujem globalny counter
}

void check_prijaty_prikaz(void)
{
	unsigned char i=0,a=0;
	
	while(1)//kontroluje pole pokial sa nevyskytne \r alebo \n
	{
		if(pole_rx[i]==':')
		break;
		
		i++;
	}
	i++;
	
	while(1)//bude zapisovat do pole_triedene pokial nenarazi na \r alebo \n
	{
		if((pole_rx[i]=='\r')||(pole_rx[i]=='\n'))
		break;
		else
		{
			pole_pretriedene[a]=pole_rx[i];
			a++;
		}
		
		i++;
	}
	pole_pretriedene[99]='\0';
	counter=0;//po skonceni kontroly vynulujem globalny counter
}

unsigned char command_compare(void)
{
	check_prijaty_prikaz();
	if (compare_strings(pole_pretriedene,"Test",3)==1)
	{
		return 1;
	}else
	if (compare_strings(pole_pretriedene,"ON",1)==1)//((pocet znakov)-1)
	{
		return 2;
	}else
	if (compare_strings(pole_pretriedene,"OFF",2)==1)//((pocet znakov)-1)
	{
		return 3;
	}else
	if (compare_strings(pole_pretriedene,"on",1)==1)//((pocet znakov)-1)
	{
		return 4;
	}else
	if (compare_strings(pole_pretriedene,"off",2)==1)//((pocet znakov)-1)
	{
		return 5;
	}else
	{
		return 0;
	}
}

int compare_strings(unsigned char prijate[],unsigned char kontrolne[], int kontrolny_pocet)
{
	int i=0;
	do
	{
		if(prijate[i]==kontrolne[i])
		i++;
		else
		break;
	} while(kontrolne[i]!='\0');
	i--;
	
	if (i==kontrolny_pocet)
	return 1;
	else return 0;
}

void USART_Send_String (unsigned char string[])
{
	unsigned char i=0;
	
	do
	{
		USART_Transmit(string[i]);
		i++;
	} while(string[i-1]!='\n');
}

void USART_Send_Command (unsigned char string[])
{
	unsigned char i=0;	
	do
	{
		USART_Transmit(string[i]);
		i++;
	} while(string[i-1]!='\n');
}

void WiFi_Init(void)
{
	char 
	set_mode[]={"AT+CWMODE=2\r\n"},
	ESP_reset[]={"AT+RST\r\n"},
	set_multiple_connection[]={"AT+CIPMUX=1\r\n"},
	Access_point_secured[]={"AT+CWSAP=\"CyberPunk LAMP\",\"beworthy\",1,3\r\n"},
	server[]={"AT+CIPSERVER=1,80\r\n"};
	int delay=600;
	
	/*_delay_ms(200);
	PORTD &=~ (1<<PD2);	//     cbi
	_delay_ms(12);
	PORTD |= (1<<PD2);	//     sbi
	_delay_ms(600);*/
	_delay_ms(1500);
	USART_Send_String(ESP_reset);
	_delay_ms(1500);
	
	USART_Send_String(set_mode);
	_delay_ms(delay);
	
	USART_Send_String(set_multiple_connection);
	_delay_ms(delay);
	
	USART_Send_String(Access_point_secured);
	_delay_ms(delay);
	
	USART_Send_String(server);
	_delay_ms(delay);
}

int main(void)
{
	DDRB=0b11000111;
	DDRC=0b00000000;
	DDRD=0b11111111; //DDRX=0b11111111 su vystupy a DDRX=0b00000000 su vstupy
	PORTB=0b00111000;
	PORTC=0b11111111;
	PORTD=0b00000111;
	
	USART_Init ( 15/*MYUBRR*/ );
	
	char
		CIPSEND_OFF[]={"AT+CIPSEND=0,10\r\n"},
		OFF[]={"Lamp OFF\r\n"},
		CIPSEND_ON[]={"AT+CIPSEND=0,9\r\n"},
		ON[]={"Lamp ON\r\n"};
		int	command_compare_returned_value=9;
		
		PORTD |= (1<<PIND3);	//     sbi
		WiFi_Init();
		
	
	while(1)
	{			
		_delay_ms(10);
		if(Flag==1)
		{
			
			_delay_ms(150);
			command_compare_returned_value=command_compare();
			
			if(command_compare_returned_value==1)
			{
				USART_Send_String("AT+CIPSEND=0,21\r\n");
				_delay_ms(50);
				USART_Send_String("Test prebehol dobre\r\n");
			}else
			if(command_compare_returned_value==2)//ON
			{				
				PORTD |= (1<<PIND3);	//     sbi
				USART_Send_Command(CIPSEND_ON);
				_delay_ms(50);
				USART_Send_Command(ON);
			}else
			if(command_compare_returned_value==3)//OFF
			{
				PORTD &=~ (1<<PIND3);	//     cbi
				USART_Send_Command(CIPSEND_OFF);
				_delay_ms(50);
				USART_Send_Command(OFF);
			}else
			if(command_compare_returned_value==4)//on
			{
				PORTD |= (1<<PIND3);	//     sbi
				USART_Send_Command(CIPSEND_ON);
				_delay_ms(50);
				USART_Send_Command(ON);
			}else
			if(command_compare_returned_value==5)//off
			{				
				PORTD &=~ (1<<PIND3);	//     cbi
				USART_Send_Command(CIPSEND_OFF);
				_delay_ms(50);
				USART_Send_Command(OFF);
			}else
			if(pole_pretriedene[1]!=27)
			{
				USART_Send_Command("AT+CIPSEND=0,5\r\n");
				_delay_ms(50);
				USART_Send_Command("Nop\r\n");
			}	
			command_compare_returned_value=9;
			pole_pretriedene[1]=27;		
			Flag=0;
		}
	}

	
	return 0;
}
