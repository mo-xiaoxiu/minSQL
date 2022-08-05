#ifndef DBSTRUCT_H_
#define DBSTRUCT_H_

#include <stdint.h> //uint_32_t
#include "resultType.h"
#include <stdio.h>




// 3
#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255
#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute) //计算某个字段的大小
//表示一行数据的结构体
typedef struct {
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE + 1]; //“\0”
    char email[COLUMN_EMAIL_SIZE + 1];
} Row;

//各个字段的大小
const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
//各个字段的偏移量
#define ID_OFFSET  0
#define USERNAME_OFFSET  (ID_OFFSET + ID_SIZE)
#define EMAIL_OFFSET (USERNAME_OFFSET + USERNAME_SIZE)
#define ROW_SIZE (ID_SIZE + USERNAME_SIZE + EMAIL_SIZE)
//一页大小
const uint32_t PAGE_SIZE = 4096; //4KB
//一张表最多又多少页： 100
#define TABLE_MAX_PAGES 100
//一页有多少行
#define ROWS_PER_SIZE (PAGE_SIZE / ROW_SIZE)
//一张表最多有多少行
#define TABLE_MAX_ROWS (ROWS_PER_SIZE * TABLE_MAX_PAGES)





//寻呼机
typedef struct {
    int file_descriptor; //文件描述符
    uint32_t file_length;
    void* page[TABLE_MAX_PAGES];
} Pager;

//结构体：表
typedef struct {
    uint32_t num_rows;
    //void* pages[TABLE_MAX_PAGES];
    Pager* pager; //表中通过寻呼机定位
} Table;

//光标
typedef struct {
    Table* table; //指向一个表
    uint32_t row_nums; //推进到表中的哪一行
    bool end_of_table; //是否到达表的结尾
} Cursor;

//结构体
//输入缓冲区
typedef struct InputBuffer{
    char* buffer;
    size_t buffer_length;
    ssize_t input_length;
} InputBuffer;
//语句
typedef struct Statement
{
    StatementType type;
    Row row_to_insert; //only insert
} Statement;


#endif