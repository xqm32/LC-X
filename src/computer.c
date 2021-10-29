#include "computer.h"
#include <stdio.h>

// 符号扩展
#define SEXT(Data, Highest) \
    (Data & 1 << (Highest - 1) ? Data | (0x10000 - (1 << (Highest - 1))) : Data)
// 零扩展
#define ZEXT(Data) (Data)
// 取位
#define BIT(bit) (IR & (1 << bit))

// 指令执行
#define INS ins[(IR & 0xF000) >> 12]

// 指令注释
// 参见《计算机系统概论》A.3 表 A-1

// SR、SR1、SR2 均为源寄存器
#define SR_N (IR & 0x0E00) >> 9
#define SR1_N (IR & 0x01C0) >> 6
#define SR2_N (IR & 0x0007)
#define DR_N (IR & 0x0E00) >> 9     // 目标寄存器
#define BaseR_N (IR & 0x01C0) >> 6  // 基址寄存器

#define SR R[SR_N]
#define SR1 R[SR1_N]
#define SR2 R[SR2_N]
#define DR R[DR_N]
#define BaseR R[BaseR_N]

#define imm5 IR & 0x001F  // 5 bit 立即数
#define offset6 IR & 0x003F  // 6 bit 有符号数值，用于与基址 BaseR 相加寻址
#define PCoffset9 IR & 0x01FF  // 9 bit 有符号数值，用于与程序计数器 PC 相加寻址
#define PCoffset11 \
    IR & 0x07FF  // 11 bit 有符号数值，用于与程序计数器 PC 相加寻址
#define trapvect8 IR & 0x00FF  // 8 bit 无符号数值，用于寻址 trap 服务

#define BaseR_offset6 (WORD)(BaseR + SEXT(offset6, 6))
#define PC_PCoffset9 (WORD)(PC + SEXT(PCoffset9, 9))
#define PC_PCoffset11 (WORD)(PC + SEXT(PCoffset11, 11))

// 指令中的 n、z、p 位
#define n IR & 0x0800
#define z IR & 0x0400
#define p IR & 0x0200

#define N PSR & 0x0004
#define Z PSR & 0x0002
#define P PSR & 0x0001

#define CC PSR & 0x0007
#define PL PSR & 0x0700
#define PM PSR & 0x8000

WORD RUN;
WORD mem[MEMSIZE], MAR, MDR;
WORD R[8], PSR;
WORD IR, PC;

// 指令集
void BR();
void ADD();
void LD();
void ST();
void JSR();
void AND();
void LDR();
void STR();
void RTI();
void NOT();
void LDI();
void STI();
void JMP();
void reserved();
void LEA();
void TRAP();

void (*ins[16])() = {BR,  ADD, LD,  ST,  JSR, AND,      LDR, STR,
                     RTI, NOT, LDI, STI, JMP, reserved, LEA, TRAP};

// 基于写入 DR 的值设置条件码 N、Z、P
void setcc() {
    PSR &= 0xFFF8;
    PSR |= DR ? DR & 0x8000 ? 0x0004 : 0x0001 : 0x0002;
}

