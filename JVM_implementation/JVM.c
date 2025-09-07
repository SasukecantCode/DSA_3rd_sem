#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define STACK_MAX 50
#define LOCALS_MAX 50
#define MAX_INSTRUCTIONS 256
#define LINE_MAX_LEN 256

typedef enum {
    OP_LDC, OP_ILOAD, OP_ISTORE,
    OP_IADD, OP_ISUB, OP_IMUL, OP_IDIV,
    OP_IFEQ, OP_IFLT, OP_IFGT,
    OP_READ, OP_PRINT,
    OP_HALT
} OpCode;

typedef struct {
    OpCode op;
    int arg;
} Instruction;

Instruction program[MAX_INSTRUCTIONS];
int program_length = 0;

int stack[STACK_MAX];
int sp = -1;
int locals[LOCALS_MAX];

void push(int value) {
    if (sp >= STACK_MAX - 1) {
        printf("Stack overflow\n");
        exit(1);
    }
    stack[++sp] = value;
}

int pop() {
    if (sp < 0) {
        printf("Stack underflow\n");
        exit(1);
    }
    return stack[sp--];
}

void execute() {
    int pc = 0;
    while (pc < program_length) {
        Instruction instr = program[pc];

        switch (instr.op) {
            case OP_LDC:
                push(instr.arg);
                pc++;
                break;
            case OP_ILOAD:
                if (instr.arg < 0 || instr.arg >= LOCALS_MAX) {
                    printf("Invalid local index\n");
                    exit(1);
                }
                push(locals[instr.arg]);
                pc++;
                break;
            case OP_ISTORE:
                if (instr.arg < 0 || instr.arg >= LOCALS_MAX) {
                    printf("Invalid local index\n");
                    exit(1);
                }
                locals[instr.arg] = pop();
                pc++;
                break;
            case OP_IADD: {
                int b = pop();
                int a = pop();
                push(a + b);
                pc++;
                break;
            }
            case OP_ISUB: {
                int b = pop();
                int a = pop();
                push(a - b);
                pc++;
                break;
            }
            case OP_IMUL: {
                int b = pop();
                int a = pop();
                push(a * b);
                pc++;
                break;
            }
            case OP_IDIV: {
                int b = pop();
                if (b == 0) {
                    printf("Division by zero\n");
                    exit(1);
                }
                int a = pop();
                push(a / b);
                pc++;
                break;
            }
            case OP_IFEQ: {
                int v = stack[sp]; // peek, don't pop
                pc += (v == 0) ? instr.arg : 1;
                break;
            }
            case OP_IFLT: {
                int v = stack[sp]; // peek, don't pop
                pc += (v < 0) ? instr.arg : 1;
                break;
            }
            case OP_IFGT: {
                int v = stack[sp]; // peek, don't pop
                pc += (v > 0) ? instr.arg : 1;
                break;
            }
            case OP_READ: {
                int x;
                scanf("%d", &x);
                push(x);
                pc++;
                break;
            }
            case OP_PRINT:
                if (sp >= 0)
                    printf("%d\n", stack[sp]);
                else
                    printf("Stack empty\n");
                pc++;
                break;
            case OP_HALT:
                return;
            default:
                printf("Unknown opcode\n");
                exit(1);
        }
    }
}

OpCode get_opcode(const char *token) {
    if (strcmp(token, "ldc") == 0) return OP_LDC;
    if (strcmp(token, "iload") == 0) return OP_ILOAD;
    if (strcmp(token, "istore") == 0) return OP_ISTORE;
    if (strcmp(token, "iadd") == 0) return OP_IADD;
    if (strcmp(token, "isub") == 0) return OP_ISUB;
    if (strcmp(token, "imul") == 0) return OP_IMUL;
    if (strcmp(token, "idiv") == 0) return OP_IDIV;
    if (strcmp(token, "ifeq") == 0) return OP_IFEQ;
    if (strcmp(token, "iflt") == 0) return OP_IFLT;
    if (strcmp(token, "ifgt") == 0) return OP_IFGT;
    if (strcmp(token, "read") == 0) return OP_READ;
    if (strcmp(token, "print") == 0) return OP_PRINT;
    if (strcmp(token, "halt") == 0) return OP_HALT;
    return -1;
}

void load_program(FILE *input) {
    char line[LINE_MAX_LEN];
    while (fgets(line, sizeof(line), input) && program_length < MAX_INSTRUCTIONS) {
        char opcode_str[10];
        int operand;
        int count = sscanf(line, "%s %d", opcode_str, &operand);
        OpCode op = get_opcode(opcode_str);
        if (op == -1) {
            printf("Unknown instruction: %s\n", opcode_str);
            exit(1);
        }
        program[program_length].op = op;
        program[program_length].arg = (count == 2) ? operand : 0;
        program_length++;
    }
}

int main(int argc, char *argv[]) {
    FILE *input = stdin;
    if (argc > 1) {
        input = fopen(argv[1], "r");
        if (!input) {
            printf("Error: Cannot open file %s\n", argv[1]);
            return 1;
        }
    }

    load_program(input);
    if (input != stdin)
        fclose(input);

    execute();
    return 0;
}