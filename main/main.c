/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/stream_buffer.h"

#include "FreeRTOS_CLI.h"

#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_intr_alloc.h"
#include "soc/uart_struct.h"
#include "soc/uart_reg.h"

#include "esp_log.h"

#define UART_CLI  ( UART_NUM_1 )
#define RX_PIN    ( GPIO_NUM_18 )
#define TX_PIN    ( GPIO_NUM_19 )
#define BUF_SIZE  ( 512 )
#define TAG       "UART-FEED"

static QueueHandle_t xUartEvents = NULL;

#ifdef WAS_ABLE_TO_GET_UART_ISR_WORKING
static void IRAM_ATTR isr_uart_handler( void * arg )
{
    BaseType_t xShouldYield = pdFALSE;
    uint16_t irq_status = UART1.int_st.val;
    uint16_t n_read = UART1.status.rxfifo_cnt;
    uint8_t byte = 0u;

    /* Forward to CLI input stream */
    for( int i=0; i<n_read; i++ )
    {
        byte = UART1.fifo.rw_byte;
        FreeRTOS_CLIInStreamWriteFromISR( &byte, 1u, &xShouldYield );
    }

    uart_clear_intr_status( UART_CLI, UART_RXFIFO_FULL_INT_CLR | UART_RXFIFO_TOUT_INT_CLR );

    if( xShouldYield )
    {
        taskYIELD();
    }
}
#endif


static uint8_t xTmpBuffer[BUF_SIZE];
static void prvUartEventHandlerTask( void *pvParameters )
{
    uart_event_t xEvent = { 0 };
    size_t ulNWritten = 0;
    size_t ulNRead = 0;

    uint32_t ulInStreamPermit = FreeRTOS_CLIInStreamClaim( STREAM_WRITE, pdMS_TO_TICKS( 1000 ) );
    uint32_t ulOutStreamPermit = FreeRTOS_CLIOutStreamClaim( STREAM_READ, pdMS_TO_TICKS( 1000 ) );
    configASSERT( ulInStreamPermit & STREAM_WRITE );    
    configASSERT( ulOutStreamPermit & STREAM_READ );

    while(1)
    {
        /* Forward bytes to CLI instream */
        if( xQueueReceive( xUartEvents, (void *)&xEvent, portMAX_DELAY ) )
        {
            memset( xTmpBuffer, 0u, sizeof(xTmpBuffer) );
            
            switch( xEvent.type )
            {
                case UART_DATA:
                    uart_read_bytes( UART_CLI, xTmpBuffer, xEvent.size, portMAX_DELAY );
                    ulNWritten = FreeRTOS_CLIInStreamWrite( xTmpBuffer, xEvent.size, pdMS_TO_TICKS( 1000 ) );
                    ESP_LOGI( TAG, "UART --> Forwarded %d/%d B --> CLI In", ulNWritten, xEvent.size );
                    break;
                case UART_FIFO_OVF:
                case UART_BUFFER_FULL:
                    uart_flush_input( UART_CLI );
                    xQueueReset( xUartEvents );
                    break;
                default:
                    break;
            }
        }

        /* Forward CLI out stream bytes to UART */
        ulNRead = FreeRTOS_CLIOutStreamRead( xTmpBuffer, sizeof(xTmpBuffer), pdMS_TO_TICKS( 1000 ) );
        ulNWritten = uart_write_bytes( UART_CLI, (void *)xTmpBuffer, ulNRead );
        ESP_LOGI( TAG, "CLI Out --> Forwarded %d/%d B --> UART", ulNWritten, ulNRead );
    }

}

void app_main(void)
{
    /* Setup CLI */
    uart_config_t xUartConfig = { 0 };
    xUartConfig.baud_rate = 115200;
    xUartConfig.data_bits = UART_DATA_8_BITS;
    xUartConfig.parity = UART_PARITY_DISABLE;
    xUartConfig.stop_bits = UART_STOP_BITS_1;
    xUartConfig.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    xUartConfig.source_clk = UART_SCLK_APB;

    ESP_ERROR_CHECK( uart_param_config( UART_CLI, &xUartConfig ) );
    ESP_ERROR_CHECK( uart_driver_install( UART_CLI, BUF_SIZE * 2, BUF_SIZE * 2, 20, &xUartEvents, 0 ) );
    ESP_ERROR_CHECK( uart_set_pin( UART_CLI, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE ) );

    configASSERT( FreeRTOS_CLICreateTask() );

    xTaskCreate( prvUartEventHandlerTask, "UART-EVENTS", configMINIMAL_STACK_SIZE + 2048, NULL, 12, NULL );
}