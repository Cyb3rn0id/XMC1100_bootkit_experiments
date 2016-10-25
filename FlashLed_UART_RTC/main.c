/*
 * XMC1100 Bootkit experiment
 * (C)2016 Giovanni Bernardo
 * http://www.settorezero.com
 *
 * This example does:
 * a) alternating led flash using interrupt
 * b) uart tx/rx
 * c) simple use of RTC
 *
 * USIC0 channel 1 is routed on P1.2 and P1.3 that are
 * connected to USB bridge
 *
 * 25 october 2016
 *
 */

#include <xmc_gpio.h>
#include <xmc_uart.h>
#include <xmc_rtc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define LED1 P0_5
#define LED2 P0_6
#define LED3 P1_4
#define LED4 P1_5
#define UART_RX P1_3
#define UART_TX P1_2

#define TICKS_PER_SECOND 1000
#define TICKS_WAIT 1000

// standard messages on UART
const uint8_t welcome[] = "XMC1100 Demo by\n\rGiovanni Bernardo - http://www.settorezero.com\n\r";
const uint8_t err_time[] = "\n\rTime not valid!\n\r";
const uint8_t err_date[] = "\n\rDate not valid!\n\r";
const uint8_t err_dow[] = "\n\rDay not valid!\n\r";
const uint8_t ok_message[] = "\n\rOk!\n\r";
const uint8_t ex_message[] = "\n\rExit\n\r";
const uint8_t pls_time[]="\n\rPlease Set Time (HH:mm:ss)\n\r>";
const uint8_t pls_date[]="\n\rPlease Set Date (dd-mm-yyyy)\n\r>";
const uint8_t pls_dow[]="\n\rPlease Set day of week (0 to 6 where 0=Sunday,1=Monday etc.)\n\r>";

const uint8_t daynames[7][3]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
const uint8_t monthnames[12][3]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

// flags for entering in set mode
bool setTime=false;
bool setDate=false;
bool setDayofweek=false;
bool settings=false;

bool started=false;

uint8_t setTimeDateBuffer[10]; // buffer used to store time and date settings
uint8_t ybuffer[4]; // year buffer for conversion in string
// uint8_t buffer[15]; // used for experiments

XMC_RTC_TIME_t actual_time; // structure used to retrieve date/time values from RTC

// UART configuration
const XMC_UART_CH_CONFIG_t uart_config =
	{
	.data_bits=8,
	.stop_bits=1,
	.baudrate=115200,
	.parity_mode=XMC_USIC_CH_PARITY_MODE_NONE
	};

// RTC Configuration
// 0x7FFF is the default value for prescaler (1:1)
const XMC_RTC_CONFIG_t rtc_conf=
	{
		{
		.seconds=0,
		.minutes=0,
		.hours=23,
		.days=23,
		.daysofweek=XMC_RTC_WEEKDAY_SUNDAY,
		.month=XMC_RTC_MONTH_OCTOBER,
		.year=2016
		},
	.prescaler=0x7FFF
	};

// print strings until terminator \0
void print(const uint8_t *message)
	{
	uint8_t i=0;
	while (message[i]!='\0')
		{
		XMC_UART_CH_Transmit(XMC_UART0_CH1, message[i++]);
		}
	}

// print number of chars
void printn(const uint8_t *message, uint8_t length)
	{
	uint8_t i=0;
	while (i<length)
		{
		XMC_UART_CH_Transmit(XMC_UART0_CH1, message[i++]);
		}
	}

