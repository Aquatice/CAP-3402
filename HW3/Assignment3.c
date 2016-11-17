/*
			Francisco Rios, Casey Barth, Scot Wells 
				     COP 3402 - Assignment 3
					    November 15, 2015


*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Constants
// Keeps track of all the useful constants throughout the program
#define CMAX 12
#define CODE_SIZE 100
#define ERRMAX 4
#define TRUE 1
#define FALSE 0
#define MAX_SYMBOL_TABLE_SIZE 100

// Creates an enum to enumerate all symbols
typedef enum
{
    nulsym = 1, identsym, numbersym, plussym, minussym, multsym, slashsym,
    oddsym, eqlsym, neqsym, lessym, leqsym, gtrsym, geqsym, lparentsym,
    rparentsym, commasym, semicolonsym, periodsym, becomessym, beginsym,
    endsym, ifsym, thensym, whilesym, dosym, callsym, constsym, varsym,
    procsym, outsym, readsym, elsesym
} tokenType;

// Enums
enum OPCODE {NUL, LIT, OPR, LOD, STO, CAL, INC, JMP, JPC, SIO1, SIO2, SIO3};
enum STACK_OPERATION {
	RET, NEG, ADD, SUB, MUL, DIV, ODD,
	MOD, EQL, NEQ, LSS, LEQ, GTR, GEQ
};
enum SIOOPS {SOT = 1, SIN, HLT};

// Constant arrays holding OPCODE values. Indecies correspond to OPCODE Number
const char* OPCODE_STRINGS[] = {
	"nul", "lit", "opr", "lod", "sto", "cal", 
	"inc", "jmp", "jpc", "sio", "sio", "sio"
};

// Symbol names, and other information that we'll
// need throughout the course of this program
char * symbolName[] = {
	"", "nul", "ident", "number", "plus", "minus",
	"mult", "slash", "odd", "eql", "neq", "les",
	"leq", "grt", "geq", "lparent", "rparent", "comma",
	"semicolon", "period", "becomes", "begin", "end",
	"if", "then", "while", "do", "call", "const",
	"var", "proc", "out", "read", "else"
};

// Declaration of the input file pointer that will 
// be used greatly throughout this program
FILE* input;
FILE* symbolTableOutput;
FILE* codefp;

// Declaration of required global variables
// Keeps track of other useful information
int code_size = 0;
int lexilvl = 0;
int line = 0;
int first = 1;
int currTokenLocation = 0;
int cx = 0;
int numSymbols = 0;
int m = 0;
int currentIdentityType;
int argT = 0;
int argS = 0;
int argM = 0;
int argA = 0;
int argVM = 0;

// Declaration of the 'code' struct that holds an opcode value, l, and m
typedef struct{
	int op;
	int l;
	int m;
} code;

typedef struct{
    int sp;
    char outVari[5];
} OutPrint;

// Declaration of the symbol struct that 
// holds the symbolic representation of a code
typedef struct{
	int kind;
	char name[10];
	int val;
	int level;
	int addr;
} symbol;

// Declaration of the struct token
typedef struct{
    int kind;
    char name[CMAX+1];
	int value;
	int hasValue;
} token;

// Declaring the useful enums that will be 
// used frequently throughout this project
typedef enum {VARERR = 0, NUMERR, VARLENERR, SYMBERR} error_type;

// Creating the structs and arrays of structs that will be utilized
token currToken;
token nameRec[500];
OutPrint Pout[100];

// Creating the symbol array and single symbols needed
symbol symbol_table[MAX_SYMBOL_TABLE_SIZE];
symbol tempSymbol;
symbol Osymbol;
symbol Isymbol;

// Creating the array of code
code codeStack[CODE_SIZE];

// Look at all those functions we're going to be using
int checkIdentType();
symbol getSymbol();
token getToken();
void Program();
void Block();
void Statement();
void Condition();
void Expression();
void Term();
void Factor();
void nextToken();
void emit(int op, int l, int m);
void printSymbolTable(FILE *);
void printCode(FILE*);
void printParserError(int errorNum);

/**
 * Reads a text file, provided as an argument, that should be parsed
 * and turned into machine code that can be executed. 
 * 		
 * @param  argc  number of arguments passed
 * @param  argv  array of strings of the arguments that were passed
 * @return       0 for success
 */
