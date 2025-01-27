
#include <mega16.h>
#include <alcd.h>
#include <delay.h>
#include <math.h>
// variables' initialization
char sec = 0;                               //variable to store seconds
char mins = 0;                              //variable to store minutes
char hrs = 0;                               //variable to store hours
char temperature = 0;                        //variable to store temperature
char studentNumber = 0;                     //variable to store student number in the library
void T2_init();                             //Timer2 initialization function based on atmega16 datasheet instructions
char Read_Temperature();                    //function for analog to digital conversion on PA3 and calculating temperature
void Temperature_Management();              //function for choosing the required heater(s) or AC(s) to turn on based on temperature
void Light_Management();                    //function for turning on required light portion(s) base on number of students present

// main code
void main(void)
{
	DDRB = 0b11111011;                      //set PortB inputs and outputs
	PORTB = 0b00001111;                     //set pins of lights as high to be low level triggered & int2 pullup resistor
	DDRC.0 = 1;                             //set the FULL indicator pin as outpot
	PORTD.3 = 1;                            // set int1 pullup resistor
	PORTC.0 = 1;                            //set the FULL indicator pin to be low level triggered
	lcd_init(16);                           //initialize lcd
#asm("sei")                                 //enable global interrupts
	MCUCSR |= (1 << ISC2);                  //set int2 to be rising edge triggered
	MCUCR |= (1 << ISC10) | (1 << ISC11);   //set int1 to be rising edge triggered
	GICR |= (1 << INT2) | (1 << INT1);      //enable int2 & int1
	delay_ms(1000);                         //wait for external clock to stabilize
	T2_init();                              //Timer2 initialization
	ADCSRA = 0b10000000;                    //ADC enable
	temperature = Read_Temperature();       //Read temperature for the 1st time
	//main program loop
	while (1)
		{
		Light_Management();
		Temperature_Management();
		//Show clock on lcd
		lcd_gotoxy(4, 0);
		lcd_printf("%u:%u:%u", hrs, mins, sec);
		if(sec % 5 == 0) temperature = Read_Temperature();           //update temperature if 5 seconds have passed
		//show temperature on lcd
		lcd_gotoxy(0, 1);
		lcd_printf("%u+-1C", temperature);
		//show student number on lcd
		lcd_gotoxy(12, 1);
        //show "FULL" instead of student number and turn on red led if student number >=25
		if (studentNumber >= 25)
			{
			lcd_printf("FULL");
			PORTC.0 = 0;
			}
		else
			{
			lcd_printf("%u", studentNumber);
			PORTC.0 = 1;
			}
		delay_ms(100);
		lcd_clear();
		}
}





//EXT2 interrupt
interrupt [EXT_INT2] void studentLeaving(void)
{
	if (studentNumber > 0)studentNumber--;              //decrement student number by one
}

interrupt [EXT_INT1] void studentEntering(void)
{
	if (studentNumber < 25)studentNumber++;             //increment student number by one
}

//Timer2 overflow inturrupt
interrupt [TIM2_OVF] void t2_ovf_int(void)
{
	sec++;                                              //increment seconds by one
	if(sec > 59) {sec = 0; mins++;}                     //increment minutes and reset seconds when seconds reach 60
	if(mins > 59) {mins = 0; hrs++;}                    //increment hours and reset minutes when minutes reach 60
}

void T2_init()
{
	TIMSK &= ~( (1 << OCIE2) | (1 << TOIE2) );          //disable timer2 interrupts 
	ASSR = (1 << AS2);                                  //Set timer2 to be asynchronous(external clock)
	TCNT2 = 0;                                          //Clear counter
	TCCR2 = 0b00000101;                                 //set prescaler
	while (ASSR & ((1 << TCN2UB) | (1 << TCR2UB)));     //Wait for register update
	TIFR |= (0b11000000) ;                              //clear inturrupt flags
	TIMSK |= 1 << TOIE2;                                //enable timer2 overflow interrupt
}

char Read_Temperature()
{
	char result = 0;         //variable to store adc conversion result
	//set vcc as voltage reference, result to be left adjusted and adc3(PA3) to be used
	ADMUX |= (0 << REFS1) | (1 << REFS0) | (1 << ADLAR) | (0 << MUX4) | (0 << MUX3 ) | (0 << MUX2) | (1 << MUX1) | (1 << MUX0);
	ADCSRA |= (1 << ADSC) | (1 << ADPS2) | (0 << ADPS1) | (1 << ADPS0);                 // set prescaler & start conversion
	while(!(ADCSRA & (1 << ADIF)));         // wait for ADIF conversion complete return
	ADCSRA |= (1 << ADIF);                  //Clear ADIF
	//Calculate result
    if (ADCH >= 0 && ADCH <=25)result = ADCH*2;       
    else if (ADCH >25)result = 50;
    else result = 0;          
	return result;
}

void Temperature_Management()
{
	//turn on 2 heaters if temperature is between 0 & 10
	if (temperature >= 0 && temperature < 10)
		{
		PORTB.4 = 1;
		PORTB.5 = 1;
		PORTB.6 = 0;
		PORTB.7 = 0;
		}
	//turn on 1 heater if temperature is between 10 & 20
	if (temperature >= 10 && temperature < 20)
		{
		PORTB.4 = 1;
		PORTB.5 = 0;
		PORTB.6 = 0;
		PORTB.7 = 0;
		}
	//turn of all heaters & ACs if temperature is between 20 & 30
	if (temperature >= 20 && temperature < 30)
		{
		PORTB.4 = 0;
		PORTB.5 = 0;
		PORTB.6 = 0;
		PORTB.7 = 0;
		}
	//turn on 1 AC if temperature is between 30 & 40
	if (temperature >= 30 && temperature < 40)
		{
		PORTB.4 = 0;
		PORTB.5 = 0;
		PORTB.6 = 1;
		PORTB.7 = 0;
		}
	//turn on 2 ACs if temperature is between 40 & 50
	if (temperature >= 40 && temperature <= 50)
		{
		PORTB.4 = 0;
		PORTB.5 = 0;
		PORTB.6 = 1;
		PORTB.7 = 1;
		}
}

void Light_Management()
{
	//Turn on 1 portion of light if number of students is greater than 0 and less than 10
	if (studentNumber > 0 && studentNumber <= 10)
		{
		PORTB.3 = 1;
		PORTB.1 = 1;
		PORTB.0 = 0;
		}
	//Turn on 2 portions of light if number of students is greater than 10 and less than 20
	if (studentNumber > 10 && studentNumber < 20)
		{
		PORTB.3 = 1;
		PORTB.1 = 0;
		PORTB.0 = 0;
		}
	//Turn on 3 portions of light if number of students is greater than 20
	if (studentNumber >= 20)
		{
		PORTB.3 = 0;
		PORTB.1 = 0;
		PORTB.0 = 0;
		}
	//Turn off alll lights if number of students is 0
	if (studentNumber <= 0)
		{
		PORTB.3 = 1;
		PORTB.1 = 1;
		PORTB.0 = 1;
		}
}