#ifndef __COMPUTER_H__
#define __COMPUTER_H__

#include <stdint.h>

/*
    LC-X 的基本架构：
        1. 16 位的 CPU 指令大小
        2. 65536 字节的内存大小
    参见《计算机系统概论》4.2
 */

#define WORD uint16_t  // 字的大小
#define MEMSIZE 65536  // 内存大小

extern WORD RUN;  // RUN 状态锁存器，表示目前 LC-X 的运行状态

// 内存
// 参见《计算机系统概论》4.1.1
extern WORD mem[MEMSIZE];  // LC-X 的内存空间
extern WORD MAR;           // 内存地址寄存器
extern WORD MDR;           // 内存数据寄存器

// 处理单元
extern WORD R[8];  // 通用寄存器
extern WORD PSR;   // 处理器状态寄存器

// 控制单元
extern WORD IR;  // 指令寄存器，内容为正在被执行的指令
extern WORD PC;  // 程序计数器（指令指针），内容为下一条指令的地址

// LC-X 启动入口
void run();

#endif