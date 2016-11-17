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
int lineNumber = 1;


// Declaration of the 'code' struct that holds an opcode value, l, and m
typedef struct{
	int op;
	int l;
	int m;
} code;

typedef struct
{
    char * code;
    struct node * next;
} node;

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
// Function prototypes
int lexicalAnalyzerMain();
node * createNode();
node * alphaInitial(char initLetter, node * tail, FILE * ifp, FILE * ofp);
node * symbolInitial(char initSymbol, node * tail, FILE * ifp, FILE * ofp);
node * numericalInitial(char initVal, node * tail, FILE * ifp, FILE * ofp);
void getLex(FILE * lexTable, FILE * lexList, char * text, symbol * symTable, int * numSymbols);
int putInTable(char * text, symbol * symTable, int * numSymbols);
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
int virtualMachineMain();
int base(int L, int base);
void printOPCodes();
void runProgram();
void nextInstruction();
void operate();
void stackPrinter();

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
	lexicalAnalyzerMain();
	
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
	
	virtualMachineMain();

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
	}
}

/* ==============================================================================================
                               LEXICAL ANALYZER START
================================================================================================= */


// Main function, handles most file pointers and printing
int lexicalAnalyzerMain()
{
    int c;
    char character;

    int i = 0;
    int symbolCount = 0;

    // Initialization of the linked list that will hold the commands
    node *head;
    node *tail;
    head = tail = createNode();

    symbol symbolList[100];

    // Instantiation of each node
    for(i = 0; i < 100; i++)
    {
        symbolList[i].type = 0;
        strcpy(symbolList[i].name, "");
        symbolList[i].value = 0;
        symbolList[i].lValue = 0;
        symbolList[i].mValue = 0;
    }

    // Creation of file pointers
    FILE *ifp = fopen("input.txt", "r");
    FILE *ofp = fopen("cleaninput.txt", "w");

    c = fgetc(ifp);

    // Parse the input file and create the clean input file
    while(c != EOF)
    {
        // If the character is a letter
        if(isalpha(c))
        {
            character = c;
            tail = alphaInitial(c, tail, ifp, ofp);
            c = fgetc(ifp);
        }
        // If the character is a punctuation mark
        else if(ispunct(c))
        {
            character = c;
            tail = symbolInitial(c, tail, ifp, ofp);
            c = fgetc(ifp);
        }
        // If the character is a digit
        else if(isdigit(c))
        {
            tail = numericalInitial(c, tail, ifp, ofp);
            c = fgetc(ifp);
        }
        // If it's none of the above
        else
        {
            fprintf(ofp, "%c", c);
            c = fgetc(ifp);
        }
    }

    // Creates the output file pointers
    FILE * lexTable= fopen("lexemetable.txt", "w");
    FILE * lexList = fopen("tokenlist.txt", "w");

    // Prints the header for the Lexeme Table
    fprintf(lexTable, "Lexeme\t\tToken Type\n");

    // Prints the rows of the lexeme table
    for(i=0; head->next != NULL; head = head->next)
    {
        fprintf(lexTable, "%s\t\t\t", head->code);
        getLex(lexTable, lexList, head->code, symbolList, &symbolCount);
    }

    // Close all the file pointers
    fclose(lexTable);
    fclose(lexList);
    fclose(ofp);

    // End the program
    return 0;
}

/* Pre-Conditions: None
    Post-Conditions: Return a pointer to a node */
node * createNode()
{
    //Create a new node
    node * temp = malloc(sizeof(node));
    temp->next = NULL;

    // Return it
    return temp;
}

/* Pre-Conditions: Takes in a character, the tail of a linked list,
    an input file pointer, and an output file pointer
    Post-Conditions: Returns a pointer to a node*/
