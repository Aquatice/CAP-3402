/*
    Sam Rios
    COP 3402
    PM/0 Virtual Machine
    September 17, 2015
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Constant arrays holding OPCODE values. Indecies correspond to OPCODE Number
const char* OPCODES[] = {"nul", "lit", "opr", "lod", "sto", "cal", "inc", "jmp", "jpc", "sio", "sio", "sio"};
const char* STACKOPR[] = {"RET", "NEG", "ADD", "SUB", "MUL", "DIV", "ODD", "MOD", "EQL", "NEQ", "LSS", "LEQ", "GTR", "GEQ"};

const int MAX_STACK_HEIGHT = 2000; // you can statically allocate a stack store of this size.
const int MAX_CODE_LENGTH = 500; // you can statically allocate a code store of this size.
const int MAX_LEXI_LEVELS = 3;

// Enums
enum OPCODE {NUL, LIT, OPR, LOD, STO, CAL, INC, JMP, JPC, SIO};
enum STACK_OP {RET, NEG, ADD, SUB, MUL, DIV, ODD, MOD, EQL, NEQ, LSS, LEQ, GTR, GEQ};

// Instruction structure
struct instructions
{
    int opCode;
    int L;
    int M;
};

// Registers for program
int PC = 0;
int BP = 1;
int SP = 0;
int IR = 0;
int halt = 0;


struct instructions currentInst;

// Creates an array with arbitrarily large size to serve as the stack
int stack[2000];

// Creates an array of instruction structs of arbitrarily large size
struct instructions code[500];

// Global variable to keep track of how long the program is
int progLength = 0;

// Prototypes
int base(int L, int base);
void printOPCodes();
void runProgram();
void nextInstruction();
void operate();
void stackPrinter();

// Now we can begin
int  virtualMachineMain()
{
    // Start by initializing the stack
    stack[1] = 0;
    stack[2] = 0;
    stack[3] = 0;

    // Create the input file and output file pointers
    FILE * ifp = fopen("mcode.txt", "r");
    FILE * ofp = fopen("stacktrace.txt", "w");

    // Variable to hold the info we pull out of mcode.txt
    int opCode, L, M;

    // Iterative variable
    int i = 0;

    // While we haven't reached the end of the file
    while(fscanf(ifp, "%d", &opCode) != EOF)
    {
        // Read in the L and M values (note we already read in the OPCode
        fscanf(ifp, "%d", &L);
        fscanf(ifp, "%d", &M);

        // Store the values in the current struct
        code[i].opCode = opCode;
        code[i].L = L;
        code[i].M = M;

        // Move on to the next struct
        i++;
    }

    // We're done loading the file, set progLength
    progLength = i;

    // Print the list of OP codes
    printOPCodes(ofp);

    // Output comes from runProgram, so go ahead and print the output header
    printf("Output: ");

    // Run the program!
    runProgram(ofp);

    return 0;
}

// Used to find the a base L levels deep
int base(int L, int base)
{
    while(L > 0)
    {
        base = stack[base+2];
        L--;
    }
    return base;
}

void printOPCodes(FILE* ofp)
{
    int i;

    // Print the header
    fprintf(ofp, "Line \t OP \t L \t M \n");

    // Print the data
    for(i=0; i < progLength; i++)
    {
        // Print (in order) the line number, the OP code, the value of L and the value of M, then print a new line
        fprintf(ofp, "%d    \t %s \t %d \t %d", i, OPCODES[code[i].opCode], code[i].L, code[i].M);
        fprintf(ofp, "\n");
    }
}

void runProgram(FILE* ofp)
{
    // Establishing formatting (painfully)
    fprintf(ofp, "\n \t\t\t\t\t pc \t bp \t sp \t stack\n");
    fprintf(ofp, "Initial Values  \t %d \t\t %d \t\t %d \n", PC, BP, SP);

    // When we get a BP = 0, then we've clearly hit a call of OPR -> RET, and we need to stop
    int i;
    for(i = 0; i < progLength-1; i++)
    {
            fprintf(ofp, "%2d  %s  %d  %d", PC, OPCODES[code[PC].opCode], code[PC].L, code[PC].M);
            if(halt != 1)
            {
                nextInstruction();
            }

            fprintf(ofp, "\t\t %2d \t %d \t\t %2d \t", PC, BP, SP);
            if(halt != 1)
            {
                if(i != 0)
                {
                    stackPrinter(ofp);
                }
            }
            else
            {
                exit(0);
            }
            fprintf(ofp, "\n");
    }
}

// Get the OPCode at the location given by PC and execute it accordingly
void nextInstruction()
{
    currentInst = code[PC];
    PC++;

    switch(currentInst.opCode)
    {
        // Push a literal (M) onto the stack
        case LIT:
            SP++;
            stack[SP] = currentInst.M;
            break;
        // Perform an operation on the stack, go to the operation function
        case OPR:
            operate();
            break;
        // Push the value at M from L levels down the stack
        case LOD:
            SP++;
            stack[SP] = stack[base(currentInst.L, BP) + currentInst.M];
            break;
        // Pop value off the stack and store it at M, L levels down
        case STO:
            stack[base(currentInst.L, BP) + currentInst.M] = stack[SP];
            SP--;
            break;
        // Call the code at index M
        case CAL:
            stack[SP+1] = 0;
            stack[SP+2] = base(currentInst.L, BP); //Static
            stack[SP+3] = BP;
            stack[SP+4] = PC;

            BP = SP + 1;
            PC = currentInst.M;
            break;
        // Allocate M locals at the top of the stack
        case INC:
            SP += currentInst.M;
            break;
        // Jump to instruction M
        case JMP:
            PC = currentInst.M;
            break;
        // Jump to instruction M if top element is 0
        case JPC:
            if(stack[SP] == 0)
            {
                PC = currentInst.M;
            }
            SP--;
            break;
        // Handles standard I/O
        case SIO:
            if(currentInst.M == 1)
            {
                // Print Out
                printf("%d", stack[SP]);
                SP--;
                break;
            }
            else if(currentInst.M == 2)
            {
                // Read In
                SP++;
                scanf("%d", &stack[SP]);
                break;
            }
            else if(currentInst.M == 3)
            {
                // HALT
                PC = 0;
                BP = 0;
                SP = 0;

                halt = 1;
                break;
            }
    }
}

// Contains the switch statement for the OPR command
void operate()
{
    switch(currentInst.M)
    {
        // Returns the current AR
        case RET:
            SP = BP - 1;
            PC = stack[SP+4];
            BP = stack[SP+3];
            break;
        // Gets the negative of the current value
        case NEG:
            stack[SP] = -stack[SP];
            break;
        // Adds the previous value and the current value together
        case ADD:
            SP--;
            stack[SP] = stack[SP] + stack[SP+1];
            break;
        // Subtracts the current value from the previous value
        case SUB:
            SP--;
            stack[SP] = stack[SP] - stack[SP+1];
            break;
        // Multiplies the previous value and the current value
        case MUL:
            SP--;
            stack[SP] = stack[SP] * stack[SP+1];
            break;
        // Divides the previous value by the current value
        case DIV:
            SP--;
            stack[SP] = stack[SP] / stack[SP+1];
            break;
        // Checks if the value is odd
        case ODD:
            if(stack[SP] % 2 == 1)
            {
                stack[SP] = 1;
            }
            else
            {
                stack[SP] = 0;
            }
            break;
        // Finds the previous value modulo the current value
        case MOD:
            SP--;
            stack[SP] = stack[SP] % stack[SP+1];
            break;
        // Checks to see if previous value and current value are equal
        case EQL:
            SP--;
            if(stack[SP] == stack[SP+1])
            {
                stack[SP] = 1;
            }
            else
            {
                stack[SP] = 0;
            }
            break;
        // Checks to see if previous and current values are inequal
        case NEQ:
            SP--;
            if(stack[SP] != stack[SP+1])
            {
                stack[SP] = 1;
            }
            else
            {
                stack[SP] = 0;
            }
            break;
        // Checks to see if previous value is less than current value
        case LSS:
            SP--;
            if(stack[SP] < stack[SP+1])
            {
                stack[SP] = 1;
            }
            else
            {
                stack[SP] = 0;
            }
            break;
        // Checks to see if previous value is less than OR equal to current value
        case LEQ:
            SP--;
            if(stack[SP] <= stack[SP+1])
            {
                stack[SP] = 1;
            }
            else
            {
                stack[SP] = 0;
            }
            break;
        // Checks to see if previous value is greater than current value
        case GTR:
            SP--;
            if(stack[SP] > stack[SP+1])
            {
                stack[SP] = 1;
            }
            else
            {
                stack[SP] = 0;
            }
            break;
        // Checks to see if previous value is greater than OR equal to current value
        case GEQ:
            SP--;
            if(stack[SP] >= stack[SP+1])
            {
                stack[SP] = 1;
            }
            else
            {
                stack[SP] = 0;
            }
            break;
    }
}

// Prints the | separator for each activation record
void stackPrinter(FILE* ofp)
{
    // If there are elements in the stack
    if(SP > -1 && halt != 1)
    {
        int stackHeight;

        // And the stack pointer is further than 3 ahead of the base pointer
        if(SP >= BP+3)
        {
            // SP represents the stack height
            stackHeight = SP;
        }
        else
        {
            // Otherwise, the stack height is given by BP + 3
            stackHeight = BP + 3;
        }
            int bps[3];
            bps[0] = BP;
            bps[1] = stack[bps[0] + 2];
            bps[2] = stack[bps[1] + 2];

            int i;

            // For the height of the stack
            for(i = 1; i <= stackHeight; i++)
            {
                // Print out each digit
                if(i != 0)
                {
                    fprintf(ofp, " %d ", stack[i]);
                }

                // Check to see if i+1 is anywhere in the array of base pointers
                if(BP != 0 && i != stackHeight)
                {
                    int member = 0;

                    int j;
                    for(j = 0; j < 3; j++)
                    {
                        if(i+1 == bps[j])
                        {
                            member = 1;
                        }
                    }

                    // If it is, we need to print a divider
                    if(member == 1)
                    {
                        fprintf(ofp, " | ");
                    }
                }
            }
    }
}
