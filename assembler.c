/**
 * Project 1
 * Assembler code fragment for LC-2K
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Every LC2K file will contain less than 1000 lines of assembly.
#define MAXLINELENGTH 1000
#define MAXLABELS 100

struct Label
{
    char name[MAXLINELENGTH];
    int address;
};

struct Label labels[MAXLABELS];
int labelCount = 0;

int readAndParse(FILE *, char *, char *, char *, char *, char *);
static void checkForBlankLinesInCode(FILE *inFilePtr);
static inline int isNumber(char *);
static inline void printHexToFile(FILE *, int);
void firstPass(FILE *inFilePtr);
void secondPass(FILE *inFilePtr, FILE *outFilePtr);
static int isValidRegister(char *reg);
static int lineIsBlank(char *line);
static int isAlpha(char c);
static int isAlnum(char c);

int main(int argc, char **argv)
{
    char *inFileString, *outFileString;
    FILE *inFilePtr, *outFilePtr;

    if (argc != 3)
    {
        printf("error: usage: %s <assembly-code-file> <machine-code-file>\n",
               argv[0]);
        exit(1);
    }

    inFileString = argv[1];
    outFileString = argv[2];

    inFilePtr = fopen(inFileString, "r");

    if (inFilePtr == NULL)
    {
        printf("error in opening %s\n", inFileString);
        exit(1);
    }

    outFilePtr = fopen(outFileString, "w");

    if (outFilePtr == NULL)
    {
        printf("error in opening %s\n", outFileString);
        exit(1);
    }

    // Check for blank lines in the middle of the code.
    checkForBlankLinesInCode(inFilePtr);

    // First pass: calculate addresses for labels
    firstPass(inFilePtr);

    // Second pass: generate machine code
    secondPass(inFilePtr, outFilePtr);

    fclose(inFilePtr);
    fclose(outFilePtr);

    return 0;
}

static int isAlpha(char c)
{
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

static int isAlnum(char c)
{
    return (isAlpha(c) || (c >= '0' && c <= '9'));
}

void firstPass(FILE *inFilePtr)
{
    char label[MAXLINELENGTH], opcode[MAXLINELENGTH], arg0[MAXLINELENGTH],
        arg1[MAXLINELENGTH], arg2[MAXLINELENGTH];
    int address = 0;

    rewind(inFilePtr);

    while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2))
    {
        if (strlen(label) > 0)
        {

            if (strlen(label) > 6)
            {
                printf("Error: Label '%s' is too long (max 6 characters)\n", label);
                exit(1);
            }

            // Check for valid characters (letters and numbers, starting with a letter)
            if (!isAlpha(label[0]))
            {
                printf("Error: Label '%s' must start with a letter\n", label);
                exit(1);
            }
            for (int i = 1; i < strlen(label); i++)
            {
                if (!isAlnum(label[i]))
                {
                    printf("Error: Label '%s' contains invalid characters\n", label);
                    exit(1);
                }
            }

            for (int i = 0; i < labelCount; i++)
            {
                if (strcmp(label, labels[i].name) == 0)
                {
                    printf("Error: Duplicate label '%s' at address '%d\n", label, address);
                    exit(1);
                }
            }

            strcpy(labels[labelCount].name, label);
            labels[labelCount].address = address;
            labelCount++;

            if (labelCount >= MAXLABELS)
            {
                printf("Error: Too many labels\n");
                exit(1);
            }
        }

        if (strcmp(opcode, "add") == 0 || strcmp(opcode, "nor") == 0 ||
            strcmp(opcode, "lw") == 0 || strcmp(opcode, "sw") == 0 ||
            strcmp(opcode, "beq") == 0 || strcmp(opcode, "jalr") == 0 ||
            strcmp(opcode, "halt") == 0 || strcmp(opcode, "noop") == 0 ||
            strcmp(opcode, ".fill") == 0)
        {
            address++;
        }
        else if (strlen(opcode) > 0)
        {
            printf("Error: Invalid opcode '%s' at address %d\n", opcode, address);
            exit(1);
        }
    }
}

void secondPass(FILE *inFilePtr, FILE *outFilePtr)
{
    char label[MAXLINELENGTH], opcode[MAXLINELENGTH], arg0[MAXLINELENGTH],
        arg1[MAXLINELENGTH], arg2[MAXLINELENGTH];
    int address = 0;

    rewind(inFilePtr);

    while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2))
    {
        int machineCode = 0;

        // Handle different opcodes
        if (strcmp(opcode, "add") == 0)
        {

            if (!isValidRegister(arg0) || !isValidRegister(arg1) || !isValidRegister(arg2))
            {
                exit(1);
            }

            machineCode = (0 << 22) | (atoi(arg0) << 19) | (atoi(arg1) << 16) | atoi(arg2);
        }
        else if (strcmp(opcode, "nor") == 0)
        {

            if (!isValidRegister(arg0) || !isValidRegister(arg1) || !isValidRegister(arg2))
            {
                exit(1);
            }

            machineCode = (1 << 22) | (atoi(arg0) << 19) | (atoi(arg1) << 16) | atoi(arg2);
        }
        else if (strcmp(opcode, "lw") == 0 || strcmp(opcode, "sw") == 0)
        {

            if (!isValidRegister(arg0) || !isValidRegister(arg1))
            {
                exit(1);
            }

            int opcodeBits = (strcmp(opcode, "lw") == 0) ? 2 : 3;
            int offset;

            if (isNumber(arg2))
            {
                offset = atoi(arg2);
            }
            else
            {
                int found = 0;
                for (int i = 0; i < labelCount; i++)
                {
                    if (strcmp(labels[i].name, arg2) == 0)
                    {
                        offset = (strcmp(opcode, "beq") == 0) ? labels[i].address - (address + 1) : labels[i].address;
                        found = 1;
                        break;
                    }
                }
                if (!found)
                {
                    printf("Error: Label '%s' not found\n", arg2);
                    exit(1);
                }
            }

            if (offset < -32768 || offset > 32767)
            {
                printf("Error: offset '%d' out of range for instruction at address %d\n", offset, address);
                exit(1);
            }

            if (isNumber(arg2))
            {
                offset = atoi(arg2);
            }
            else
            {
                int found = 0;
                for (int i = 0; i < labelCount; i++)
                {
                    if (strcmp(labels[i].name, arg2) == 0)
                    {
                        offset = labels[i].address;
                        found = 1;
                        break;
                    }
                }
                if (!found)
                {
                    printf("Error: Label '%s' not found\n", arg2);
                    exit(1);
                }
            }

            machineCode = (opcodeBits << 22) | (atoi(arg0) << 19) | (atoi(arg1) << 16) | (offset & 0xFFFF);
        }
        else if (strcmp(opcode, "beq") == 0)
        {

            if (!isValidRegister(arg0) || !isValidRegister(arg1))
            {
                exit(1);
            }

            int opcodeBits = 4;
            int offset;

            if (isNumber(arg2))
            {
                offset = atoi(arg2);
            }
            else
            {
                int found = 0;
                for (int i = 0; i < labelCount; i++)
                {
                    if (strcmp(labels[i].name, arg2) == 0)
                    {
                        offset = labels[i].address - (address + 1);
                        found = 1;
                        break;
                    }
                }
                if (!found)
                {
                    printf("Error: Label '%s' not found\n", arg2);
                    exit(1);
                }
            }

            if (offset < -32768 || offset > 32767)
            {
                printf("Error: offset '%d' out of range for instruction at address %d\n", offset, address);
                exit(1);
            }

            machineCode = (opcodeBits << 22) | (atoi(arg0) << 19) | (atoi(arg1) << 16) | (offset & 0xFFFF);
        }
        else if (strcmp(opcode, "jalr") == 0)
        {
            if (!isValidRegister(arg0) || !isValidRegister(arg1))
            {
                exit(1);
            }
            machineCode = (5 << 22) | (atoi(arg0) << 19) | (atoi(arg1) << 16);
        }
        else if (strcmp(opcode, "halt") == 0)
        {
            machineCode = (6 << 22);
        }
        else if (strcmp(opcode, "noop") == 0)
        {
            machineCode = (7 << 22);
        }
        else if (strcmp(opcode, ".fill") == 0)
        {
            int fillValue;
            if (isNumber(arg0))
            {
                fillValue = atoi(arg0);

                // Check if the fill value exceeds the 32-bit range
                if (fillValue < INT32_MIN || fillValue > INT32_MAX)
                {
                    printf("Error: .fill value '%d' exceeds 32-bit limit\n", fillValue);
                    exit(1);
                }

                machineCode = fillValue;
            }
            else
            {
                int found = 0;
                for (int i = 0; i < labelCount; i++)
                {
                    if (strcmp(labels[i].name, arg0) == 0)
                    {
                        machineCode = labels[i].address;
                        found = 1;
                        break;
                    }
                }
                if (!found)
                {
                    printf("Error: Label '%s' not found for .fill\n", arg0);
                    exit(1);
                }
            }
        }
        else
        {
            printf("Error: Unrecognized opcode '%s' at address %d\n", opcode, address);
            exit(1);
        }

        // Write machine code to output file
        printHexToFile(outFilePtr, machineCode);
        address++;
    }
}

// Returns non-zero if the registers are out of range
static int isValidRegister(char *reg)
{
    if (!isNumber(reg))
    {
        printf("Error: Register '%s' is not a valid integer\n", reg);
        return 0;
    }

    int regNum = atoi(reg);
    if (regNum < 0 || regNum > 7)
    {
        printf("Error: Register '%d' out of range (must be between 0 and 7)\n", regNum);
        return 0;
    }

    return 1;
}

// Returns non-zero if the line contains only whitespace.
static int lineIsBlank(char *line)
{
    char whitespace[4] = {'\t', '\n', '\r', ' '};
    int nonempty_line = 0;
    for (int line_idx = 0; line_idx < strlen(line); ++line_idx)
    {
        int line_char_is_whitespace = 0;
        for (int whitespace_idx = 0; whitespace_idx < 4; ++whitespace_idx)
        {
            if (line[line_idx] == whitespace[whitespace_idx])
            {
                line_char_is_whitespace = 1;
                break;
            }
        }
        if (!line_char_is_whitespace)
        {
            nonempty_line = 1;
            break;
        }
    }
    return !nonempty_line;
}

// Exits 2 if file contains an empty line anywhere other than at the end of the file.
// Note calling this function rewinds inFilePtr.
static void checkForBlankLinesInCode(FILE *inFilePtr)
{
    char line[MAXLINELENGTH];
    int blank_line_encountered = 0;
    int address_of_blank_line = 0;
    rewind(inFilePtr);

    for (int address = 0; fgets(line, MAXLINELENGTH, inFilePtr) != NULL; ++address)
    {
        // Check for line too long
        if (strlen(line) >= MAXLINELENGTH - 1)
        {
            printf("error: line too long\n");
            exit(1);
        }

        // Check for blank line.
        if (lineIsBlank(line))
        {
            if (!blank_line_encountered)
            {
                blank_line_encountered = 1;
                address_of_blank_line = address;
            }
        }
        else
        {
            if (blank_line_encountered)
            {
                printf("Invalid Assembly: Empty line at address %d\n", address_of_blank_line);
                exit(2);
            }
        }
    }
    rewind(inFilePtr);
}

/*
 * NOTE: The code defined below is not to be modifed as it is implimented correctly.
 */