node * alphaInitial(char initLetter, node * tail, FILE * ifp, FILE * ofp)
{
    // Initial declarations, max word length, location, next char
    int length = 11;
    int nextChar;
    int location = 1;

    // Allocates memory for the code snippet
    char * code = calloc(length+1, sizeof(char));

    // Initialize code to "", then give it the first value
    strcpy(code, "");
    code[0] = initLetter;

    // Then get the next character
    nextChar = fgetc(ifp);

    // Loop through until we've reached something that isn't a digit and isn't a letter
    while(isalpha(nextChar) || isdigit(nextChar))
    {
        if(location >= length)
        {
            printf("ERROR - Line %d - Variable or Word Exceeds Maximum Length\n", lineNumber);
            printf("Terminating Program...\n");
            exit(1);
        }

        code[location] = nextChar;
        location++;
        nextChar = fgetc(ifp);
    }

    // THE EVIL IS DEFEATED
    if(strcmp(code, "begin") == 0)
    {
	// This makes sure that 'begin' can't escape our line counting (which relied on semi-colons)
    	lineNumber++;
    } 

    // Update the linked list
    tail->code = malloc(strlen(code)+1);
    strcpy(tail->code, code);
    tail->next = createNode();

    // Be free, small memory location! Free!
    free(code);

    // If we've reached the end, go backwards so main can read it
    if(nextChar != EOF)
    {
        fseek(ifp, -1, SEEK_CUR);
    }

    // Print the code
    fprintf(ofp, "%s", tail->code);
    
    // Return the new tail
    return tail->next;
}

/* Pre-Conditions: Takes in a character, the tail of a linked list,
    an input file pointer, and an output file pointer
    Post-Conditions: Returns a pointer to a node*/
node * symbolInitial(char initSymbol, node * tail, FILE * ifp, FILE * ofp)
{
    // Symbols can be no longer than 2 characters long
    int maxSymbols = 2;

    // Allocates memory for our string
    char * symbol = calloc(maxSymbols+1, sizeof(char));

    //Initializes symbol with initSymbol
    strcpy(symbol, "");
    symbol[0] = initSymbol;

    // Look at the first character and then act based on that information
    switch(initSymbol)
    {
        // Could be comment or division
        case '/':
        {
            char nextChar = fgetc(ifp);

            // If the next character is *, then we have a comment opener
            if(nextChar == '*')
            {
                // Keep looking for the */ that concludes the comment
                while(nextChar != '/')
                {
                    while (nextChar != '*')
                    {
                        if(nextChar == EOF)
                        {
                            printf("ERROR - No ending symbol for comments\n");
                            printf("Terminating program...\n");
                            exit(1);
                        }
                        nextChar = fgetc(ifp);
                    }
                    nextChar = fgetc(ifp);
                }

                // Once it has been found, return the tail
                return tail;
            }
            // Otherwise, return to the main program so it can be processed appropriately
            else
            {
                if(nextChar != EOF)
                {
                    fseek(ifp, -1, SEEK_CUR);
                }
            }
            break;
        }
        // < can result in three cases <, <>, or <=
        case '<':
        {
            char nextChar = fgetc(ifp);
            int location = 1;

            // Checks for <= or <>
            if(nextChar == '=' || nextChar == '>')
            {
                // And assigns accordingly if it's found
                symbol[location] = nextChar;
            }
            // Otherwise, revert and process as <
            else
            {
                if(nextChar != EOF)
                {
                    fseek(ifp, -1, SEEK_CUR);
                }
            }
            break;
        }
        // > can result in > or >=
        case '>':
        {
            char nextChar = fgetc(ifp);
            int location = 1;

            // Check for >=
            if(nextChar == '=')
            {
                // Assign accordingly if found
                symbol[location] = nextChar;
            }
            // Otherwise revert and process as >
            else
            {
                if(nextChar != EOF)
                {
                    fseek(ifp, -1, SEEK_CUR);
                }
            }
            break;
        }
        // Need to ensure it's being used properly
        case ':':
        {
            char nextChar = fgetc(ifp);
            int location = 1;

            // Assign accordingly if it is
            if(nextChar == '=')
            {
                symbol[location] = nextChar;
            }
            // Otherwise, it's no good, and we terminate
            else
            {
                printf("ERROR - Line %d - Invalid Symbol Input! \n", lineNumber);
		printf("Terminating Program... \n");
                exit(1);
            }
            break;
        }
        // These have no ambiguous cases, so process them at face value
        case '+':
            break;
        case '-':
            break;
        case '*':
            break;
        case '(':
            break;
        case ')':
            break;
        case '=':
            break;
        case ',':
            break;
        case '.':
            break;
        case ';':
        {
            lineNumber++;
            break;
        }
        // If it bears no similarity to any of the above, it's an invalid symbol
        default:
            printf("ERROR - Line %d - Invalid Symbol Input!\n", lineNumber);
	    printf("Terminating Program... \n");
            exit(1);
    }

    // Allocate memory and assign
    tail->code = malloc(strlen(symbol)+1);
    strcpy(tail->code, symbol);
    tail->next = createNode();

    // Be free, small memory location! Free!
    free(symbol);

    // Print it out
    fprintf(ofp, "%s", tail->code);

    // Return the new tail
    return tail->next;
}