// System Timer Interrupt
void SysTick_Handler(void)
	{
	// does nothing until entering in main loop
	if (!started)
		{
		return;
		}

    static uint32_t ticks = 0;
    ticks++;
    if (ticks == TICKS_WAIT)
  	  {
    	XMC_GPIO_ToggleOutput(LED1);
    	XMC_GPIO_ToggleOutput(LED2);
    	ticks = 0;

    	// Send actual time on UART if not in setting mode
    	if (!settings)
    		{
    		XMC_RTC_GetTime(&actual_time); // retrieve date and time

    		// print time (HH:mm:ss)
    		XMC_UART_CH_Transmit(XMC_UART0_CH1, (actual_time.hours/10)+'0');
    		XMC_UART_CH_Transmit(XMC_UART0_CH1, (actual_time.hours%10)+'0');
    		XMC_UART_CH_Transmit(XMC_UART0_CH1, ':');
    		XMC_UART_CH_Transmit(XMC_UART0_CH1,(actual_time.minutes/10)+'0');
    		XMC_UART_CH_Transmit(XMC_UART0_CH1,(actual_time.minutes%10)+'0');
    		XMC_UART_CH_Transmit(XMC_UART0_CH1, ':');
    		XMC_UART_CH_Transmit(XMC_UART0_CH1, (actual_time.seconds/10)+'0');
    		XMC_UART_CH_Transmit(XMC_UART0_CH1, (actual_time.seconds%10)+'0');

    		// space between date and time
    		XMC_UART_CH_Transmit(XMC_UART0_CH1, ' ');

    		// print date (ddd dd-MMM-yyyy)
    		printn(daynames[actual_time.daysofweek],3);
    		XMC_UART_CH_Transmit(XMC_UART0_CH1, ' ');

    		XMC_UART_CH_Transmit(XMC_UART0_CH1, (actual_time.days/10)+'0');
    		XMC_UART_CH_Transmit(XMC_UART0_CH1, (actual_time.days%10)+'0');
    		XMC_UART_CH_Transmit(XMC_UART0_CH1, '-');

    		//XMC_UART_CH_Transmit(XMC_UART0_CH1,((actual_time.month+1)/10)+'0');
    		//XMC_UART_CH_Transmit(XMC_UART0_CH1,((actual_time.month+1)%10)+'0');
    		printn(monthnames[actual_time.month],3);

    		XMC_UART_CH_Transmit(XMC_UART0_CH1, '-');
    		sprintf((char *)ybuffer,"%d",actual_time.year);
    		print(ybuffer);

    		/*
    		XMC_UART_CH_Transmit(XMC_UART0_CH1, '2');
    		XMC_UART_CH_Transmit(XMC_UART0_CH1, '0');
    		XMC_UART_CH_Transmit(XMC_UART0_CH1, ((actual_time.year-2000)/10)+'0');
    		XMC_UART_CH_Transmit(XMC_UART0_CH1, ((actual_time.year-2000)%10)+'0');
			*/

    		/*
    		// this was just for understand what are raw0 and raw1 elements
    		sprintf((char *)buffer,"%d",actual_time.raw0);
    		XMC_UART_CH_Transmit(XMC_UART0_CH1, ' ');
    		print(buffer,sizeof(buffer));
    		*/

    		// print only carriage return
    		XMC_UART_CH_Transmit(XMC_UART0_CH1, 13); // cr
    		//XMC_UART_CH_Transmit(XMC_UART0_CH1, 10); // lf
    		} // \ not setting mode
  	  	  } // \ticks
	} // \SysTick_Handler