// 二进制：0000 n z p PCoffset9
void BR() {
    printf("BR%c%c%c\n", n ? 'n' : ' ', z ? 'z' : ' ', p ? 'p' : ' ');
    if ((n && N) || (z && Z) || (p && P))
        PC = PC_PCoffset9;
}
// 二进制：0001 DR SR1 000 SR2
// 二进制：0001 DR SR1 1 imm5
void ADD() {
    printf("ADD\n");
    if (BIT(5) == 0)
        DR = SR1 + SR2;
    else
        DR = SR1 + SEXT(imm5, 5);
    setcc();
}
// 二进制：0010 DR PCoffset9
void LD() {
    printf("LD\tR[%d]:\t%04X\tmem[%04X]:\t%04X\n", DR_N, DR, PC_PCoffset9,
           mem[PC_PCoffset9]);
    DR = mem[PC_PCoffset9];
    printf("\tR[%d]:\t%04X\tmem[%04X]:\t%04X\n", DR_N, DR, PC_PCoffset9,
           mem[PC_PCoffset9]);
    setcc();
}
// 二进制：0011 SR PCoffset9
void ST() {
    printf("ST\tR[%d]:\t%04X\tmem[%04X]:\t%04X\n", SR_N, SR, PC_PCoffset9,
           mem[PC_PCoffset9]);
    mem[PC_PCoffset9] = SR;
    printf("\tR[%d]:\t%04X\tmem[%04X]:\t%04X\n", SR_N, SR, PC_PCoffset9,
           mem[PC_PCoffset9]);
}
// 二进制：0100 1 PCoffset11
void JSR() {
    printf("JSR\n");
    R[7] = PC;
    if (BIT(11) == 0)
        PC = BaseR;
    else
        PC = PC_PCoffset11;
}
// 二进制：0101 DR SR1 000 SR2
// 二进制：0101 DR SR1 1 imm5
void AND() {
    printf("AND\n");
    if (BIT(5) == 0)
        DR = SR1 & SR2;
    else
        DR = SR1 & SEXT(imm5, 5);
    setcc();
}
// 二进制：0110 DR PCoffset9
void LDR() {
    printf("LDR\n");
    DR = mem[BaseR_offset6];
    setcc();
}
// 二进制：0111 SR BaseR offset6
void STR() {
    printf("STR\tR[%d]:\t%04X\tmem[%04X]:\t%04X\n", SR_N, SR, BaseR_offset6,
           mem[BaseR_offset6]);
    mem[BaseR_offset6] = SR;
    printf("\tR[%d]:\t%04X\tmem[%04X]:\t%04X\n", SR_N, SR, BaseR_offset6,
           mem[BaseR_offset6]);
}
// 二进制：1000 0000 0000 0000
void RTI() {
    printf("RTI\n");
    if (PM == 0) {
        PC = mem[R[6]];
        ++R[6];
        WORD TEMP = mem[R[6]];
        PSR = TEMP;
    }
}
// 二进制：1001 DR SR 111111
void NOT() {
    printf("NOT\n");
    DR = ~SR;
    setcc();
}
// 二进制：1010 DR PCoffset9
void LDI() {
    printf("LDI\n");
    DR = mem[mem[PC_PCoffset9]];
    setcc();
}
// 二进制：1011 SR PCoffset9
void STI() {
    printf("STI\n");
    mem[mem[PC_PCoffset9]] = SR;
}
// 二进制：1100 000 BaseR 000000
void JMP() {
    printf("JMP\tR[%d]:\t%04X\n", BaseR_N, BaseR);
    PC = BaseR;
}
// 二进制：1101 0000 0000 0000
void reserved() {
    return;
}
// 二进制：1110 DR PCoffset9
void LEA() {
    printf("LEA\n");
    DR = PC_PCoffset9;
    setcc();
}
// 二进制：1111 0000 trapvect8
void TRAP() {
    printf("TRAP\t");
    R[7] = PC;
    PC = mem[ZEXT(trapvect8)];
    switch (trapvect8) {
        case 0x20:  // GETC
            printf("GETC\n\t");
            R[0] = getchar();
            break;
        case 0x21:  // OUT
            printf("OUT\tmem[%04X]:\t%04X\n\t", R[0], mem[R[0]]);
            putchar((char)R[0]);
            printf("\n");
            break;
        case 0x22:  // PUTS
            printf("PUTS\tmem[%04X]:\t%04X\n\t", R[0], mem[R[0]]);
            for (WORD* i = mem + R[0]; *i != '\0'; ++i)
                putchar((char)*i);
            printf("\n");
            break;
        case 0x23:  // IN
            printf("IN\n\t");
            R[0] = getchar();
            break;
        case 0x24:  // PUTSP
            printf("PUTSP\tmem[%04X]:\t%04X\n\t", R[0], mem[R[0]]);
            puts((char*)(mem + R[0]));
            break;
        case 0x25:  // HALT
            printf("HALT\n");
            RUN = 0;
            break;
    }
    PC = R[7];
}

// 六个指令节拍
// 没有严格按照指令节拍来模拟，后续有待优化
void fetch()  // 取指令
{
    MAR = PC++;
    MDR = mem[MAR];
    IR = MDR;
}
void decode()  // 译码
{
    INS();
}
void evaluate_address()  // 地址计算
{
    return;
}
void fetch_operand()  // 取操作数
{
    return;
}
void execute()  // 执行
{
    return;
}
void store_result()  // 存结果
{
    return;
}

void print_info() {
    // 打印各寄存器内容
    printf("\tMAR:\t%04X\tMDR:\t%04X\tIR:\t%04X\tPC:\t%04X\tPSR:\t%04X\n\t",
           MAR, MDR, PSR, IR, PC);
    // 打印通用寄存器内容
    for (int i = 0; i < 7; ++i)
        printf("R[%d]:\t%04X\t", i, R[i]);
    printf("R[7]:\t%04X\n", R[8]);
}

void run() {
    RUN = 1;
    PC = *mem;  // 内存中第一个值为指令执行地址
    while (RUN) {
        // 完成一个指令周期
        // 参见《计算机系统概论》4.3.2
        fetch();
        decode();
        evaluate_address();
        fetch_operand();
        execute();
        store_result();
        // 完成周期后输出信息
        print_info();
    }
}