int main(int argc, char* argv[])
{
	int i = 0;
	for(i = 0; i < argc; i++)
	{
		if(argv[i] == 't')
		{
			// Print out the token list!
			argT = 1;
		}
		else if(argv[i] == 's')
		{
			// Print out the symbol table
			argS = 1;
		}
		else if(argv[i] == 'm')
		{
			// Print out the machine code
			argM = 1;
		}
		else if(argv[i] == 'a')
		{
			// Print out the disassembled code
			argA = 1;
		}
		else if(argv[i] == 'v')
		{
			// Print out the stack trace
			argVM = 1;
		}
	}
    // Open the input and output text files
    input = fopen("tokenlist.txt", "r");

    symbolTableOutput = fopen("symboltable.txt", "w");
    codefp = fopen("mcode.txt", "w");

    while (feof(input) != 1) {
    	nameRec[currTokenLocation++] = getToken();
    }

    // reset token location
    currTokenLocation = 0;

    // parse the program
    Program();

    printCode(codefp);

    printSymbolTable(symbolTableOutput);

    fclose(input);
    fclose(symbolTableOutput);
    fclose(codefp);

	return 0;
}

/**
 * retrieves a token from the input file.
 * 
 * @return the next token from the input file
 */
token getToken() {
	int kind;
	token Token;

	Token.kind = 0;
	Token.hasValue = 0;
	Token.value = 0;

    if (fscanf(input, "%d", &kind) != 0) {
    	Token.kind = kind;

    	if (kind == identsym) {
    		if (fscanf(input, "%s", Token.name) == 0) {
    			printf("Could not get identsym name\n");
    			exit(1);
    		}
    	} else if (kind == numbersym) {
    		if (fscanf(input, "%d", &Token.value) == 0) {
    			printf("Could not get numbersym number\n");
    			exit(1);
    		}
    	}
    }

    return Token;
}

/**
 * makes the global, curr token point to the next token in the list
 */
void nextToken() {
	currToken = nameRec[currTokenLocation++];
}

// BEGINNING OF PARSER CODE
// ============================================================================
void Program() {
    // Get the next token
	nextToken();
	
	Block();

	// If the program isn't ended by a period
	if (currToken.kind != periodsym) {
        // Throw an error
		printParserError(9);
	} else {
	    // Otherwise, tell the user we're good!
		printf("\nNo errors, program is syntactically correct\n");
	}
}

