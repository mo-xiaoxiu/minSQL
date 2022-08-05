# minSQL
`学习cstakc的db_tutorial书写的小型数据库代码，学习使用`

* 文档链接：

  [cstack-db_tutorial](https://cstack.github.io/db_tutorial/)

* 源仓库地址：

  [db_tutorial](https://github.com/cstack/db_tutorial)

## 介绍

前端：

* 分析器
* 解析器
* 代码生成器

*前端输入：SQL语句查询；输出：虚拟机字节码*

后端：

* **虚拟机**：将前端生成的字节码作为指令。然后它可以对一个或多个表或索引执行操作，每个表或索引都存储在称为 B 树的数据结构中
* **B树**：每个节点的长度为一页。B-tree 可以通过向寻呼机发出命令从磁盘检索页面或将其保存回磁盘
* **寻呼机**：接收读取或写入数据页的命令。它负责在数据库文件中的适当偏移处读取/写入。它还将最近访问的页面缓存在内存中，并确定何时需要将这些页面写回磁盘
* **OS接口**：根据 sqlite 编译的操作系统而有所不同的层（*不支持跨平台*）



## 关键



* `main()`

  ```c
  int main(int argc, char** argv) {
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
  
          if(input_buffer->buffer[0] == '.') { //元命令
              switch (do_meta_command(input_buffer, table)) //解析元命令
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
  
          switch (execute_statement(&statement, table)) //执行语句
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
  ```

* 函数：

  * 打开数据库文件，接收用户输入

  ```c
  //打开数据库文件：包含创建表
  Table* db_open(const char* filename) {
      Pager* pager = pager_open(filename); //初始化寻呼机
      uint32_t num_rows = pager->file_length / ROW_SIZE; //寻呼机中文件有多少行
      Table* table = (Table*)malloc(sizeof(Table)); //创建表
      table->pager = pager;
      table->num_rows = num_rows;
  
      return table;
  }
  
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
  ```

  * 创建输入缓冲区

  ```c
  //创建输入缓冲区
  InputBuffer* new_input_buffer() {
      InputBuffer* input_buffer = (InputBuffer*)malloc(sizeof(InputBuffer));
      input_buffer->buffer = NULL;
      input_buffer->buffer_length = 0;
      input_buffer->input_length = 0;
  
      return input_buffer;
  }
  ```

  * 解析元命令

  ```c
  //解析元命令
  MetaCommandResult do_meta_command(InputBuffer* input_buffer, Table* table) {
      if(strcmp(input_buffer->buffer, ".exit") == 0) { //退出命令
          close_input_buffer(input_buffer); //关闭缓冲区
          db_close(table); //关闭数据库文件
          exit(EXIT_SUCCESS);
      }else{
          return META_COMMAND_UNRECONIZED_COMMAND;
      }
  }
  
  //关闭释放输入缓冲区
  void close_input_buffer(InputBuffer* input_buffer) {
      free(input_buffer->buffer);
      free(input_buffer);
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
          pager_flush(pager, i, PAGE_SIZE); //清除页面的工作交给寻呼机
          free(pager->page[i]);
          pager->page[i] = NULL;
      }
  
      //清理多余的行
      uint32_t num_additional_rows = table->num_rows % ROWS_PER_SIZE;
      if(num_additional_rows > 0) {
          uint32_t page_num = num_full_pages;
          if(pager->page[page_num] != NULL) {
              pager_flush(pager, page_num, num_additional_rows); //剩余的行数据也给清理掉
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
      } //文件位置
  
      //写回文件
      ssize_t byte_written = write(pager->file_descriptor, pager->page[page_num], size);
      if(byte_written == -1) {
          printf("Error writing: %d\n", errno);
          exit(EXIT_FAILURE);
      }
  }
  ```

  * 解析准备语句

  ```c
  //解析准备语句
  PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement) {
      if(strncmp(input_buffer->buffer, "insert", 6) == 0) {
          return prepare_insert(input_buffer, statement);
      }
      if(strncmp(input_buffer->buffer, "select", 6) == 0) {
          statement->type = STATEMENT_SELECT;
          return PREPARE_SUCCESS;
      }
  
      return PREPARE_UNRECONIZED_STATEMENT;
  }
  
  //解析插入语句
  PrepareResult prepare_insert(InputBuffer* input_buffer, Statement* statement) {
      statement->type = STATEMENT_INSERT; //赋值语句类型为插入
      
      //解析行数据各个字段
      char* key_word = strtok(input_buffer->buffer, " ");
      char* id_string = strtok(NULL, " ");
      char* username = strtok(NULL, " ");
      char* email = strtok(NULL, " ");
  
      if(id_string == NULL || username == NULL || email == NULL) {
          return PREPARE_SYNTAX_ERROR; //解析错误
      }
  
      int id = atoi(id_string);
      if(id < 0) {
          return PREPARE_NEGTIVE_ID; //id必须为正
      }
      //字符串过长的情况
      if(strlen(username) > COLUMN_USERNAME_SIZE) {
          return PREPARE_STRING_TO_LONG;
      }
      if(strlen(email) > COLUMN_EMAIL_SIZE) {
          return PREPARE_STRING_TO_LONG;
      }
  
      //插入专有
      statement->row_to_insert.id = id;
      strcpy(statement->row_to_insert.username, username);
      strcpy(statement->row_to_insert.email, email);
  
      return PREPARE_SUCCESS;
  }
  ```

  * 执行语句

  ```c
  //执行语句
  ExecuteResult execute_statement(Statement* statement, Table* table) {
      switch (statement->type)
      {
      case (STATEMENT_INSERT): //插入
          //printf("This is where we could do an insert.\n");
          return execute_insert(statement, table);
          break;
      case (STATEMENT_SELECT): //查询
          //printf("This is where we could do an select.\n");
          return execute_select(statement, table);
          break;
      }
  }
  
  //执行插入操作
  ExecuteResult execute_insert(Statement* statement, Table* table) {
      if(table->num_rows >= TABLE_MAX_ROWS) { //判断表中的行数是否超过限定的最大行数
          return EXECUTE_TABLE_FULL;
      }
  
      Row* row_to_insert = &(statement->row_to_insert); //插入的那行数据
      Cursor* cursor = table_end(table); //定位到最后一行
      serialize_row(row_to_insert, cursor_value(cursor)); //序列化行数据：最后一行插入(通过光标定位)
      table->num_rows += 1; //表中行数 + 1
  
      return EXECUTE_SUCCESS;
  }
  
  //定位到最后一行：创建指向表末尾的光标
  void* table_end(Table* table) {
      Cursor* cursor = malloc(sizeof(Cursor));
      cursor->table = table;
      cursor->row_nums = table->num_rows;
      cursor->end_of_table = true;
  
      return cursor;
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
  
  //最后一行插入：使用光标定位
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
  
  //执行查询语句
  ExecuteResult execute_select(Statement* statement, Table* table) {
      Cursor* cursor = table_start(table); //创建光标指向表的开头
      Row row;
      
      //利用光标推进读取数据：遍历表
      while(!(cursor->end_of_table)) {
          deserialize_row(cursor_value(cursor), &row);
          print_row(&row);
          cursor_advance(cursor);
      }
  
      free(cursor);
  
      return EXECUTE_SUCCESS;
  }
  
  //光标前进/光标推进
  void cursor_advance(Cursor* cursor) {
      cursor->row_nums += 1;
      if(cursor->row_nums >= cursor->table->num_rows) { //光标定位是否超出表末尾
          cursor->end_of_table = true;
      }
  }
  
  //反序列化行数据
  void deserialize_row(void* source, Row* destination) {
      memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
      memcpy(&(destination->username), source + USERNAME_OFFSET, USERNAME_SIZE);
      memcpy(&(destination->email), source + EMAIL_OFFSET, EMAIL_SIZE);
  }
  ```

* 细节

  * `getline`：`	ssize_t getline(char **lineptr, size_t *n, FILE *stream)`
    * 包含`#define _GNU_SOURCE`
  * 使用宏定义常量：如一张表的最大页数100，每张表中有多少行等
  * 结构体：

  ```c
  //表示一行数据
  typedef struct {
      uint32_t id;
      char username[COLUMN_USERNAME_SIZE + 1]; //“\0”
      char email[COLUMN_EMAIL_SIZE + 1];
  } Row;
  
  //寻呼机
  typedef struct {
      int file_descriptor; //文件描述符
      uint32_t file_length;
      void* page[TABLE_MAX_PAGES];
  } Pager;
  
  //表
  typedef struct {
      uint32_t num_rows;
      Pager* pager; //表中通过寻呼机定位
  } Table;
  
  //光标
  typedef struct {
      Table* table; //指向一个表
      uint32_t row_nums; //推进到表中的哪一行
      bool end_of_table; //是否到达表的结尾
  } Cursor;
  
  //输入缓冲区
  typedef struct InputBuffer{
      char* buffer;
      size_t buffer_length;
      ssize_t input_length;
  } InputBuffer;
  
  //语句
  typedef struct Statement
  {
      StatementType type; //语句类型
      Row row_to_insert; //only insert
  } Statement;
  
  ```

  * 权举：表示状态

  ```c
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
  
  //执行结果状态
  typedef enum {
      EXECUTE_SUCCESS,
      EXECUTE_TABLE_FULL
  } ExecuteResult;
  ```

  ---

  



