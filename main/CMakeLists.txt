set( FREERTOS_CLI_DIR ../lib/FreeRTOS-Plus-CLI)

idf_component_register( SRCS
                            main.c
                            ${FREERTOS_CLI_DIR}/FreeRTOS_CLI.c
                        INCLUDE_DIRS
                            ${FREERTOS_CLI_DIR}
                            inc
)