void Block() {
	int ctemp; // Holds the code index
	int currM; // Holds the number of variables

	// If we're on the first lexicographic level
	if (first) {
        // Set the level equal to 0 and that we're no longer on the first
		lexilvl = 0;
		first = FALSE;
	} else {
        // Otherwise move up a lexicographical level
		lexilvl++;
	}

	// If the token is a constant
	if (currToken.kind == constsym) {
		do {
			// Copy the type into the symbol struct
			symbol_table[numSymbols].kind = constsym;

            // Then get the next token
			nextToken();

			// If the token is of the type ident
			if (currToken.kind != identsym) {
                // Throw an error
				printParserError(4);
			}

			// Copy name into symbol struct
			strcpy(symbol_table[numSymbols].name, currToken.name);

            // Get the next token
			nextToken();

			// If that token isn't an equals sign, throw an error
			if (currToken.kind != eqlsym) {
				printParserError(3);
			}

            // Get the next token
			nextToken();

			// If that token isn't a number, throw an error
			if (currToken.kind != numbersym) {
				printParserError(2);
			}

			// Copy the value into the symbol struct
			symbol_table[numSymbols].val = currToken.value;

			// Increase number of symbols
			numSymbols++;

            // Then grab the next token
			nextToken();

		} while (currToken.kind == commasym); // Continue if there's a comma found

        // If we find something other than a semicolon afterwards
		if (currToken.kind != semicolonsym) {
            // Throw an error
			printParserError(5);
		}

		line++;
		
		// Update current Line
		nextToken();
	}

	// Make room for DL/ SL/ RA
	m = 0;

	currM = 4;

	// If the current token is a variable
	if (currToken.kind == varsym) {
		do {
			// Copy type into the symbol Struct
			symbol_table[numSymbols].kind = varsym;

            // And get the next token
			nextToken();

            // You know the drill by now, if we don't find the expected symbol
			if (currToken.kind != identsym) {
                // Throw an error
				printParserError(4);
			}

			// Copy name, level, and address into the symbol struct
			strcpy(symbol_table[numSymbols].name, currToken.name);
			symbol_table[numSymbols].level = lexilvl;
			symbol_table[numSymbols].addr = currM++;

			// Increase the symbol counter
			numSymbols++;

            // And get the next token
			nextToken();
		} while (currToken.kind == commasym);

        // If we don't encounter a semicolon after this, throw an error
		if (currToken.kind != semicolonsym) {
			printParserError(5);
		}

		m += currM;

        // Get the next token
		nextToken();
	}

	// If the token is a procedure
	if (currToken.kind == procsym) {
        // Then we need to prepare appropriately
		ctemp = cx;
		emit(JMP, 0, 0);
	} else {
		emit(INC, 0, m);
	}

	// So long as the current token is still a procedure
	while (currToken.kind == procsym) {

		// Store the symbol kind into the symbol Struct
		symbol_table[numSymbols].kind = procsym;

		emit(JMP, 0, cx+1);

        // Grab the next token
		nextToken();

        // If the next token isn't an identifier, throw an error
		if (currToken.kind != identsym) {
			printParserError(6);
		}

		// Store name, level, and address in the symbol Struct
		strcpy(symbol_table[numSymbols].name, currToken.name);
		symbol_table[numSymbols].level = lexilvl;
		symbol_table[numSymbols].addr = cx - 1;

        // Increase the numSymbols counter by one
		numSymbols++;

        // And get the next token
		nextToken();

        // Now we expect a semicolon, if one is not found, we throw an error
		if (currToken.kind != semicolonsym) {
			printParserError(5);
		}

        // Then get the next token
		nextToken();

        // And execute the block
		Block();

        // Semicolon expected, you know the drill
		if (currToken.kind != semicolonsym) {
			printParserError(5);
		}

		// Store where to jump in first jump command
		codeStack[ctemp].m = cx;

        // Emit, increase the line number, and grab the next token
		emit(INC, 0, currM);

		nextToken();
	}

    // If the found token is not a 'begin', throw an error
	if (currToken.kind != beginsym) {
		printParserError(7);
	}

	// And run the statement within
	Statement();

    // Emit call
    if (lexilvl <= 0) {
        emit(SIO3, 0, HLT);
    } else {
        emit(OPR, 0, RET);
    }

	// Decrease the lexicographical level
	lexilvl--;
}

