#define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>

//Задержка между миганиями значенями на семисегментном индикаторе
#define DELAY 400	

uint8_t display[3][3] = {{0xF9,0x99,0xF8},		//1 4 7
			 {0xA4,0x92,0x80},		//2 5 8
			 {0xB0,0x82,0x90}};		//3 6 9

int makeRandom(uint16_t upper) 
{
	return (uint16_t)((double)rand() / ((double)RAND_MAX + 1) * upper);
}

uint8_t toDisplayNum(uint8_t num)
{
	uint8_t i,j;
	
	i = num / 3;
	j = num % 3;
	
	return display[i][j];
}

void comboPort(uint8_t num)
{
	PORTC = num & 0b00000011;
	PORTB = num & 0b11111100;
}

void initUART()
{
	//UBRR=f/(16*band)-1 f=8000000Гц band=9600,
	UBRRH=0;
	UBRRL=51;
	UCSRA=0x00;
	//Разрешение работы передатчика
	UCSRB=(1<<TXEN);	
	//Установка формата посылки: 8 бит данных, 1 стоп-бит
	UCSRC=(1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0);	
}

void sendByte(uint8_t b)
{
	//Устанавливается, когда регистр свободен
	while(!(UCSRA & (1 << UDRE)));	
	UDR = b;
}

void sendStr(uint8_t *s)
{
	while (*s) sendByte(*s++);
}

void sendNum(uint8_t num)
{
	if (num > 99) sendByte(num / 100 + '0');
	if (num > 9) sendByte((num % 100) / 10 + '0');
	sendByte(num % 10 + '0');
}

int main(void)
{
	uint8_t i, j, randomNumber, tempNumber, buttonCount, n = 0;
	
	uint8_t  portState[3] = {0xDF,0xBF,0x7F};
	uint8_t inputState[3] = {0x04,0x08,0x10};
								 
	uint8_t* arr;
	uint8_t* backup_arr;
	
	//Источник энтропии - счетчик задержки нажатия клавиш
	uint16_t delayCounter = eeprom_read_word((uint16_t*)1);
	
	DDRC  = 0xFF;
	DDRB  = 0xFF;
	
	DDRD  = 0xE3;
	PORTD = 0xFF;
	
	initUART();

    while (1) 
    {	
				
		arr = (uint8_t *)malloc((n + 1) * sizeof(uint8_t));
	
		if (n > 0) {
			memcpy(arr, backup_arr, n);
			free(backup_arr);
		}
	
		tempNumber = eeprom_read_byte((uint8_t*)0);
		srand(delayCounter);
		tempNumber ^= (makeRandom(1000)%255);
		eeprom_write_byte((uint8_t*)0, (uint8_t)(tempNumber % 8));
		randomNumber = eeprom_read_byte((uint8_t*)0);
		
		arr[n] = toDisplayNum(randomNumber);
		
		for (i = 0; i < n + 1; i++)
		{
			comboPort(arr[i]);
			_delay_ms(DELAY);
			comboPort(0x00);
			_delay_ms(DELAY);
		}
		buttonCount = 0;
		while(buttonCount < n + 1)
		{
			for(i = 0; i < 3; i++)
			{
				PORTD = portState[i];
				for(j = 0; j < 3; j++)
				{
					if(!(PIND & inputState[j]))
					{
						while((PIND & inputState[j]) != inputState[j]);
						if(display[i][j] == arr[buttonCount]){
							comboPort(display[i][j]);
							buttonCount++;	
						} else {
							sendStr("Game over! Your score: ");
							sendNum(n);
							sendByte(13);
							//Мигаем GG
							while(1) 
							{
								comboPort(0xC2);
								_delay_ms(DELAY);
								comboPort(0x00);
								_delay_ms(DELAY);
							}
						}
					}
					delayCounter++;
				}
			}
		}
		
		backup_arr = (uint8_t*)malloc((n + 1) * sizeof(uint8_t));
		memcpy(backup_arr, arr, n + 1);
		free(arr);

		_delay_ms(DELAY);
		comboPort(0x00);
		_delay_ms(DELAY * 2);
		
		eeprom_write_word((uint16_t*)1, (uint16_t)(delayCounter));
		
		n++;
    }
}