/* Pre-Conditions: Takes in a character, the tail of a linked list,
    an input file pointer, and an output file pointer
    Post-Conditions: Returns a pointer to a node*/
node * numericalInitial(char initVal, node * tail, FILE * ifp, FILE * ofp)
{
    // Set head pointer, and max length of numerical values
    int length = 5;
    int location = 1;
    int nextVal;

    // Allocate space for a number up to 5 digits
    char * number = calloc(length+1, sizeof(char));
    strcpy(number, "");
    number[0] = initVal;

    nextVal = fgetc(ifp);

    // While the next value is a digit
    while(isdigit(nextVal))
    {
        // And is less than the max length
        if(location >= length)
        {
            printf("ERROR - Numerical Value exceeds 5 digits\n");
            printf("Terminating program... \n");
            exit(1);
        }

        // Read in a new number
        number[location] = nextVal;
        location++;

        nextVal = fgetc(ifp);
    }

    // If the next value is a letter, it's invalid, and we terminate
    if(isalpha(nextVal))
    {
        printf("ERROR - Line %d - Invalid Variable Name: Does not start with a letter.\n", lineNumber);
        printf("Terminating program... \n");
        exit(1);
    }

    // Creates a new tail node
    tail->code = malloc(strlen(number)+1);
    strcpy(tail->code, number);
    tail->next = createNode();

    // Be free, small memory location! Free!
    free(number);

    // Revert and allow the main program to work with it
    if(nextVal != EOF)
    {
        fseek(ifp, -1, SEEK_CUR);
    }

    // Print out the newly stored data
    fprintf(ofp, "%s", tail->code);

    // Return the next node
    return tail->next;
}

/* Pre-Conditions: Two output file pointers, a variable name, a table of symbols,
    and the number of symbols in the table
    Post-Conditions: N/A */