void Statement() {
    // Holds address value
	int currM;
	int ctemp;

	// Holds a separate address value for JMP
	int jTemp;

	// Holds a symbol
	symbol locTempSymbol;

    // Check to ensure the current token is an identifier
	if (currToken.kind == identsym) {
		// Checks if the variable is in the symbol table and is the correct kind
		currentIdentityType = checkIdentType();

		// If it's a constant or a procedure, throw an error
		if (currentIdentityType == constsym || currentIdentityType == procsym) {
			printParserError(12);
		}

        // If it's equal to 0, throw an error
		if (currentIdentityType == 0) {
			printParserError(11);
		}

		// Stores current symbol
		locTempSymbol = getSymbol();

        // And gets the next token
		nextToken();

        // Watches for anything other than a becomes symbol
		if (currToken.kind != becomessym) {
            // If it's not found and we find an equals sign
			if (currToken.kind == eqlsym) {
                // Throw this error
				printParserError(1);
			}

            // If it's not found throw this error
			printParserError(13);
		}

		// Get the next token
		nextToken();

        // Runs the expression
		Expression();

		// Store symbol and update symbol table
		if (lexilvl != 0) {
			emit(STO, abs(lexilvl - locTempSymbol.level), locTempSymbol.addr);
		} else {
			emit(STO, lexilvl, locTempSymbol.addr);
		}
	} else if (currToken.kind == callsym) {
        // Get the next token
		nextToken();

        // If that token is not an identifier, destroy everything
		if (currToken.kind != identsym) {
			printParserError(14);
		}

		// Check if the identifier is on symbol
		// table and the correct type of symbol
		currentIdentityType = checkIdentType();

        // More error handling, there's only so many
        // times I can type out the same comment
		if (currentIdentityType == constsym || currentIdentityType == varsym) {
			printParserError(15);
		}

		if (currentIdentityType == 0) {
			printParserError(11);
		}

        // Gets the symbol and stores it in a temporary variable
		tempSymbol = getSymbol();

        // If the lexicographical level is 
        // anything other than 0, special emit case
		if (lexilvl != 0) {
			emit(CAL, abs(lexilvl - tempSymbol.level), tempSymbol.addr);
		} else {
			emit(CAL, tempSymbol.level, tempSymbol.addr);
		}

		// And get the next token
		nextToken();
	} else if (currToken.kind == beginsym) {
        // Get the next token and run the statement within
		nextToken();
		Statement();

        // While our current token is a semicolon
		while (currToken.kind == semicolonsym) {
            // get the next token, and run the statement
			nextToken();
			Statement();
		}

		// If we encounter anything but an end symbol at the end
		if (currToken.kind != endsym) {
            // I AM ERROR
			printParserError(8);
		}

		// And get the next token
		nextToken();
	} else if (currToken.kind == ifsym) {
        // Get the next token
		nextToken();

		// And run the condition of the if
		Condition();

        // If our if symbol is not followed by a then symbol
		if (currToken.kind != thensym) {
            // Throw an error
			printParserError(16);
		}
        
		nextToken();

		// Assign proper values, emit
		ctemp = cx;
		emit(JPC, 0, 0);

		// And run the statement
		Statement();

        // Set the m value in the code stack to cx
		codeStack[ctemp].m = cx + 1;

        // Afterwards, set the temporary memory address and emit JMP
		jTemp = cx;
		emit(JMP, 0, 0);

        if (currToken.kind == elsesym) {
            // Get a new token, and then run the statement (and update codeStack)
			nextToken();
			Statement();
			codeStack[jTemp].m = cx;
		} else {
			// if we don't find an else block then we want to go back 
			// to the previous token
			currTokenLocation--;
			currToken = nameRec[currTokenLocation];
			codeStack[jTemp].m = cx;
		}
	} else if (currToken.kind == whilesym) {
        // Initialize and set currM
		int currM = cx;

        // Get the next token
		nextToken();

        // And evaluate the while loop conditional
		Condition();

        // If this isn't followed by a do statement, throw an error
		if (currToken.kind != dosym) {
			printParserError(18);
		}

		// And get the next token
		nextToken();
		ctemp = cx;

        // Emit the JPC
		emit(JPC, 0, 0);

        // Evaluate the statement
		Statement();

        // And then emit the JMP
		emit(JMP, 0, currM);

		codeStack[ctemp].m = cx;
	} else if (currToken.kind == readsym) {
        // Get the next token
		nextToken();

        // If the next token wasn't an identifier
		if (currToken.kind != identsym) {
            // ERROR, obviously
			printParserError(26);
		}

        // Get the symbol
		Isymbol = getSymbol();

        // Emit the SIO command
		emit(SIO1, 0, SIN);

        // Check lexicographical level and accommodate accordingly
		if (lexilvl != 0) {
			emit(STO, abs(lexilvl - Isymbol.level), Isymbol.addr);
		} else {
			emit(STO, lexilvl, Isymbol.addr);
		}

        // Then get the next token
		nextToken();
	} else if (currToken.kind == outsym) {
        // Get the next token
		nextToken();

        // Error handling
		if (currToken.kind != identsym) {
			printParserError(27);
		}

		Osymbol = getSymbol();

        // Emit accordingly
		emit(LOD, 0, Osymbol.addr);
		emit(SIO2, 0, SOT);

        // Get the next token
		nextToken();
	}
}

