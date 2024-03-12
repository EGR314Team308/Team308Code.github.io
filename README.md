Team 308 Code
[Link to PIC](PIC.md/README.md) <br>
[Link to ESP32](ESP32.md/README.md) <br>

#include "mcc_generated_files/mcc.h"
#include "mcc_generated_files/i2c2_master.h"
#include "mcc_generated_files/examples/i2c2_master_example.h"
#define I2C_ADDR_TC74 0x4C
#define ZERO 0x00
#include <string.h>
uint8_t timer_ms = 0;
uint8_t time_s = 0;
float time = 0;
void timer_callback(void)
{
    timer_ms = timer_ms + 1;
    if (timer_ms > 1000)
    {
        timer_ms = timer_ms - 1000;
        time_s = time_s + 1;
    }
}

/*
                         Main application
 */
uint8_t ReadTemperature = ZERO;

void main(void)
{
    // Initialize the device
    SYSTEM_Initialize();

    // If using interrupts in PIC18 High/Low Priority Mode you need to enable the Global High and Low Interrupts
    // If using interrupts in PIC Mid-Range Compatibility Mode you need to enable the Global and Peripheral Interrupts
    // Use the following macros to:

    // Enable the Global Interrupts
    INTERRUPT_GlobalInterruptEnable();

    // Disable the Global Interrupts
    //INTERRUPT_GlobalInterruptDisable();

    // Enable the Peripheral Interrupts
    INTERRUPT_PeripheralInterruptEnable();
    
TMR2_SetInterruptHandler(timer_callback);

    // Disable the Peripheral Interrupts
    //INTERRUPT_PeripheralInterruptDisable();
    while (1)
    {
        timer_callback();
        float time = time_s + timer_ms / 1000.0f;
        printf(" t= %2.3f", time);
        printf("s \n\r");
        ReadTemperature = I2C2_Read1ByteRegister(I2C_ADDR_TC74, 0x00);
        printf("Temperature = %u \r\n", ReadTemperature);
 
    }
}