/*
 * Read and parse a line of the assembly-language file.  Fields are returned
 * in label, opcode, arg0, arg1, arg2 (these strings must have memory already
 * allocated to them).
 *
 * Return values:
 *     0 if reached end of file
 *     1 if all went well
 *
 * exit(1) if line is too long.
 */
int readAndParse(FILE *inFilePtr, char *label, char *opcode, char *arg0,
                 char *arg1, char *arg2)
{
    char line[MAXLINELENGTH];
    char *ptr = line;

    /* delete prior values */
    label[0] = opcode[0] = arg0[0] = arg1[0] = arg2[0] = '\0';

    /* read the line from the assembly-language file */
    if (fgets(line, MAXLINELENGTH, inFilePtr) == NULL)
    {
        /* reached end of file */
        return (0);
    }

    /* check for line too long */
    if (strlen(line) == MAXLINELENGTH - 1)
    {
        printf("error: line too long\n");
        exit(1);
    }

    // Ignore blank lines at the end of the file.
    if (lineIsBlank(line))
    {
        return 0;
    }

    /* is there a label? */
    ptr = line;
    if (sscanf(ptr, "%[^\t\n ]", label))
    {
        /* successfully read label; advance pointer over the label */
        ptr += strlen(label);
    }

    /*
     * Parse the rest of the line.  Would be nice to have real regular
     * expressions, but scanf will suffice.
     */
    sscanf(ptr, "%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]",
           opcode, arg0, arg1, arg2);

    return (1);
}

static inline int
isNumber(char *string)
{
    int num;
    char c;
    return ((sscanf(string, "%d%c", &num, &c)) == 1);
}

// Prints a machine code word in the proper hex format to the file
static inline void
printHexToFile(FILE *outFilePtr, int word)
{
    fprintf(outFilePtr, "0x%08X\n", word);
}
