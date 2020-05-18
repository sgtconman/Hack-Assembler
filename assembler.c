
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define MAX_VARIABLES 1000
#define MAX_LINE_LENGTH 100
#define FIRST_VARIABLE 16 // first input-created variable is stored at address 16


typedef struct
{
    char *symbol;
    long value;
}
table;

// Represents a parsed line of code in a linked list
typedef struct node
{
    bool a_line;
    char address[MAX_LINE_LENGTH];
    char comp[4];
    char dest[5];
    char jump[5];
    struct node *next;
}
node;


node *code_list; // linked list for storing lines of cleaned code - no need for random access
int line_count = 0;
node *line_tail; // tracks most recent line in list for append time of O(1)

int symbol_count = 22; // Hack comes with 23 preset symbols
int variable_count = 0;

//using a table array (for simplicity) to store RAM address of predefined and user-created variables.
table symboltable[MAX_VARIABLES];

// C instruction translation tables
table dest_table[8];
table jump_table[8];
table comp_table[28];
int dest_count = 8;
int jump_count = 8;
int comp_count = 28;

// Prototypes
void initialize(void);
int parser(char raw_code[]);
int label_adder(char raw_code[]);
int code_adder(char raw_code[], bool a_check, bool jump_check, bool dest_check, int line_end, int line_start);
char* translate(node *line);
void symbol_checker(node *line);
void add_to_bin(node *in_line, char *out_line);
int mem_free(void);

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        printf("Usage: ./assembler [program].asm\n");
        return 1;
    }

    //loads preset Hack symbols and C line translations into the global tables
    initialize();

    char *filename = argv[1];

    // opens up the file for assembing
    FILE *pgrm_ptr = fopen(filename, "r");
    if (pgrm_ptr == NULL)
    {
        return 1;
    }

    char tmp_line[MAX_LINE_LENGTH];

    while (fgets(tmp_line,MAX_LINE_LENGTH,pgrm_ptr) != NULL)
    {
        parser(tmp_line); // loads line of code into code_list and any label into symboltable
    }

    fclose(pgrm_ptr);

    // creates output file called [filename].hack
    char output_name[strlen(filename)];
    strcpy(output_name, filename);
    output_name[strlen(filename)-4] = '\0';
    strcat (output_name, ".hack");
    FILE *output_ptr = fopen(output_name, "w");

    for (node *crawler = code_list; crawler != NULL; crawler = crawler->next)
    {
        char *trans_return = translate(crawler);
        fprintf(output_ptr, "%s\n", trans_return); // writes 1 line of translated code to output file
        free(trans_return); // frees the translation output allocated in the translate function

    }




    fclose(output_ptr);

    mem_free(); // frees all memory allocated to code_list and symboltable
}

