#pragma once


#include "gd32vf103.h"
#include "gd32vf103_usart.h"

#define CLTOOL_BUFFER_SIZE 128

typedef enum{
    CLTOOL_USE_DMA,
    CLTOOL_USE_BLOCKING
} cltool_mode_t;

typedef int(*cltool_cmd_function_t)(int argc, char * argv[]);
typedef int(*cltool_print_function_t)(const char *string);

typedef struct{
    char const *name;
    cltool_cmd_function_t run;
    char const *help;
} cltool_command_vector_t;


typedef struct{
    cltool_command_vector_t *p_com_vector;
    cltool_print_function_t print_handler;
    char terminator;
}cltool_config_t;


void cltool_init(cltool_config_t config);
void cltool_receive(char c);
void cltool_receiveln(const char *line);
void cltool_run_scheduled();



