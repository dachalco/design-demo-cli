/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "FreeRTOS_CLI.h"

#include "driver/uart.h"


#define UART_CLI UART_NUM_0
#define UART_CLI_

void app_main(void)
{
    /* Setup CLI */
    uart_config_t xUartConfig = { 0 };
    xUartConfig.baud_rate = 115200;
    xUartConfig.data_bits = UART_DATA_8_BITS;
    xUartConfig.parity = UART_PARITY_DISABLE;
    xUartConfig.stop_bits = UART_STOP_BITS_1;
    xUartConfig.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_param_config( UART_CLI, &xUartConfig );

    ESP_ERROR_CHECK( uart_set_pin( UART_CLI,  ));

    /* Get handles to CLI streams and tie to UART interrupt handler */

    /* Scheduler is started just after app_main exit */
}
