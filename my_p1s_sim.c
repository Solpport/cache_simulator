/*
 * Project 1
 * EECS 370 LC-2K Instruction-level simulator
 *
 * Make sure to NOT modify printState or any of the associated functions
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// DO NOT CHANGE THE FOLLOWING DEFINITIONS

// Machine Definitions
#define MEMORYSIZE 65536 /* maximum number of words in memory (maximum number of lines in a given file)*/
#define NUMREGS 8        /* total number of machine registers [0,7] */

// File Definitions
#define MAXLINELENGTH 1000 /* MAXLINELENGTH is the max number of characters we read */

// Define stateType before declaring functions
typedef struct
{
    int pc;
    int mem[MEMORYSIZE];
    int reg[NUMREGS];
    int numMemory;
} stateType;

// Forward declarations of helper functions
// Forward declarations of helper functions
static int getOpcode(int instruction);
static int getRegA(int instruction);
static int getRegB(int instruction);
static int getDestReg(int instruction);
static int getOffset(int instruction);

// Function Prototype for executeInstruction
void executeInstruction(stateType *state, int instruction, bool *halt);
static inline int convertNum(int32_t num);
void printState(stateType *);

extern void cache_init(int blockSize, int numSets, int blocksPerSet);
extern int cache_access(int addr, int write_flag, int write_data);
extern void printStats();
static stateType state;
static int num_mem_accesses = 0;
int mem_access(int addr, int write_flag, int write_data)
{
    ++num_mem_accesses;
    if (write_flag)
    {
        state.mem[addr] = write_data;
        if (state.numMemory <= addr)
        {
            state.numMemory = addr + 1;
        }
    }
    return state.mem[addr];
}
int get_num_mem_accesses()
{
    return num_mem_accesses;
}

int main(int argc, char **argv)
{
    char line[MAXLINELENGTH];
    FILE *filePtr;

    // Initialize everything to 0
    state.pc = 0;
    state.numMemory = 0;
    for (int i = 0; i < NUMREGS; i++)
    {
        state.reg[i] = 0;
    }

    if (argc != 2)
    {
        printf("error: usage: %s <machine-code file>\n", argv[0]);
    }

    filePtr = fopen(argv[1], "r");
    if (filePtr == NULL)
    {
        printf("error: can't open file %s, please ensure you are providing the correct path\n", argv[1]);
        perror("fopen");
        exit(2);
    }

    /* read the entire machine-code file into memory */
    while (fgets(line, MAXLINELENGTH, filePtr) != NULL)
    {
        if (state.numMemory >= MEMORYSIZE)
        {
            fprintf(stderr, "Error: Exceeded memory size while loading machine code.\n");
            exit(2);
        }
        if (sscanf(line, "%x", &state.mem[state.numMemory]) != 1)
        {
            fprintf(stderr, "Error: Invalid machine code at address %d: %s", state.numMemory, line);
            exit(2);
        }
        printf("memory[%d]=0x%08X\n", state.numMemory, state.mem[state.numMemory]);
        state.numMemory++;
    }

    fclose(filePtr);

    bool halt = false;

    // Simulation loop
    while (!halt)
    {
        // Checks if the PC is in bound
        if (state.pc < 0 || state.pc >= state.numMemory)
        {
            fprintf(stderr, "Error: PC out of bounds (%d)\n", state.pc);
        }

        printState(&state);

        int instruction = state.mem[state.pc];

        executeInstruction(&state, instruction, &halt);
    }

    printState(&state);

    return 0;
}

// Executes a single instruction
void executeInstruction(stateType *state, int instruction, bool *halt)
{
    int opcode = getOpcode(instruction);
    int regA, regB, destReg, offset;

    // Implement a switch-case structure to handle each opcode
    switch (opcode)
    {
    case 0: // add
        regA = getRegA(instruction);
        regB = getRegB(instruction);
        destReg = getDestReg(instruction);
        state->reg[destReg] = state->reg[regA] + state->reg[regB];
        state->pc++;
        break;

    case 1: // nor
        regA = getRegA(instruction);
        regB = getRegB(instruction);
        destReg = getDestReg(instruction);
        state->reg[destReg] = ~(state->reg[regA] | state->reg[regB]);
        state->pc++;
        break;

    case 2: // lw
    {
        regA = getRegA(instruction);
        regB = getRegB(instruction);
        offset = getOffset(instruction);

        int effectiveAddress = state->reg[regA] + offset;

        // Check if the new address is out of range
        if (effectiveAddress < 0 || effectiveAddress >= MEMORYSIZE)
        {
            fprintf(stderr, "Error: Memory access out of bounds at PC %d (Effective Address: %d)\n", state->pc, effectiveAddress);
        }

        state->reg[regB] = state->mem[effectiveAddress];
        state->pc++;
        break;
    }

    case 3: // sw
    {
        regA = getRegA(instruction);
        regB = getRegB(instruction);
        offset = getOffset(instruction);

        int effectiveAddress = state->reg[regA] + offset;

        // Check if the new address is out of range
        if (effectiveAddress < 0 || effectiveAddress >= MEMORYSIZE)
        {
            fprintf(stderr, "Error: Memory access out of bounds at PC %d (Effective Address: %d)\n", state->pc, effectiveAddress);
        }

        state->mem[effectiveAddress] = state->reg[regB];
        state->pc++;
        break;
    }

    case 4: // beq
        regA = getRegA(instruction);
        regB = getRegB(instruction);
        offset = getOffset(instruction);

        if (state->reg[regA] == state->reg[regB])
        {
            state->pc = state->pc + 1 + offset;
        }
        else
        {
            state->pc++;
        }
        break;

    case 5: // jalr
        regA = getRegA(instruction);
        regB = getRegB(instruction);
        state->reg[regB] = state->pc + 1;
        state->pc = state->reg[regA];
        break;

    case 6: // halt
        *halt = true;
        state->pc++;
        break;

    case 7: // noop
        state->pc++;
        break;

    default:
        // Handle unknown opcode
        fprintf(stderr, "Error: Unknown opcode %d at address %d\n", opcode, state->pc);
    }

    // Ensure register 0 is always 0
    state->reg[0] = 0;
}

/*
 * Helper Functions
 */

// Opcode = bits 24-22
static int getOpcode(int instruction)
{
    return (instruction >> 22) & 0x7;
}

// RegA = bits 21-19
static int getRegA(int instruction)
{
    return (instruction >> 19) & 0x7;
}

// RegB = bits 18-16
static int getRegB(int instruction)
{
    return (instruction >> 16) & 0x7;
}

// DestReg = bits 2-0
static int getDestReg(int instruction)
{
    return instruction & 0x7;
}

// Offset = bits 15-0, sign-extends it
static int getOffset(int instruction)
{
    int offset = instruction & 0xFFFF;
    return convertNum(offset);
}

/*
 * DO NOT MODIFY ANY OF THE CODE BELOW.
 */

void printState(stateType *statePtr)
{
    int i;
    printf("\n@@@\nstate:\n");
    printf("\tpc %d\n", statePtr->pc);
    printf("\tmemory:\n");
    for (i = 0; i < statePtr->numMemory; i++)
    {
        printf("\t\tmem[ %d ] 0x%08X\n", i, statePtr->mem[i]);
    }
    printf("\tregisters:\n");
    for (i = 0; i < NUMREGS; i++)
    {
        printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
    }
    printf("end state\n");
}

// convert a 16-bit number into a 32-bit Linux integer
static inline int convertNum(int32_t num)
{
    return num - ((num & (1 << 15)) ? 1 << 16 : 0);
}