#ifndef ENUM_H_
#define ENUM_H_

//权举
//解析元命令
typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECONIZED_COMMAND
} MetaCommandResult;
//解析准备语句
typedef enum {
    PREPARE_SUCCESS,
    PREPARE_UNRECONIZED_STATEMENT,
    PREPARE_SYNTAX_ERROR, //语法错误
    PREPARE_STRING_TO_LONG, //（插入）字段字符串太长
    PREPARE_NEGTIVE_ID //用户输入错误ID：负数
} PrepareResult;
//语句类型
typedef enum {
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;
typedef enum {
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL
} ExecuteResult;


#endif