void Condition() {
    // This function would be really repetitive to comment, so in short:
    // It runs the conditionals attached to if or while statements
    // Comparisons like <, >, >=, <=, are all handled here
	int relatop;

	if (currToken.kind == oddsym) {
		relatop = currToken.kind;
		nextToken();
		Expression();

		if (relatop == oddsym) {
			emit(OPR, 0, ODD);
		}
	} else {
		Expression();

		if (currToken.kind != gtrsym && currToken.kind != geqsym &&
		   currToken.kind != lessym && currToken.kind != leqsym &&
		   currToken.kind != eqlsym && currToken.kind != neqsym)
			printParserError(20);

		else {
			if (currToken.kind == gtrsym) {
				relatop = gtrsym;
			} else if (currToken.kind == geqsym) {
				relatop = geqsym;
			} else if (currToken.kind == lessym) {
				relatop = lessym;
			} else if (currToken.kind == leqsym) {
				relatop = leqsym;
			} else if (currToken.kind == eqlsym) {
				relatop = eqlsym;
			} else if (currToken.kind == neqsym) {
				relatop = neqsym;
			}
		}
		nextToken();
		Expression();

		if (relatop == gtrsym) {
			emit(OPR, 0, GTR);
		} else if (relatop == geqsym) {
			emit(OPR, 0, GEQ);
		} else if (relatop == lessym) {
			emit(OPR, 0, LSS);
		} else if (relatop == leqsym) {
			emit(OPR, 0, LEQ);
		} else if (relatop == eqlsym) {
			emit(OPR, 0, EQL);
		} else if (relatop == neqsym) {
			emit(OPR, 0, NEQ);
		}
	}
}

void Expression() {
    // Simply handles the evaluation of expressions, like addition or subtraction
	int addop;
	if (currToken.kind == plussym || currToken.kind == minussym) {
		addop = currToken.kind;
		nextToken();
		Term();
		if (addop == minussym) {
            // Negate
			emit(OPR, 0, NEG);
		}
	} else {
		Term();
	}

	while (currToken.kind == plussym || currToken.kind == minussym) {
		addop = currToken.kind;
		nextToken();
		Term();
		if (addop == plussym) {
            // Addition
			emit(OPR, 0, ADD);
		} else {
            // Subtraction
			emit(OPR, 0, SUB);
		}
	}
}

void Term() {
	int mulop;
	Factor();

    // Handles multiplication and division
	while (currToken.kind == multsym || currToken.kind == slashsym) {
		mulop = currToken.kind;

		nextToken();

		Factor();

		if (mulop == multsym) {
			emit(OPR, 0, MUL);
		} else {
			emit(OPR, 0, DIV);
		}
	}
}

void Factor() {
    // If the current token is an identifier
	if (currToken.kind == identsym) {
        // Check and hold its type
		currentIdentityType = checkIdentType();

        // If it's a constant
		if (currentIdentityType == constsym) {
            // Emit LIT
			emit(LIT, 0, tempSymbol.val);
		}

        // If it's a variable
		else if (currentIdentityType == varsym) {
            // Emit accordingly
			if (lexilvl != 0) {
				emit(LOD, abs(lexilvl - tempSymbol.level), tempSymbol.addr);
			} else {
				emit(LOD, tempSymbol.level, tempSymbol.addr);
			}
		}
        // Procedure is not valid here
		else if (currentIdentityType == procsym) {
			printParserError(21);
		}
        // Neither is 0
		else if (currentIdentityType == 0) {
			printParserError(11);
		}

        // Get the next token
		nextToken();
	}
	// Otherwise, if it's a number
	else if (currToken.kind == numbersym) {
        // Emit LIT, and get the next token
		emit(LIT, 0, currToken.value);
		nextToken();
	}
	// Otherwise if it's a left parent
	else if (currToken.kind == lparentsym) {
        // Get the next token and evaluate the expression
		nextToken();
		Expression();

        // If the next token is anything but a right parent, throw an error
		if (currToken.kind != rparentsym) {
			printParserError(22);
		}

        // And get the next token
		nextToken();
	} else
        // Otherwise error
		printParserError(23);
}

/**
 * Outputs an error message and exits the program based on a provided 
 * error number.
 * 
 * @param errorNum number identifying the error to output
 */
