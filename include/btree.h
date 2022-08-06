#ifndef BTREE_H_
#define BTREE_H_
#include <stdint.h>
#include <stdio.h>
#include "dbstruct.h"
#include "func.h"



void *get_page(Pager *pager, uint32_t page_num);
void serialize_row(Row *source, void *destination);



//访问叶节点的字段
//叶节点的单元格数量
uint32_t* leaf_node_num_cells(void* node) {
    return node + LEAF_NODE_NUM_CELLS_OFFSET;
}

//叶节点单元格
void* leaf_node_cell(void* node, uint32_t cell_num) {
    return node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE;
}

//叶节点键
uint32_t* leaf_node_key(void* node, uint32_t cell_num) {
    return leaf_node_cell(node, cell_num);
}

//叶节点值
void* leaf_node_value(void* node, uint32_t cell_num) {
    return leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE;
}

//初始化叶节点
void initiallze_leaf_node(void* node) { *leaf_node_num_cells(node); }


//用于将键值对插入叶节点：以光标作为输入
void leaf_node_insert(Cursor* cursor, uint32_t key, Row* row) {
    void* node = get_page(cursor->table->pager, cursor->page_num); //此光标对应的页

    //根据此页面得到它的单元个数
    uint32_t num_cells = *leaf_node_num_cells(node);

    //node full
    if(num_cells >= LEAF_NODE_MAX_CELLS) {
        printf("Need to implement splitting a leaf node.\n");
        exit(EXIT_FAILURE);
    }

    //光标位置在所有单元格之间
    if(cursor->cell_num < num_cells) {
        for(uint32_t i = num_cells; i > cursor->cell_num; i--) {
            //创建空间
            memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i - 1), LEAF_NODE_CELL_SIZE);
        }
    }

    //重置叶节点单元个数和key值，序列化数据(将行数据序列化存储到叶节点键值对中的值)
    *(leaf_node_num_cells(node)) += 1;
    *(leaf_node_key(node, cursor->cell_num)) = key;
    serialize_row(row, leaf_node_value(node, cursor->cell_num));
}

//树的可视化
void print_leaf_node(void* node) {
    uint32_t num_cells = *leaf_node_num_cells(node);
    printf("leaf (size %d)\n", num_cells);
    for(uint32_t i = 0; i < num_cells; i++) {
        uint32_t key = *leaf_node_key(node, i);
        printf("  - %d : %d\n", i, key);
    }
}

#endif