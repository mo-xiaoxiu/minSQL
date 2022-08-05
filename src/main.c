#define _GNU_SOURCE //getline: ssize_t getline(char **lineptr, size_t *n, FILE *stream);
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h> //open
#include <errno.h> //errno
#include "resultType.h"
#include "dbstruct.h"
#include "func.h"






int main(int argc, char** argv) {
    //首先，新建一个表
    //Table* table = new_table();

    if(argc < 2) {
        printf("Must supply a database filename.\n");
        exit(EXIT_FAILURE);
    }

    //打开数据库文件：依据用户输入的文件名
    char* filename = argv[1];
    Table* table = db_open(filename);

    //创建一个缓冲区
    InputBuffer* input_buffer = new_input_buffer();

    while (true)
    {
        print_beginLine();
        //读取用户输入
        read_input(input_buffer);

        // //1. 执行退出  .exit
        // if(strcmp(input_buffer->buffer, ".exit") == 0) {
        //     close_input_buffer(input_buffer); //关闭释放输入缓冲区
        //     exit(EXIT_SUCCESS);
        // }else{
        //     printf("Unrecognized command '%s'.\n", input_buffer->buffer);
        // }



        if(input_buffer->buffer[0] == '.') { //元命令
            switch (do_meta_command(input_buffer, table))
            {
            case (META_COMMAND_SUCCESS):
                continue;
                break;
            case (META_COMMAND_UNRECONIZED_COMMAND):
                printf("Unreconized command '%s'.\n", input_buffer->buffer);
                continue;
            }
        }
        Statement statement;
        switch (prepare_statement(input_buffer, &statement)) //准备语句
        {
        case (PREPARE_SUCCESS):
            break;
        case (PREPARE_UNRECONIZED_STATEMENT):
            printf("Unreconized keyword at start of '%s'.\n", input_buffer->buffer);
            continue;
        case (PREPARE_SYNTAX_ERROR):
            printf("Unrecognized keyword at start of '%s'.\n", input_buffer->buffer);
            continue;
        case (PREPARE_NEGTIVE_ID):
            printf("ID must be positive.\n");
            continue;
        case (PREPARE_STRING_TO_LONG):
            printf("String is to long.\n");
            continue;
        }
        //execute_statement(&statement); //执行语句
        //printf("Executed.\n");


        switch (execute_statement(&statement, table))
        {
        case (EXECUTE_SUCCESS):
            printf("Executed.\n");
            break;
        case (EXECUTE_TABLE_FULL):
            printf("Error: Table full.\n");
            break;
        }
    }
    
    return 0;
}
