/*
    Group 45
    COP 3402
    Program 2 - Lexical Analyzer
    October 15, 2015

    This lexical analyzer should take in an input file input.txt and parse it into a file
    devoid of comments. It should then parse this comment-less file into the tokens which
    comprise and make up the functionality, and output them to two files, lexemelist and lexemetable
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// Defines a struct that represents all symbols
typedef struct
{
    int type;
    char name[12];
    int value;
    int lValue;
    int mValue;
} symbol;

// Defines a struct that represents each node in a linked list
typedef struct
{
    char * code;
    struct node * next;
} node;

// Creates an enum to enumerate all symbols
typedef enum
{
    Null_sym = 1, Ident_sym, Number_sym, Plus_sym, Minus_sym, Mult_sym, Slash_sym,
    Odd_sym, Eql_sym, Neq_sym, Less_sym, Leq_sym, Gtr_sym, Geq_sym, Lparent_sym,
    Rparent_sym, Comma_sym, Semicolon_sym, Period_sym, Becomes_sym, Begin_sym,
    End_sym, If_sym, Then_sym, While_sym, Do_sym, Call_sym, Const_sym, Var_sym,
    Proc_sym, Write_sym, Read_sym, Else_sym
} tokenType;

// Function prototypes
node * createNode();
node * alphaInitial(char initLetter, node * tail, FILE * ifp, FILE * ofp);
node * symbolInitial(char initSymbol, node * tail, FILE * ifp, FILE * ofp);
node * numericalInitial(char initVal, node * tail, FILE * ifp, FILE * ofp);
void getLex(FILE * lexTable, FILE * lexList, char * text, symbol * symTable, int * numSymbols);
int putInTable(char * text, symbol * symTable, int * numSymbols);

int lineNumber = 1;

// Main function, handles most file pointers and printing
int main()
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