void initialize(void)
{
    //initilizes preset Hack symbols
    symboltable[0].symbol = "SP", symboltable[0].value = 0;
    symboltable[1].symbol = "LCL", symboltable[1].value = 1;
    symboltable[2].symbol = "ARG", symboltable[2].value = 2;
    symboltable[3].symbol = "THIS", symboltable[3].value = 3;
    symboltable[4].symbol = "THAT", symboltable[4].value = 4;
    symboltable[5].symbol = "SCREEN", symboltable[5].value = 16384;
    symboltable[6].symbol = "KBD", symboltable[6].value = 24576;

    char tmp_str[16][4];
    for (int i = 0; i < 16; i++)
    {
        symboltable[i+7].value = i;
        sprintf(tmp_str[i], "R%i", i);
        symboltable[i+7].symbol = malloc(4*sizeof(char));
        strcpy(symboltable[i+7].symbol, tmp_str[i]);
    }

    //initilizes C instruction translation tables

    jump_table[0].symbol = "null", jump_table[0].value = 000;
    jump_table[1].symbol = "JGT", jump_table[1].value = 001;
    jump_table[2].symbol = "JEQ", jump_table[2].value = 010;
    jump_table[3].symbol = "JGE", jump_table[3].value = 011;
    jump_table[4].symbol = "JLT", jump_table[4].value = 100;
    jump_table[5].symbol = "JNE", jump_table[5].value = 101;
    jump_table[6].symbol = "JLE", jump_table[6].value = 110;
    jump_table[7].symbol = "JMP", jump_table[7].value = 111;

    dest_table[0].symbol = "null", dest_table[0].value = 000;
    dest_table[1].symbol = "M", dest_table[1].value = 001;
    dest_table[2].symbol = "D", dest_table[2].value = 010;
    dest_table[3].symbol = "MD", dest_table[3].value = 011;
    dest_table[4].symbol = "A", dest_table[4].value = 100;
    dest_table[5].symbol = "AM", dest_table[5].value = 101;
    dest_table[6].symbol = "AD", dest_table[6].value = 110;
    dest_table[7].symbol = "AMD", dest_table[7].value = 111;

    comp_table[0].symbol = "0", comp_table[0].value = 0101010;
    comp_table[1].symbol = "1", comp_table[1].value = 0111111;
    comp_table[2].symbol = "-1", comp_table[2].value = 0111010;
    comp_table[3].symbol = "D", comp_table[3].value = 0001100;
    comp_table[4].symbol = "A", comp_table[4].value = 0110000;
    comp_table[5].symbol = "!D", comp_table[5].value = 0001101;
    comp_table[6].symbol = "!A", comp_table[6].value = 0110001;
    comp_table[7].symbol = "-D", comp_table[7].value = 0001111;
    comp_table[8].symbol = "-A", comp_table[8].value = 0110011;
    comp_table[9].symbol = "D+1", comp_table[9].value = 0011111;
    comp_table[10].symbol = "A+1", comp_table[10].value = 0110111;
    comp_table[11].symbol = "D-1", comp_table[11].value = 0001110;
    comp_table[12].symbol = "A-1", comp_table[12].value = 0110010;
    comp_table[13].symbol = "D+A", comp_table[13].value = 0000010;
    comp_table[14].symbol = "D-A", comp_table[14].value = 0010011;
    comp_table[15].symbol = "A-D", comp_table[15].value = 0000111;
    comp_table[16].symbol = "D&A", comp_table[16].value = 0000000;
    comp_table[17].symbol = "D|A", comp_table[17].value = 0010101;
    comp_table[18].symbol = "M", comp_table[18].value = 1110000;
    comp_table[19].symbol = "!M", comp_table[19].value = 1110001;
    comp_table[20].symbol = "-M", comp_table[20].value = 1110011;
    comp_table[21].symbol = "M+1", comp_table[21].value = 1110111;
    comp_table[22].symbol = "M-1", comp_table[22].value = 1110010;
    comp_table[23].symbol = "D+M", comp_table[23].value = 1000010;
    comp_table[24].symbol = "D-M", comp_table[25].value = 1010011;
    comp_table[25].symbol = "M-D", comp_table[25].value = 1000111;
    comp_table[26].symbol = "D&M", comp_table[26].value = 1000000;
    comp_table[27].symbol = "D|M", comp_table[27].value = 1010101;

}


int parser(char raw_code[])
{

    bool code_check = 0;

    bool a_check = 0;
    bool dest_check = 0;
    bool jump_check = 0;

    bool label_check = 0;

    int line_end = 0;
    int line_start = 0;

    // performs initial pass on the line to check which elements are present and the length of the code
    for (int i=0; raw_code[i] != '\0' && raw_code[i] != '\n' && raw_code[i] != '\r'; i++)
    {
        line_end = i+1;

        switch(raw_code[i])
        {
            case '/' :
                if (raw_code[i+1] == '/')
                {
                    line_end=i;
                    goto loop_break;
                }
            case '=' :
                dest_check=1;
                break;
            case '(' :
                label_check=1;
                label_adder(raw_code); // adds label to symbol table
                goto loop_break;
            case '@' :
                a_check=1;
                break;
            case ';' :
                jump_check=1;
                break;
            case ' ' :
                if (code_check == 1)
                {
                    line_end=i;
                    goto loop_break;
                }
                break;
            default :
                if (code_check == 0)
                {
                    line_start = i;
                }
                code_check=1; // once there has been some alphanumeric (or other symbol) input on line, any subsequent space ends line
        }
    }

    loop_break: ;

    // if there is valid code, allocates space for new line in linked list
    if (code_check == 1)
    {
        code_adder(raw_code, a_check, jump_check, dest_check, line_end, line_start);
    }






    return 0;
}


// adds new label to symbol table
int label_adder(char raw_code[])
{
    bool write_check = 0;
    int char_count = 0;
    char tmp_symbol[MAX_LINE_LENGTH];

    symbol_count++;
    symboltable[symbol_count].value = line_count;

    for (int i=0; raw_code[i]!='\0'; i++)
    {
        if (raw_code[i] == ')')
        {
            tmp_symbol[char_count] = '\0';
            char_count++;
            break;
        }

        else if (write_check == 1)
        {
            tmp_symbol[char_count] = raw_code[i];
            char_count++;
        }

        else if (raw_code[i] == '(')
        {
            write_check=1;
        }
    }
    symboltable[symbol_count].symbol = malloc(char_count*sizeof(char));
    if (symboltable[symbol_count].symbol == NULL)
    {
        return 1;
    }
    strcpy(symboltable[symbol_count].symbol,tmp_symbol);

    return 0;
}