int main(void)
	{
	// Configure UART
	XMC_UART_CH_Init(XMC_UART0_CH1, &uart_config);
	XMC_UART_CH_SetInputSource(XMC_UART0_CH1, XMC_UART_CH_INPUT_RXD,USIC0_C1_DX0_P1_3);

	// interrupt
	/*
	XMC_UART_CH_EnableEvent(XMC_UART0_CH1,XMC_UART_CH_EVENT_STANDARD_RECEIVE);
	XMC_UART_CH_SetInterruptNodePointer(XMC_UART0_CH1,XMC_UART_CH_INTERRUPT_NODE_POINTER_RECEIVE);
	NVIC_SetPriority(USIC0_0_IRQn, 3);
	NVIC_EnableIRQ(USIC0_0_IRQn);
	*/

	// Start UART
	XMC_UART_CH_Start(XMC_UART0_CH1);
	// Configure I/Os
	XMC_GPIO_SetMode(UART_TX, XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT7);
	XMC_GPIO_SetMode(UART_RX, XMC_GPIO_MODE_INPUT_TRISTATE);
	XMC_GPIO_SetMode(LED1, XMC_GPIO_MODE_OUTPUT_PUSH_PULL);
	XMC_GPIO_SetMode(LED2, XMC_GPIO_MODE_OUTPUT_PUSH_PULL);
	XMC_GPIO_SetMode(LED3, XMC_GPIO_MODE_OUTPUT_PUSH_PULL);
	XMC_GPIO_SetMode(LED4, XMC_GPIO_MODE_OUTPUT_PUSH_PULL);

	// NOTE:
	// led turns on when the output is set to LOW and vice-versa
	// see XMC1100 boot kit schematic
	XMC_GPIO_SetOutputLow(LED1); // led ON -> system interrupt will toggle every 1 second
	XMC_GPIO_SetOutputHigh(LED2); // led OFF -> as above
	XMC_GPIO_SetOutputHigh(LED3); // led always OFF
	XMC_GPIO_SetOutputHigh(LED4); // led always OFF

	// Configure RTC
	XMC_RTC_Disable();
	XMC_RTC_Init(&rtc_conf);
	XMC_RTC_Enable();
	XMC_RTC_Start();

	// Configure System Tick
	SysTick_Config(SystemCoreClock / TICKS_PER_SECOND);

	uint8_t rxData=0x00; // buffer byte from UART

	print(welcome); // start-up message
	started=true;

	while(1)
		{
		// if uart contains data...
		if((XMC_UART_CH_GetStatusFlag(XMC_UART0_CH1) & (XMC_UART_CH_STATUS_FLAG_RECEIVE_INDICATION | XMC_UART_CH_STATUS_FLAG_ALTERNATIVE_RECEIVE_INDICATION)) != 0)
	  		{
			// retrieve data from buffer
			rxData = XMC_UART_CH_GetReceivedData(XMC_UART0_CH1);
			// clear received byte flags
			XMC_UART_CH_ClearStatusFlag(XMC_UART0_CH1,XMC_UART_CH_STATUS_FLAG_RECEIVE_INDICATION | XMC_UART_CH_STATUS_FLAG_ALTERNATIVE_RECEIVE_INDICATION );
			// counter for byte received
			static uint8_t byteCount=0;

			// exit from settings on CR or LF
			if ((rxData=='\n') || (rxData=='\r'))
				{
				if (settings)
					{
					setTime=false;
					setDate=false;
					setDayofweek=false;
					settings=false;
					print(ex_message);
					}
				}

			// are we in setting time mode?
			if (setTime)
	  	  	  {
	  		  setTimeDateBuffer[byteCount++]=rxData;
	  		  // re-send byte on uart
	  		  XMC_UART_CH_Transmit(XMC_UART0_CH1, rxData);

	  		  // 8 bytes received (hh:mm:ss)
	  		  if (byteCount==8)
	  		  	  {
	  			  if ((setTimeDateBuffer[2]==':') && (setTimeDateBuffer[5]==':'))
	  			  	  {
	  				  uint8_t sh=0;
	  				  uint8_t sm=0;
	  				  uint8_t ss=0;
	  				  sh=((setTimeDateBuffer[0]-'0')*10)+setTimeDateBuffer[1]-'0';
	  				  sm=((setTimeDateBuffer[3]-'0')*10)+setTimeDateBuffer[4]-'0';
	  				  ss=((setTimeDateBuffer[6]-'0')*10)+setTimeDateBuffer[7]-'0';
	  			  	  // check if data valid
	  				  if ((sh<24) && (sm<60) && (ss<60))
	  			  	  	  {
	  					  // retrieve actual value, update only time
	  			  		  XMC_RTC_GetTime(&actual_time);
	  			  		  XMC_RTC_Disable();
	  			  		  actual_time.hours=sh;
	  			  		  actual_time.minutes=sm;
	  			  		  actual_time.seconds=ss;
						  XMC_RTC_SetTime(&actual_time);
	  			  		  XMC_RTC_Enable();
	  			  		  XMC_RTC_Start();
	  			  		  print(ok_message);
	  			  	  	  }
	  			  	  else // hour/minute/seconds value not valid
	  			  	  	  {
	  			  		  print(err_time);
	  			  	  	  }
	  			  	  }
	  			  else // format not valid (not hh:mm:ss);
	  			  	  {
	  				  print(err_time);
	  			  	  }
	  			  setTime=false;
	  			  settings=false;
	  		  	  }
	  	  	  } // \setTime

			// are we in setting date mode?
			if (setDate)
				{
				setTimeDateBuffer[byteCount++]=rxData;
				// re-send byte on uart
				XMC_UART_CH_Transmit(XMC_UART0_CH1, rxData);

				// 10 bytes received (dd-mm-yyyy)
				if (byteCount==10)
					{
				  	if (((setTimeDateBuffer[2]=='-') && (setTimeDateBuffer[5]=='-')) || ((setTimeDateBuffer[2]=='/') && (setTimeDateBuffer[5]=='/')))
				  		{
				  		uint8_t sd=0;
				  		uint8_t sm=0;
				  		uint16_t sy=0;
				  		sd=((setTimeDateBuffer[0]-'0')*10)+setTimeDateBuffer[1]-'0';
				  		sm=((setTimeDateBuffer[3]-'0')*10)+setTimeDateBuffer[4]-'0';
				  		sy=((setTimeDateBuffer[6]-'0')*1000)+((setTimeDateBuffer[7]-'0')*100);
				  		sy += ((setTimeDateBuffer[8]-'0')*10)+(setTimeDateBuffer[9]-'0');
				  		// check if data valid
				  		if ((sd<32) && (sd>0) && (sm>0) && (sm<13))
				  			{
				  			// retrieve actual value and edit only date
				  			XMC_RTC_GetTime(&actual_time);
				  			XMC_RTC_Disable();
				  			actual_time.days=sd;
				  			actual_time.month=sm-1;
				  			actual_time.year=sy;
				  			XMC_RTC_SetTime(&actual_time);
				  			XMC_RTC_Enable();
				  			XMC_RTC_Start();
				  			print(ok_message);
				  			}
				  		else // day/month/year value not valid
				  			{
				  			print(err_date);
				  			}
				  		}
				  	else // format not valid (not dd-mm-yyyy);
				  	  	{
				  		print(err_date);
				  		}
				  setDate=false;
				  settings=false;
				  }
			} // \setDate

			// are we in setting day of week mode?
			if (setDayofweek)
				{
				// re-send byte on uart
				XMC_UART_CH_Transmit(XMC_UART0_CH1, rxData);

				uint8_t dow=rxData-'0';
		  		// check if data valid
		  		if (dow<7)
					{
					// retrieve actual value and edit only day of week
		  			XMC_RTC_GetTime(&actual_time);
					XMC_RTC_Disable();
					actual_time.daysofweek=dow;
					XMC_RTC_SetTime(&actual_time);
					XMC_RTC_Enable();
					XMC_RTC_Start();
					print(ok_message);
					}
				else // day of week not valid
					{
					print(err_dow);
					}
				setDayofweek=false;
				settings=false;
				}


			// check messages on uart
			if (!settings)
				{
				switch (rxData)
					{
					// set time request
					case 't':
					case 'T':
						setTime=true;
						settings=true;
						byteCount=0;
						print(pls_time);
					 break;

					// set date request
					case 'd':
					case 'D':
						setDate=true;
						settings=true;
						byteCount=0;
						print(pls_date);
					break;

					// set day of week request
					case 'w':
					case 'W':
						setDayofweek=true;
						settings=true;
						print(pls_dow);
					break;

					// help
					case 'h':
					case 'H':
					case '?':
						print((const uint8_t *)"\n\r\n\rPress:\n\r");
						print((const uint8_t *)"t - for time settings\n\r");
						print((const uint8_t *)"d - for date settings\n\r");
						print((const uint8_t *)"w - for day-of-week settings\n\r\n\r");
					break;
					} // \switch(rxData)
			} // \no setTime, no setDate
		  } // \data in the buffer
		} // \while(1)
	} // \main
