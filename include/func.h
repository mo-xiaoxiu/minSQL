#ifndef FUNC_H_
#define FUNC_H_
#include "resultType.h"
#include "dbstruct.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>


//打印一行的各个字段
void print_row(Row* row) {
    printf("(%d, %s, %s)\n", row->id, row->username, row->email);
}
//序列化行数据
void serialize_row(Row* source, void* destination) {
    memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
    memcpy(destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
    memcpy(destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
    //确保初始化每一个字节
    // strncpy(destination + USERNAME_OFFSET, source->username, USERNAME_SIZE);
    // strncpy(destination + EMAIL_OFFSET, source->email, EMAIL_SIZE);
}
//反序列化行数据
void deserialize_row(void* source, Row* destination) {
    memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), source + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy(&(destination->email), source + EMAIL_OFFSET, EMAIL_SIZE);
}
//获取页
void* get_page(Pager* pager, uint32_t page_num) {
    if(page_num > TABLE_MAX_PAGES) {
        printf("Tried to fetch page number out of bounds. %d > %d\n", page_num,
           TABLE_MAX_PAGES);
        exit(EXIT_FAILURE);
    }

    if(pager->page[page_num] == NULL) { //cache miss
        void* page = malloc(sizeof(page));
        uint32_t num_pages = pager->file_length / PAGE_SIZE; //一个文件完整的页

        if(pager->file_length % PAGE_SIZE) { //一个页多出来的，算一页
            num_pages += 1;
        }

        if(page_num <= num_pages) { //这一页在文件范围内
            lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET); //文件定位到位置
            ssize_t byte_read = read(pager->file_descriptor, page, PAGE_SIZE); //读取文件内容到page
            if(byte_read == -1) {
                printf("Error reading file: %d\n", errno);
                exit(EXIT_FAILURE);
            }
        }

        pager->page[page_num] = page;
    }

    return pager->page[page_num];
}
//内存中定位行
// void* row_slot(Table* table, uint32_t row_num) {
//     uint32_t page_num = row_num / ROWS_PER_SIZE;
//     // void* page = table->pages[page_num];
//     // if(page == NULL) {
//     //     //当访问页的时候才分配内存
//     //     page = table->pages[page_num] = malloc(PAGE_SIZE);
//     // }
//     void* page = get_page(table->pager, page_num); //获取页
//     uint32_t row_offset = row_num % ROWS_PER_SIZE; //下一页多出来多少行
//     uint32_t byte_offset = row_offset * ROW_SIZE; //上述多出来多少字节
//     return page + byte_offset; //通过指针找到
// }
// 将内存中定位行数据的row_slot改为  使用光标去定位
//创建指向表开头的光标
void* table_start(Table* table) {
    Cursor* cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->row_nums = 0;
    cursor->end_of_table = (table->num_rows == 0);

    return cursor;
}
//创建指向表末尾的光标
void* table_end(Table* table) {
    Cursor* cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->row_nums = table->num_rows;
    cursor->end_of_table = true;

    return cursor;
}
//使用光标定位
void* cursor_value(Cursor* cursor) {
    //行号
    uint32_t row_num = cursor->row_nums;
    //通过行号->页号
    uint32_t page_num = row_num / ROWS_PER_SIZE;
    //获取页
    void* page = get_page(cursor->table->pager, page_num);
    //多余行数据的偏移量
    uint32_t row_offset = row_num % ROWS_PER_SIZE;
    uint32_t byte_offset = row_offset * ROW_SIZE;
    return page + byte_offset;
}
//光标前进/光标推进
void cursor_advance(Cursor* cursor) {
    cursor->row_nums += 1;
    if(cursor->row_nums >= cursor->table->num_rows) { //光标定位是否超出表末尾
        cursor->end_of_table = true;
    }
}
//新建一个表
//更改为打开数据库文件：包含创建表
// Table* new_table() {
//     Table* table = (Table*)malloc(sizeof(Table));
//     table->num_rows = 0;
//     for(uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
//         table->pages[i] = NULL;
//     }
//     return table;
// }
//初始化寻呼机
Pager* pager_open(const char* filename) {
    //打开文件描述符
    int fd = open(filename, 
        O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
    if(fd == -1) {
        printf("Unable to open file.\n");
        exit(EXIT_FAILURE);
    }

    //初始化文件长度
    off_t file_length = lseek(fd, 0, SEEK_END);

    //创建pager并初始化
    Pager* pager = (Pager*)malloc(sizeof(Pager));
    pager->file_descriptor = fd;
    pager->file_length = file_length;
    
    for(uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        pager->page[i] = NULL;
    }

    return pager;
}
//新建一个表 --> 更改为打开数据库文件：包含创建表
Table* db_open(const char* filename) {
    Pager* pager = pager_open(filename); //初始化寻呼机
    uint32_t num_rows = pager->file_length / ROW_SIZE; //寻呼机中文件有多少行
    Table* table = (Table*)malloc(sizeof(Table)); //创建表
    table->pager = pager;
    table->num_rows = num_rows;

    return table;
}
//寻呼机清除页面内容
void pager_flush(Pager* pager, uint32_t page_num, uint32_t size) {
    if(pager->page[page_num] == NULL) {
        printf("Tried to flush null page\n");
        exit(EXIT_FAILURE);
    }

    off_t offset = lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
    if(offset == -1) {
        printf("Error seeking: %d\n", errno);
        exit(EXIT_FAILURE);
    } //定位文件位置

    //写回文件
    ssize_t byte_written = write(pager->file_descriptor, pager->page[page_num], size);
    if(byte_written == -1) {
        printf("Error writing: %d\n", errno);
        exit(EXIT_FAILURE);
    }
}
//关闭数据库文件
void db_close(Table* table) {
    Pager* pager = table->pager;
    uint32_t num_full_pages = table->num_rows / ROWS_PER_SIZE; //表中有多少的页

    //清理页面
    for(uint32_t i = 0; i < num_full_pages; i++) {
        if(pager->page[i] == NULL) {
            continue;
        }
        pager_flush(pager, i, PAGE_SIZE); //清除写回磁盘页面的工作交给寻呼机
        free(pager->page[i]);
        pager->page[i] = NULL;
    }

    //清理多余的行
    uint32_t num_additional_rows = table->num_rows % ROWS_PER_SIZE;
    if(num_additional_rows > 0) {
        uint32_t page_num = num_full_pages;
        if(pager->page[page_num] != NULL) {
            pager_flush(pager, page_num, num_additional_rows * ROW_SIZE); //剩余的行数据也刷新到磁盘并清理掉
            free(pager->page[page_num]);
            pager->page[page_num] = NULL;
        }
    }

    //关闭文件描述符
    int res = close(pager->file_descriptor);
    if(res == -1) {
        printf("Error closing db file.\n");
        exit(EXIT_FAILURE);
    }
    //最后释放所有的页面
    for(uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        void* page = pager->page[i];
        if(page) {
            free(page);
            pager->page[i] = NULL;
        }
    }
    free(pager); //最后释放寻呼机和表
    free(table);
}
//释放一个表
// void free_table(Table* table) {
//     for(int i = 0; table->pages[i]; i++) {
//         free(table->pages[i]);
//     }
//     free(table);
// }







// 1
//创建输入缓冲区
InputBuffer* new_input_buffer() {
    InputBuffer* input_buffer = (InputBuffer*)malloc(sizeof(InputBuffer));
    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0;
    input_buffer->input_length = 0;

    return input_buffer;
}
//打印开始行
void print_beginLine() { printf("db > "); }
//读取用户输入
void read_input(InputBuffer* input_buffer) {
    ssize_t read_bytes = 
        getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);
    if(read_bytes <= 0) {
        printf("Error reading input.\n");
        exit(EXIT_FAILURE);
    }

    input_buffer->input_length = read_bytes - 1;
    input_buffer->buffer[read_bytes - 1] = 0;
}
//关闭释放输入缓冲区
void close_input_buffer(InputBuffer* input_buffer) {
    free(input_buffer->buffer);
    free(input_buffer);
}




