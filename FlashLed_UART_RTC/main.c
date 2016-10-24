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
 * 24 october 2016
 *
 */

#include <xmc_gpio.h>
#include <xmc_uart.h>
#include <xmc_rtc.h>
#include <stdbool.h>

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
const uint8_t pls_time[]="\n\rPlease Set Time (HH:mm:ss)\n\r";
const uint8_t pls_date[]="\n\rPlease Set Date (dd-mm-yy)\n\r";
const uint8_t pls_dow[]="\n\rPlease Set day of week (0 to 6 where 0=Sunday,1=Monday etc.)\n\r";

const uint8_t daynames[7][3]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
const uint8_t monthnames[12][3]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

bool setTime=false;
bool setDate=false;
bool setDayofweek=false;
bool settings=false;

uint8_t setTimeDateBuffer[9];

XMC_RTC_TIME_t actual_time;

// UART configuration
const XMC_UART_CH_CONFIG_t uart_config =
	{
	.data_bits = 8U,
	.stop_bits = 1U,
	.baudrate = 115200
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

// rapid function for printing strings
void print(const uint8_t *message, uint8_t length)
	{
	for (uint8_t i=0; i<length; i++)
		{
		XMC_UART_CH_Transmit(XMC_UART0_CH1, message[i]);
		}
	}

// System Timer Interrupt
void SysTick_Handler(void)
	{
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
    		print (daynames[actual_time.daysofweek],sizeof(daynames[actual_time.daysofweek]));
    		XMC_UART_CH_Transmit(XMC_UART0_CH1, ' ');

    		XMC_UART_CH_Transmit(XMC_UART0_CH1, (actual_time.days/10)+'0');
    		XMC_UART_CH_Transmit(XMC_UART0_CH1, (actual_time.days%10)+'0');
    		XMC_UART_CH_Transmit(XMC_UART0_CH1, '-');

    		//XMC_UART_CH_Transmit(XMC_UART0_CH1,((actual_time.month+1)/10)+'0');
    		//XMC_UART_CH_Transmit(XMC_UART0_CH1,((actual_time.month+1)%10)+'0');
    		print(monthnames[actual_time.month],sizeof(monthnames[actual_time.month]));

    		XMC_UART_CH_Transmit(XMC_UART0_CH1, '-');
    		XMC_UART_CH_Transmit(XMC_UART0_CH1, '2');
    		XMC_UART_CH_Transmit(XMC_UART0_CH1, '0');
    		XMC_UART_CH_Transmit(XMC_UART0_CH1, ((actual_time.year-2000)/10)+'0');
    		XMC_UART_CH_Transmit(XMC_UART0_CH1, ((actual_time.year-2000)%10)+'0');

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
	// Start UART
	XMC_UART_CH_Start(XMC_UART0_CH1);
	// Configure I/Os
	XMC_GPIO_SetMode(UART_TX, XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT7);
	XMC_GPIO_SetMode(UART_RX, XMC_GPIO_MODE_INPUT_TRISTATE);
	XMC_GPIO_SetMode(LED1, XMC_GPIO_MODE_OUTPUT_PUSH_PULL);
	XMC_GPIO_SetMode(LED2, XMC_GPIO_MODE_OUTPUT_PUSH_PULL);
	XMC_GPIO_SetMode(LED3, XMC_GPIO_MODE_OUTPUT_PUSH_PULL);
	XMC_GPIO_SetMode(LED4, XMC_GPIO_MODE_OUTPUT_PUSH_PULL);
	XMC_GPIO_SetOutputHigh(LED1);
	XMC_GPIO_SetOutputLow(LED2);
	XMC_GPIO_SetOutputHigh(LED3);
	XMC_GPIO_SetOutputHigh(LED4);
	// Configure RTC
	XMC_RTC_Disable();
	XMC_RTC_Init(&rtc_conf);
	XMC_RTC_Enable();
	XMC_RTC_Start();
	// Configure System Tick
	SysTick_Config(SystemCoreClock / TICKS_PER_SECOND);

	uint8_t rxData=0x00; // buffer byte from UART

	print(welcome,sizeof(welcome));

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
	  			  		  print(ok_message,sizeof(ok_message));
	  			  	  	  }
	  			  	  else // hour/minute/seconds value not valid
	  			  	  	  {
	  			  		  print(err_time,sizeof(err_time));
	  			  	  	  }
	  			  	  }
	  			  else // format not valid (not hh:mm:ss);
	  			  	  {
	  				  print(err_time,sizeof(err_time));
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

				// 8 bytes received (hh:mm:ss)
				if (byteCount==8)
					{
				  	if ((setTimeDateBuffer[2]=='-') && (setTimeDateBuffer[5]=='-'))
				  		{
				  		uint8_t sd=0;
				  		uint8_t sm=0;
				  		uint8_t sy=0;
				  		sd=((setTimeDateBuffer[0]-'0')*10)+setTimeDateBuffer[1]-'0';
				  		sm=((setTimeDateBuffer[3]-'0')*10)+setTimeDateBuffer[4]-'0';
				  		sy=((setTimeDateBuffer[6]-'0')*10)+setTimeDateBuffer[7]-'0';
				  		// check if data valid
				  		if ((sd<32) && (sm<13) && (sy<100))
				  			{
				  			// retrieve actual value and edit only date
				  			XMC_RTC_GetTime(&actual_time);
				  			XMC_RTC_Disable();
				  			actual_time.days=sd;
				  			actual_time.month=sm-1;
				  			actual_time.year=2000+sy;
				  			XMC_RTC_SetTime(&actual_time);
				  			XMC_RTC_Enable();
				  			XMC_RTC_Start();
				  			print(ok_message,sizeof(ok_message));
				  			}
				  		else // day/month/year value not valid
				  			{
				  			print(err_date,sizeof(err_date));
				  			}
				  		}
				  	else // format not valid (not dd-mm-yy);
				  	  	{
				  		print(err_date,sizeof(err_date));
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
					print(ok_message,sizeof(ok_message));
					}
				else // day of week not valid
					{
					print(err_dow,sizeof(err_dow));
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
						print(pls_time,sizeof(pls_time));
					 break;

					// set date request
					case 'd':
					case 'D':
						setDate=true;
						settings=true;
						byteCount=0;
						print(pls_date,sizeof(pls_date));
					break;

					// set day of week request
					case 'w':
					case 'W':
						setDayofweek=true;
						settings=true;
						print(pls_dow,sizeof(pls_dow));
					break;

					// help
					case 'h':
					case 'H':
					case '?':
						print((const uint8_t *)"\n\r\n\rPress:\n\r",12);
						print((const uint8_t *)"t - for time settings\n\r",23);
						print((const uint8_t *)"d - for date settings\n\r",23);
						print((const uint8_t *)"w - for day-of-week settings\n\r\n\r",32);
					break;
					} // \switch(rxData)


			if ((rxData=='\n') || (rxData=='\r'))
				{
				if (settings)
					{
					setTime=false;
					setDate=false;
					setDayofweek=false;
					settings=false;
					print(ex_message,sizeof(ex_message));
					}
				}

			} // \no setTime, no setDate
		  } // \data in the buffer
		} // \while(1)
	} // \main
