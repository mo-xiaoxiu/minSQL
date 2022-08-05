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


//节点头部格式
const uint32_t NODE_TYPE_SIZE = sizeof(uint8_t); //节点类型
const uint32_t NODE_TYPE_OFFSET = 0; //节点类型偏移量
const uint32_t IS_ROOT_SIZE = sizeof(uint8_t); //是否为根节点
const uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
const uint32_t PARENT_POINTER_SIZE = sizeof(uint32_t); //父节点指针大小
const uint32_t PARENT_POINTER_OFFSET = IS_ROOT_SIZE + IS_ROOT_OFFSET; //父节点指针偏移量 = 根节点大小 + 根节点偏移量
const uint32_t COMMON_NODE_HEADER_SIZE = 
    NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;  //节点头部大小 = 节点类型 + 根节点大小 + 父节点指针大小


//叶子节点格式
const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t); //叶节点单元格数量大小
const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE; //叶节点单元个数偏移量 = 节点头部大小
const uint32_t LEAF_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_OFFSET;

//叶节点主体：单元数组，每个单元格是一个键， 后接一个值（序列化行数据）
const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_KEY_OFFSET = 0;
const uint32_t LEAF_NODE_VALUE_SIZE = ROW_SIZE; //叶节点值大小 = 行大小
const uint32_t LEAF_NODE_VALUE_OFFSET = LEAF_NODE_KEY_SIZE + LEAF_NODE_KEY_OFFSET;
const uint32_t LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE; //单元格大小 = 键大小 + 值大小
const uint32_t LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE; //叶节点分配给单元格的空间 = 页大小 - 叶节点头部大小



#endif