// 2
//解析元命令
MetaCommandResult do_meta_command(InputBuffer* input_buffer, Table* table) {
    if(strcmp(input_buffer->buffer, ".exit") == 0) {
        close_input_buffer(input_buffer);
        //free_table(table);
        db_close(table);
        exit(EXIT_SUCCESS);
    }else{
        return META_COMMAND_UNRECONIZED_COMMAND;
    }
}
//解析插入语句
PrepareResult prepare_insert(InputBuffer* input_buffer, Statement* statement) {
    statement->type = STATEMENT_INSERT;
    
    char* key_word = strtok(input_buffer->buffer, " ");
    char* id_string = strtok(NULL, " ");
    char* username = strtok(NULL, " ");
    char* email = strtok(NULL, " ");

    if(id_string == NULL || username == NULL || email == NULL) {
        return PREPARE_SYNTAX_ERROR;
    }

    int id = atoi(id_string);
    if(id < 0) {
        return PREPARE_NEGTIVE_ID;
    }
    if(strlen(username) > COLUMN_USERNAME_SIZE) {
        return PREPARE_STRING_TO_LONG;
    }
    if(strlen(email) > COLUMN_EMAIL_SIZE) {
        return PREPARE_STRING_TO_LONG;
    }

    statement->row_to_insert.id = id;
    strcpy(statement->row_to_insert.username, username);
    strcpy(statement->row_to_insert.email, email);

    return PREPARE_SUCCESS;
}
//解析准备语句
PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement) {
    if(strncmp(input_buffer->buffer, "insert", 6) == 0) {
        // statement->type = STATEMENT_INSERT;
        // int args_assigned = sscanf(
        //     input_buffer->buffer, "insert %d %s %s", &(statement->row_to_insert.id),
        //     statement->row_to_insert.username, statement->row_to_insert.email
        // );
        // if(args_assigned < 3) { //参数个数不到3，表示语法错误
        //     return PREPARE_SYNTAX_ERROR;
        // }
        // return PREPARE_SUCCESS;

        return prepare_insert(input_buffer, statement);
    }
    if(strncmp(input_buffer->buffer, "select", 6) == 0) {
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }

    return PREPARE_UNRECONIZED_STATEMENT;
}
//执行插入操作
ExecuteResult execute_insert(Statement* statement, Table* table) {
    if(table->num_rows >= TABLE_MAX_ROWS) {
        return EXECUTE_TABLE_FULL;
    }

    Row* row_to_insert = &(statement->row_to_insert); //找到插入的那行数据
    Cursor* cursor = table_end(table); //定位到最后一行
    serialize_row(row_to_insert, cursor_value(cursor)); //序列化行数据：最后一行插入(通过光标定位)
    table->num_rows += 1; //表中行数 + 1

    free(cursor);

    return EXECUTE_SUCCESS;
}
//执行查询语句
ExecuteResult execute_select(Statement* statement, Table* table) {
    Cursor* cursor = table_start(table);
    Row row;
    // for(uint32_t i = 0; i <table->num_rows; i++) {
    //     deserialize_row(row_slot(table, i), &row);
    //     print_row(&row);
    // }
    //利用光标推进读取数据
    while(!(cursor->end_of_table)) {
        deserialize_row(cursor_value(cursor), &row);
        print_row(&row);
        cursor_advance(cursor);
    }

    free(cursor);

    return EXECUTE_SUCCESS;
}
//执行语句
ExecuteResult execute_statement(Statement* statement, Table* table) {
    switch (statement->type)
    {
    case (STATEMENT_INSERT):
        //printf("This is where we could do an insert.\n");
        return execute_insert(statement, table);
        break;
    case (STATEMENT_SELECT):
        //printf("This is where we could do an select.\n");
        return execute_select(statement, table);
        break;
    }
}

#endif