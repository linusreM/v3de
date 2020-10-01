#include "cltool.h"
#include "gd32vf103.h"
#include "stdlib.h"
#include "string.h"
#define NULL_PTR (void*)0
#define CLTOOL_MAX_ARG 16

static char buffer[CLTOOL_BUFFER_SIZE] = {0};
static char command[CLTOOL_BUFFER_SIZE] = {0};


static int buf_index = 0;

static cltool_config_t conf;

void cltool_init(cltool_config_t config){
    conf = config; 
}

void cltool_receive(char c)
{
    if(c == '\b')
    {
        if(buf_index > 0) buf_index--;
        return;
    }
    if(buf_index >= CLTOOL_BUFFER_SIZE) return;
    if(c != conf.terminator && buf_index < CLTOOL_BUFFER_SIZE - 1)
    {
        buffer[buf_index++] = c;
    }
    else
    {
        buffer[buf_index] = '\0';
        strncpy(command, buffer, CLTOOL_BUFFER_SIZE);
        buf_index = 0;
    }
    
}

void cltool_receiveln(const char *line)
{
    for(int i = 0; buf_index < CLTOOL_BUFFER_SIZE && line[i] != '\0'; i++) cltool_receive(line[i]);
}

void cltool_run_scheduled()
{
    if(command[0] == 0) return;
    cltool_command_vector_t *cmd = conf.p_com_vector;
    char *argv[CLTOOL_MAX_ARG];
    int argc = 1;


    for(int i = 0; command[i] != '\0' && i < CLTOOL_BUFFER_SIZE; i++)
    {
        if(command[i] == ' ' && argc < CLTOOL_MAX_ARG)
        {
            argv[argc] = &command[i + 1];
            argc++;
            command[i] = '\0';
        }
    }

    while(cmd->name[0] != 0)
    {
        if(!strncmp(cmd->name, command, CLTOOL_BUFFER_SIZE))
        {
            argv[0] = command;
            cmd->run(argc, argv);
            command[0] = '\0';
            return;
        }
        cmd++;
    }
    command[0] = '\0';
}
