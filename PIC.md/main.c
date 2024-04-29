#include "mcc_generated_files/mcc.h"
#include "mcc_generated_files/i2c2_master.h"
#include "mcc_generated_files/examples/i2c2_master_example.h"
#define I2C_ADDR_TC74 0x4C
#define HIH6030_I2C_Humidity  0x27
#define ZERO 0x00
#include <string.h>
#include <stdio.h>


/*
                         Main application
 */
uint8_t ReadTemperature = ZERO;
uint8_t fwd = 0b11101111;
uint8_t bckwd = 0b11101101;
uint8_t data ;

float Kp_temp = 0.1; 
float Kp_humid = 0.1;
uint8_t desiredTemp = 28;
float desiredHumid = 40;

float humidityPercentage = 0;

uint8_t PrevTemp = 0;
float PrevHumid = 0;

uint8_t minTemp = 20;
uint8_t maxTemp = 32;
float minHumid = 30.0;
float maxHumid = 45.0;

uint16_t timer_ms = 0;
int time_s = 0;

//uint8_t DataEUSART1;
uint8_t DataEUSART2;

void EUSART2_ISR(void)
    {
	EUSART2_Receive_ISR();
       	 
	if (EUSART2_is_rx_ready())
    {
    DataEUSART2 = EUSART2_Read();
	if (EUSART2_is_tx_ready())
    {
	//EUSART1_Write(DataEUSART2);
  	}
	}
}

// check if a change is needed
bool CompData(uint8_t currentTemp, float currentHumid) {
    //returns true if any value is out of bounds
    if (currentTemp < minTemp || currentTemp > maxTemp || 
        currentHumid < minHumid || currentHumid > maxHumid) {
        // At least one of the values is out of bounds
        return true;
    } else {
        // Both values are within bounds
        return false;
    }
}

//Checks if any change is needed
void ProcessData(uint8_t currentTemp, float currentHumid, bool *tempOver, bool *tempUnder, bool *humidOver, bool *humidUnder) {
    // changes values to true based on their relative to minimum values
    if (currentTemp < minTemp) {
        *tempUnder = true;
    } else if (currentHumid < minHumid) {
        *humidUnder = true;
    } else {
        *tempUnder = false;
        *humidUnder = false;
    }

    // changes values to true based on their relative to maximum values
    if (currentTemp > maxTemp) {
        *tempOver = true;
    } else if (currentHumid > maxHumid) {
        *humidOver = true;
    } else {
        *tempOver = false;
        *humidOver = false;
    }

}

void timer_callback(void) {
    timer_ms = timer_ms +1;
    if (timer_ms > 1000) {
        timer_ms = timer_ms - 1000;
        time_s = time_s + 1;
        // This should print the time, however we found out
        // the eusart has a certain amount of memory allocated for it
        // adding more than 2 total print statements to the code will 
        // cause it to not compile. However this code should interrupt and print
        // the time
        // printf("z Time: %d \n", time_s);
    }
}



void main(void)
{
    // Initialize the device
    SYSTEM_Initialize();
    SPI1_Initialize();
    
    //EUSART1_Initialize();
    EUSART2_Initialize();
    
    EUSART2_ISR();
    

    // If using interrupts in PIC18 High/Low Priority Mode you need to enable the Global High and Low Interrupts
    // If using interrupts in PIC Mid-Range Compatibility Mode you need to enable the Global and Peripheral Interrupts
    // Use the following macros to:
    
    
    //Necessary variables
    bool temp_flag = false;
    bool humid_flag = false;
    
    int vent_state = 0;
    
    bool vent_open = false;
    
    bool tempOver = false;
    bool humidOver = false;
    bool tempUnder = false;
    bool humidUnder = false;
    /*EUSART2_SetRxInterruptHandler(EUSART2_ISR);
    EUSART1_SetRxInterruptHandler(EUSART1_ISR);*/
    
   
    // Enable the Global Interrupts
    INTERRUPT_GlobalInterruptEnable();

    // Disable the Global Interrupts
    //INTERRUPT_GlobalInterruptDisable();

    // Enable the Peripheral Interrupts
    INTERRUPT_PeripheralInterruptEnable();

    // Disable the Peripheral Interrupts
    //INTERRUPT_PeripheralInterruptDisable();
    
    TMR2_SetInterruptHandler(timer_callback);
    
    IO_RC0_SetLow();
    
    //main loop
    while (1)
    {
        // Grab info from temp sensor
        ReadTemperature = I2C2_Read1ByteRegister(I2C_ADDR_TC74, 0x00);

        printf("q Temperature = %u \r\n", ReadTemperature); // print to ESP32
        
        __delay_ms(50);
       
        // set humidity sensor to regular mode
        I2C2_WriteNBytes(HIH6030_I2C_Humidity, 0x0000, 2);
       
        __delay_ms(50);
       
        //Grab byte from humidity sensor and store it in an array
        uint8_t humidityData[2];
        I2C2_ReadNBytes(HIH6030_I2C_Humidity, humidityData, 2);
       
        __delay_ms(50);
       
        // calculate raw humidity from byte
        uint16_t rawHumidity = (humidityData[0] << 8) | humidityData[1];
       
        //calculate RH% from raw humidity
        float humidityPercentage = (rawHumidity / 16382.0) * 100.0;
        printf("x Raw Humidity = %u \r\n", rawHumidity); // prints to ESP32
        
        // Checks if a change is needed
        if (CompData(ReadTemperature, humidityPercentage)) {
            
            // checks to see what type of change is needed
            ProcessData(ReadTemperature, humidityPercentage, &tempOver, &tempUnder, &humidOver, &humidUnder);
            
            float tempError = ReadTemperature - desiredTemp;
            float humidError = humidityPercentage - desiredHumid;
            float tempControlAction = Kp_temp * tempError;
            float humidControlAction = Kp_humid * humidError;
            float controlAvg = (tempControlAction + humidControlAction) / 2;
            
            // Checks change, and the vents state
            if ((tempOver == true || humidOver == true) && vent_open == false) {
            vent_open = true;
            
            //Moves motor forward
            IO_RA5_SetLow(); // Set GPIO Low
            // Declare the data
            data = SPI1_ExchangeByte(fwd);
            // Set GPIO High
            IO_RA5_SetHigh();
            // Delay
            uint16_t delay_ms = (uint16_t)(controlAvg * 1000);
            IO_RA5_SetLow();
            
            
            // Checks change, and the vents state
            } else if ((tempUnder == true || humidUnder == true) && vent_open == true) {
            vent_open = false;
            
            //motor moves backward
            IO_RA5_SetLow(); // Set GPIO Low
            // Declare the data
            data = SPI1_ExchangeByte(bckwd);
            // Set GPIO High
            IO_RA5_SetHigh();
            // Delay
            __delay_ms(1000);
            IO_RA5_SetLow();
            }
            
        }
   
    }
}