void getLex(FILE * lexTable, FILE * lexList, char * text, symbol * symTable, int * numSymbols)
{
    int i = 0;

    // If the first character is a letter
    if(isalpha(text[0]))
    {
        // Checks that variable length condition one more time
        if(strlen(text) > 11)
        {
            printf("ERROR - Variable name exceeds maximum length\n");
            printf("Terminating program... \n");
            exit(1);
        }

        // Check for all reserved words, and print the token values to their respective destinations
        if(strcmp(text, "odd") == 0)
        {
            fprintf(lexTable, "%d\n", 8);
            fprintf(lexList, "%d ", 8);
        }
        else if(strcmp(text, "begin") == 0)
        {
            fprintf(lexTable, "%d\n", 21);
            fprintf(lexList, "%d ", 21);
        }
        else if(strcmp(text, "end") == 0)
        {
            fprintf(lexTable, "%d\n", 22);
            fprintf(lexList, "%d ", 22);
        }
        else if(strcmp(text, "if") == 0)
        {
            fprintf(lexTable, "%d\n", 23);
            fprintf(lexList, "%d ", 23);
        }
        else if(strcmp(text, "then") == 0)
        {
            fprintf(lexTable, "%d\n", 24);
            fprintf(lexList, "%d ", 24);
        }
        else if(strcmp(text, "while") == 0)
        {
            fprintf(lexTable, "%d\n", 25);
            fprintf(lexList, "%d ", 25);
        }
        else if(strcmp(text, "do") == 0)
        {
            fprintf(lexTable, "%d\n", 26);
            fprintf(lexList, "%d ", 26);
        }
        else if(strcmp(text, "call") == 0)
        {
            fprintf(lexTable, "%d\n", 27);
            fprintf(lexList, "%d ", 27);
        }
        else if(strcmp(text, "const") == 0)
        {
            fprintf(lexTable, "%d\n", 28);
            fprintf(lexList, "%d ", 28);
        }
        else if(strcmp(text, "var") == 0)
        {
            fprintf(lexTable, "%d\n", 29);
            fprintf(lexList, "%d ", 29);
        }
        else if(strcmp(text, "procedure") == 0)
        {
            fprintf(lexTable, "%d\n", 30);
            fprintf(lexList, "%d ", 30);
        }
        else if(strcmp(text, "write") == 0)
        {
            fprintf(lexTable, "%d\n", 31);
            fprintf(lexList, "%d ", 31);
        }
        else if(strcmp(text, "read") == 0)
        {
            fprintf(lexTable, "%d\n", 32);
            fprintf(lexList, "%d ", 32);
        }
        else if(strcmp(text, "else") == 0)
        {
            fprintf(lexTable, "%d\n", 33);
            fprintf(lexList, "%d ", 33);
        }
        // If we're here, it isn't a reserved word, which means it's a variable
        else
        {
            // And as such they should be handled appropriately
            fprintf(lexTable, "%d\n", 2);
            i = putInTable(text, symTable, numSymbols);
            fprintf(lexList, "2 %s ", text);
        }
    }
    // If the first character is a digit
    else if(isdigit(text[0]))
    {
        // It's a value, and should be handled as such
        fprintf(lexTable, "3\n");
        i = putInTable(text, symTable, numSymbols);
        fprintf(lexList, "3 %s ", text);
    }
    // Finally, if the first character is a punctuation mark
    else if(ispunct(text[0]))
    {
        // Use the first character to switch between each statement
        switch(text[0])
        {
            case '+':
                fprintf(lexTable, "%d\n", 4);
                fprintf(lexList, "%d ", 4);
                break;
            case '-':
                fprintf(lexTable, "%d\n", 5);
                fprintf(lexList, "%d ", 5);
                break;
            case '*':
                fprintf(lexTable, "%d\n", 6);
                fprintf(lexList, "%d ", 6);
                break;
            case '/':
                fprintf(lexTable, "%d\n", 7);
                fprintf(lexList, "%d ", 7);
                break;
            case '=':
                fprintf(lexTable, "%d\n", 9);
                fprintf(lexList, "%d ", 9);
                break;
            // Remember to account for the multiple cases nested within <
            case '<':
                if(strlen(text) > 1)
                {
                    // <> case
                    if(strcmp(text, "<>") == 0)
                    {
                        fprintf(lexTable, "%d\n", 10);
                        fprintf(lexList, "%d ", 10);
                        break;
                    }
                    // <= case
                    else if(strcmp(text, "<=") == 0)
                    {
                        fprintf(lexTable, "%d\n", 12);
                        fprintf(lexList, "%d ", 12);
                        break;
                    }
                }
                // Otherwise, it's just <
                else
                {
                    fprintf(lexTable, "%d\n", 11);
                    fprintf(lexList, "%d ", 11);
                    break;
                }
                break;
            // Account for the multiple cases nested within >
            case '>':
                if(strlen(text) > 1)
                {
                    // >= case
                    if(strcmp(text, ">=") == 0)
                    {
                        fprintf(lexTable, "%d\n", 14);
                        fprintf(lexList, "%d ", 14);
                        break;
                    }
                }
                // Otherwise, it's just the > case
                else
                {
                    fprintf(lexTable, "%d\n", 13);
                    fprintf(lexList, "%d ", 13);
                    break;
                }
                break;
            case '(':
                fprintf(lexTable, "%d\n", 15);
                fprintf(lexList, "%d ", 15);
                break;
            case ')':
                fprintf(lexTable, "%d\n", 16);
                fprintf(lexList, "%d ", 16);
                break;
            case ',':
                fprintf(lexTable, "%d\n", 17);
                fprintf(lexList, "%d ", 17);
                break;
            case ';':
                fprintf(lexTable, "%d\n", 18);
                fprintf(lexList, "%d ", 18);
                break;
            case '.':
                fprintf(lexTable, "%d\n", 19);
                fprintf(lexList, "%d ", 19);
                break;
            case ':':
                fprintf(lexTable, "%d\n", 20);
                fprintf(lexList, "%d ", 20);
                break;
        }
    }
}

/* Pre-Conditions: Takes in a string, a table of symbols, and the number of symbols
    Post-Conditions: Returns an integer */
int putInTable(char * text, symbol * symTable, int * numSymbols)
{
    int i = 0;

    // Check the symbol table to see if it's already been added
    for(i = 0; i < *numSymbols; i++)
    {
        // And if it has, return it's index
        if(strcmp(symTable[i].name, text) == 0)
        {
            return i;
        }
    }

    // If not, put it on the last index, and increment the number of symbols
    strcpy(symTable[*numSymbols].name, text);
    *numSymbols = *numSymbols + 1;

    // Return the number of symbols, compensating for the new string
    return (*numSymbols-1);
}

/* ==============================================================================================
									VM START
================================================================================================= */

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

int virtualMachineMain()
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