void printParserError(int errorNum) {
    // Errors are assigned a numerical value that corresponds to an error
    // In the switch statement below. Not much more to explain than that
	printf("\n");
	printf("ERROR on line %d: \n", line);
	switch(errorNum) {
		case 1: printf("Use = instead of :=.\n");
			break;
		case 2: printf("= must be followed by a number.\n");
			break;
		case 3: printf("Identifier must be followed by =.\n");
			break;
		case 4: printf("const, var, procedure must be followed by identifier.\n");
			break;
		case 5: printf("Semicolon or comma missing.\n");
			break;
		case 6: printf("Incorrect symbol after procedure declaration.\n");
			break;
		case 7: printf("Statement expected.\n");
			break;
		case 8: printf("Incorrect symbol after statement part in block.\n");
			break;
		case 9: printf("Period expected.\n");
			break;
		case 10: printf("Semicolon between statements missing.\n");
			break;
		case 11: printf("Undeclared identifier.\n");
			break;
		case 12: printf("Assignment to constant or procedure is not allowed.\n");
			break;
		case 13: printf("Assignment operator expected.\n");
			break;
		case 14: printf("call must be followed by an identifier.\n");
			break;
		case 15: printf("Call of a constant or variable is meaningless.\n");
			break;
		case 16: printf("then expected.\n");
			break;
		case 17: printf("Semicolon or } expected.\n");
			break;
		case 18: printf("do expected.\n");
			break;
		case 19: printf("Incorrect symbol following statement.\n");
			break;
		case 20: printf("Relational operation expected.\n");
			break;
		case 21: printf("Expression must not contain a procedure identifier.\n");
			break;
		case 22: printf("Right parenthesis missing.\n");
			break;
		case 23: printf("The preceding factor cannot being with this symbol.\n");
			break;
		case 24: printf("An expression cannot begin with this symbol.\n");
			break;
		case 25: printf("This number is too large.\n");
			break;
		default:
			break;
	}
	// Quit if we get an error
	exit(1);
}

/**
 * looks up a symbol based on the current token value
 * @return  the symbol that was found
 */
symbol getSymbol() {
	symbol sym;
    // Gets a requested symbol
	int i;
	for(i = numSymbols; i >= 0; i--) {
		if (strcmp(currToken.name, symbol_table[i].name) == 0) {
			return symbol_table[i];
		}
	}

	return sym;
}

/**
 * @return identifier type based on the current token
 */
int checkIdentType() {
    // Returns an integer representing the identifier type
	int i;
	for(i = numSymbols; i >= 0; i--) {
		if (strcmp(currToken.name, symbol_table[i].name) == 0) {
			tempSymbol = symbol_table[i];
			return symbol_table[i].kind;
		}
	}

	return 0;
}

/**
 * creates a record in the code stack
 * 
 * @param op 
 * @param l 
 * @param m 
 */
void emit(int op, int l, int m) {
	if (cx > CODE_SIZE) {
		printParserError(25);
	} else {
		codeStack[cx].op = op;
		codeStack[cx].l = l;
		codeStack[cx].m = m;
		cx++;
	}
}

/**
 * prints the symbol table to an output file
 * 
 * @param output  File handle to symbol table should be printed to
 */
void printSymbolTable(FILE *output) {
    // Prints the symbol table
	int i;
	for(i = 0; i < numSymbols; i++) {
		fprintf(
			output,
			"%-10s\t%-8s\t%d\t%d\n", 
			symbol_table[i].name,
			symbolName[symbol_table[i].kind], 
			symbol_table[i].level,
			symbol_table[i].val == constsym ? symbol_table[i].val : symbol_table[i].addr
		);
		if(argS == 1)
		{
			// We've been asked to print this out to the console
			printf(
			"%-10s\t%-8s\t%d\t%d\n", 
			symbol_table[i].name,
			symbolName[symbol_table[i].kind], 
			symbol_table[i].level,
			symbol_table[i].val == constsym ? symbol_table[i].val : symbol_table[i].addr
		);
		}
	}
}

/**
 * Print the generated code to an output file
 * 
 * @param output  file handle the code should be printed to
 */
void printCode(FILE *output) {
    // Prints out the generated code
	int i;
	for(i = 0; i < cx; i++) {
		fprintf(
			output, 
			"%d %d %d\n", 
			codeStack[i].op, 
			codeStack[i].l, 
			codeStack[i].m
		);
		if(argM = 1)
		{
			// The machine code has been requested in the console
			printf(
			"%d %d %d\n", 
			codeStack[i].op, 
			codeStack[i].l, 
			codeStack[i].m
			);
		}
	}
}