int code_adder(char raw_code[], bool a_check, bool jump_check, bool dest_check, int line_end, int line_start)
{
    node *new_line = malloc(sizeof(node));
    if (new_line == NULL)
    {
        return 1;
    }
    new_line->next = NULL;
    line_count++;

    int char_count = 0;
    int comp_char_count = 0;
    int jump_char_count = 0;

    if (a_check == 1)
    {
        new_line->a_line = 1;

        for (int i = line_start; i < line_end; i++)
        {
            new_line->address[char_count] = raw_code[i];
            char_count++;
        }

        new_line->address[char_count] = '\0';
        }

    else
    {
        if (dest_check == 1)
        {
            for (int i = line_start; i < line_end; i++)
            {
                if (raw_code[i] == '=')
                {
                    new_line->dest[char_count] = '\0';
                    char_count++;
                    break;
                }

                new_line->dest[char_count] = raw_code[i];
                char_count++;
            }
        }

        else
        {
            strcpy(new_line->dest, "null");
        }


        for (int i = line_start + char_count, j = 0; i < line_end; i++, j++)
        {
            if (raw_code[i] == ';')
            {
                char_count++;
                break;
            }

            new_line->comp[j] = raw_code[i];
            char_count++;
            comp_char_count++;
        }
        new_line->comp[comp_char_count] = '\0';


        if (jump_check == 1)
        {
            for (int i = line_start + char_count, j = 0; i < line_end; i++, j++)
            {
                new_line->jump[j] = raw_code[i];
                jump_char_count++;
            }
            new_line->jump[jump_char_count] = '\0';
        }
        else
        {
            strcpy(new_line->jump, "null");
        }


    }


    if (line_count == 1)
    {
        code_list = new_line;
        line_tail = new_line;
    }
    else
    {
        line_tail->next = new_line;
        line_tail = new_line;
    }

    return 0;
}


char* translate(node *line)
{
    char *trans_out = malloc(17*sizeof(char));

    if (line->a_line == 1)
    {
        if (isdigit(line->address[0]) == 0)
        {
            symbol_checker(line); // if first character of address is not digit, searches symboltable for the symbol and adds new variable if not found
        }

        add_to_bin(line, trans_out); //converts address to 15 bit binary
        trans_out[0] = '0'; //signifies line of A code with first bit = 0
        trans_out[16] = '\0';

    }

    else
    {
        trans_out[0] = '1', trans_out[1] = '1', trans_out[2] = '1'; //signifies line of C code with first 3 bits = 1

        for (int i = 0; i < dest_count; i++)
        {
            if (strcmp(line->dest, dest_table[i].symbol) == 0)
            {
                trans_out[10] = dest_table[i].value[0] + '0';
                trans_out[11] = dest_table[i].value[1] + '0';
                trans_out[12] = dest_table[i].value[2] + '0';
                break;
            }
        }

        for (int i = 0; i < jump_count; i++)
        {
            if (strcmp(line->dest, dest_table[i].symbol) == 0)
            {
                trans_out[13] = dest_table[i].value[0] + '0';
                trans_out[14] = dest_table[i].value[1] + '0';
                trans_out[15] = dest_table[i].value[2] + '0';
                break;
            }
        }



    }
    printf("%s\n", trans_out);
    return trans_out;

}

void symbol_checker(node *line)
{
    bool symbol_found = 0;

    for (int i = 0; i <= symbol_count; i++)
    {
        if (strcmp(line->address, symboltable[i].symbol) == 0)
        {
            char val_str[MAX_LINE_LENGTH];
            sprintf(val_str, "%ld", symboltable[i].value);
            strcpy(line->address, val_str); // if match is found in table, replaces the symbol in code with the symbol address from table
            symbol_found = 1;
        }
    }
    if (symbol_found == 0) // if symbol is not found in table, creates symbol as new variable
    {
        symbol_count++;
        symboltable[symbol_count].value = variable_count + FIRST_VARIABLE; // inputs next available variable address into symbol table
        char var_address[MAX_LINE_LENGTH];
        sprintf(var_address, "%i", variable_count + FIRST_VARIABLE); // replaces the variable in the code with the new variables address
        strcpy(line->address, var_address);

        variable_count++;
        symboltable[symbol_count].symbol = malloc(MAX_LINE_LENGTH*sizeof(char));
        strcpy(symboltable[symbol_count].symbol, line->address);  // allocates space for and adds new variable name to symboltable
    }

}


// converts the decimal address(string) to 15bit binary and puts it in the translation char array
void add_to_bin(node *in_line, char *out_line)
{
    long int_address = atoi(in_line->address);
    printf("%ld\n", int_address);

    int bin_count = 0;

    while (int_address != 0)
    {
        int mod = int_address % 2;
        out_line[15-bin_count] = mod + '0';
        int_address /= 2;
        bin_count++;
    }
    while (bin_count < 15)
    {
        out_line[15-bin_count] = '0';
        bin_count++;
    }
}


int mem_free(void)
{

    return 0;
}