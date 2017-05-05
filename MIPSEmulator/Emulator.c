#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define XSTR(x) STR(x)		//can be used for MAX_ARG_LEN in sscanf 
#define STR(x) #x 

#define MAX_PROG_LEN 250	//maximum number of lines a program can have
#define MAX_LINE_LEN 50		//maximum number of characters a line of code may have
#define MAX_OPCODE   8		//number of opcodes supported (length of opcode_str and opcode_func)
#define MAX_REGISTER 32		//number of registers (size of register_str) 
#define MAX_ARG_LEN  20		//used when tokenizing a program line, max size of a token 

#define ADDR_TEXT    0x00400000 //where the .text area starts in which the program lives 
#define TEXT_POS(a)  ((a==ADDR_TEXT)?(0):(a - ADDR_TEXT)/4) //can be used to access text[] 

const char *register_str[] = { "$zero",
	"$at",
	"$v0", "$v1",
	"$a0", "$a1", "$a2", "$a3",
	"$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
	"$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
	"$t8", "$t9",
	"$k0", "$k1",
	"$gp",
	"$sp",
	"$fp",
	"$ra"
};

typedef int(*opcode_function)(unsigned int, unsigned int*, char*, char*, char*, char*);

/* Space for the assembler program */
char prog[MAX_PROG_LEN][MAX_LINE_LEN];
int prog_len = 0;

unsigned int bytelist[MAX_PROG_LEN];
char labelpointers[MAX_PROG_LEN][MAX_ARG_LEN]; // used for lookup of labels

// getting label ids, sorting out addr/id conversions etc
unsigned int getbyteidaddress(int byteid) {
	return (unsigned int)(byteid + ADDR_TEXT);
}

int getaddressbyteid(unsigned int address) {
	return (int)(address - ADDR_TEXT);
}

int getlabelbyteid(char* label) {
	for (int i = 0; i < prog_len; i++) {
		if (strcmp(labelpointers[i], label) == 0) {
			return i;
		}
	}

	return -1;
}

// CONVERSIONS
/* function to create bytecode for instruction nop
conversion result is passed in bytecode
function always returns 0 (conversion OK) */
int opcode_nop(unsigned int offset, unsigned int *bytecode, char *opcode, char *arg1, char *arg2, char *arg3) {
	*bytecode = 0;
	return (0);
}

/* function to create bytecode for instruction add */
int opcode_add(unsigned int offset, unsigned int *bytecode, char *opcode, char *arg1, char *arg2, char *arg3) {
	// STEP 1: assign "add" func bytecode
	*bytecode = 0x20;

	// STEP 2: Translate destination register into a number
	int i;
	for (i = 0; i<MAX_REGISTER; i++) {
		if (!strcmp(arg1, register_str[i])) break;
	}

	*bytecode |= i << 11;

	// STEP 3: Translate source register 1 into a number
	for (i = 0; i<MAX_REGISTER; i++) {
		if (!strcmp(arg2, register_str[i])) break;
	}

	*bytecode |= i << 21;

	// STEP 4: Translate source register 2 into a number
	for (i = 0; i<MAX_REGISTER; i++) {
		if (!strcmp(arg3, register_str[i])) break;
	}

	*bytecode |= i << 16;

	// assign "add" opcode
	*bytecode |= 0x00 << 26;

	return (0);
}

/* function to create bytecode for instruction addi */
int opcode_addi(unsigned int offset, unsigned int *bytecode, char *opcode, char *arg1, char *arg2, char *arg3) {
	// STEP 1: place immediate in first 16 bits
	*bytecode = atoi(arg3);

	// STEP 2: Translate destination register into a number
	int i;
	for (i = 0; i<MAX_REGISTER; i++) {
		if (!strcmp(arg1, register_str[i])) break;
	}

	*bytecode |= i << 16;

	// STEP 3: Translate source register into a number
	for (i = 0; i<MAX_REGISTER; i++) {
		if (!strcmp(arg2, register_str[i])) break;
	}

	*bytecode |= i << 21;

	// assign "addi" opcode
	*bytecode |= 0x08 << 26;

	return (0);
}

/* function to create bytecode for instruction andi */
int opcode_andi(unsigned int offset, unsigned int *bytecode, char *opcode, char *arg1, char *arg2, char *arg3) {
	// STEP 1: place immediate in first 16 bits
	*bytecode = atoi(arg3);

	// STEP 2: Translate destination register into a number
	int i;
	for (i = 0; i<MAX_REGISTER; i++) {
		if (!strcmp(arg1, register_str[i])) break;
	}

	*bytecode |= i << 16;

	// STEP 3: Translate source register into a number
	for (i = 0; i<MAX_REGISTER; i++) {
		if (!strcmp(arg2, register_str[i])) break;
	}

	*bytecode |= i << 21;

	// assign "addi" opcode
	*bytecode |= 0x0C << 26;

	return (0);
}

/* function to create bytecode for instruction srl */
int opcode_srl(unsigned int offset, unsigned int *bytecode, char *opcode, char *arg1, char *arg2, char *arg3) {
	// STEP 1: assign "srl" func code
	*bytecode = 0x02;

	// STEP 2: Translate destination register into a number
	int i;
	for (i = 0; i<MAX_REGISTER; i++) {
		if (!strcmp(arg1, register_str[i])) break;
	}

	*bytecode |= i << 11;

	// STEP 3: Translate source register into a number
	for (i = 0; i<MAX_REGISTER; i++) {
		if (!strcmp(arg2, register_str[i])) break;
	}

	*bytecode |= i << 16;

	// STEP 4: Embed shift number
	*bytecode |= atoi(arg3) << 6;

	// assign "srl" opcode
	*bytecode |= 0x00 << 26;

	return (0);
}

/* function to create bytecode for instruction sll */
int opcode_sll(unsigned int offset, unsigned int *bytecode, char *opcode, char *arg1, char *arg2, char *arg3) {
	// STEP 1: assign "sll" func code
	*bytecode = 0x00;

	// STEP 2: Translate destination register into a number
	int i;
	for (i = 0; i<MAX_REGISTER; i++) {
		if (!strcmp(arg1, register_str[i])) break;
	}

	*bytecode |= i << 11;

	// STEP 3: Translate source register into a number
	for (i = 0; i<MAX_REGISTER; i++) {
		if (!strcmp(arg2, register_str[i])) break;
	}

	*bytecode |= i << 16;

	// STEP 4: Embed shift number
	*bytecode |= atoi(arg3) << 6;

	// assign "sll" opcode
	*bytecode |= 0x00 << 26;

	return (0);
}

/* function to create bytecode for instruction beq */
int opcode_beq(unsigned int offset, unsigned int *bytecode, char *opcode, char *arg1, char *arg2, char *arg3) {
	// STEP 1: place immediate in first 16 bits
	*bytecode = getbyteidaddress(getlabelbyteid(arg3));

	// STEP 2: Translate destination register into a number
	int i;
	for (i = 0; i<MAX_REGISTER; i++) {
		if (!strcmp(arg1, register_str[i])) break;
	}

	*bytecode |= i << 16;

	// STEP 3: Translate source register into a number
	for (i = 0; i<MAX_REGISTER; i++) {
		if (!strcmp(arg2, register_str[i])) break;
	}

	*bytecode |= i << 21;

	// assign "beq" opcode
	*bytecode |= 0x04 << 26;

	return (0);
}

/* function to create bytecode for instruction beq */
int opcode_bne(unsigned int offset, unsigned int *bytecode, char *opcode, char *arg1, char *arg2, char *arg3) {
	// STEP 1: place immediate in first 16 bits
	*bytecode = getbyteidaddress(getlabelbyteid(arg3));

	// STEP 2: Translate destination register into a number
	int i;
	for (i = 0; i<MAX_REGISTER; i++) {
		if (!strcmp(arg1, register_str[i])) break;
	}

	*bytecode |= i << 16;

	// STEP 3: Translate source register into a number
	for (i = 0; i<MAX_REGISTER; i++) {
		if (!strcmp(arg2, register_str[i])) break;
	}

	*bytecode |= i << 21;

	// assign "bne" opcode
	*bytecode |= 0x05 << 26;

	return (0);
}

const char *opcode_str[] = { "nop", "add", "addi", "andi", "beq", "bne", "srl", "sll" };
opcode_function opcode_func[] = { &opcode_nop, &opcode_add, &opcode_addi, &opcode_andi, &opcode_beq, &opcode_bne, &opcode_srl, &opcode_sll };

/* Elements for running the emulator */
unsigned int registers[MAX_REGISTER] = { 0 }; // the registers
unsigned int pc = 0; // the program counter
unsigned int text[MAX_PROG_LEN] = { 0 }; //the text memory with our instructions

										 /* a function to print the state of the machine */
int print_registers() {
	int i;
	printf("registers:\n");
	for (i = 0; i<MAX_REGISTER; i++) {
		printf(" %d: %d\n", i, registers[i]);
	}
	printf(" Program Counter: 0x%08x\n", pc);
	return(0);
}

/* function to execute bytecode */
int exec_bytecode() {
	printf("EXECUTING PROGRAM ...\n");
	pc = ADDR_TEXT; //set program counter to the start of our program

	for (int i = 0; i < prog_len; i++) {
		printf("Byte %i: 0x%08x\n", i, bytelist[i]);
	}

	print_registers(); // print out the state of registers at the end of execution

	printf("... DONE!\n");
	return(0);

}

/* function to create bytecode */
int make_bytecode() {
	unsigned int bytecode; // holds the bytecode for each converted program instruction 
	int j = 0; //instruction counter (equivalent to program line)

	char label[MAX_ARG_LEN + 1];
	char opcode[MAX_ARG_LEN + 1];
	char arg1[MAX_ARG_LEN + 1];
	char arg2[MAX_ARG_LEN + 1];
	char arg3[MAX_ARG_LEN + 1];

	printf("READING METADATA ...\n");
	while (j < prog_len) {
		memset(label, 0, sizeof(label));
		memset(opcode, 0, sizeof(opcode));
		memset(arg1, 0, sizeof(arg1));
		memset(arg2, 0, sizeof(arg2));
		memset(arg3, 0, sizeof(arg3));

		if (strchr(&prog[j][0], ':')) { //check if the line contains a label
			if (sscanf(&prog[j][0], "%" XSTR(MAX_ARG_LEN) "[^:]: %" XSTR(MAX_ARG_LEN) "s %" XSTR(MAX_ARG_LEN) "s %" XSTR(MAX_ARG_LEN)
				"s %" XSTR(MAX_ARG_LEN) "s", label, opcode, arg1, arg2, arg3) != 5) { //parse the line with label
				printf("parse error line %d\n", j);
				return(-1);
			}
		}
		else {
			if (sscanf(&prog[j][0], "%" XSTR(MAX_ARG_LEN) "s %" XSTR(MAX_ARG_LEN) "s %" XSTR(MAX_ARG_LEN)
				"s %" XSTR(MAX_ARG_LEN) "s", opcode, arg1, arg2, arg3) != 4) { //parse the line without label
				printf("parse error line %d\n", j);
				return(-1);
			}
		}

		strcpy(labelpointers[j], label);

		j++;
	}

	j = 0; //reset j

	printf("ASSEMBLING PROGRAM ...\n");
	while (j<prog_len) {
		memset(label, 0, sizeof(label));
		memset(opcode, 0, sizeof(opcode));
		memset(arg1, 0, sizeof(arg1));
		memset(arg2, 0, sizeof(arg2));
		memset(arg3, 0, sizeof(arg3));

		bytecode = 0;

		if (strchr(&prog[j][0], ':')) { //check if the line contains a label
			if (sscanf(&prog[j][0], "%" XSTR(MAX_ARG_LEN) "[^:]: %" XSTR(MAX_ARG_LEN) "s %" XSTR(MAX_ARG_LEN) "s %" XSTR(MAX_ARG_LEN)
				"s %" XSTR(MAX_ARG_LEN) "s", label, opcode, arg1, arg2, arg3) != 5) { //parse the line with label
				printf("parse error line %d\n", j);
				return(-1);
			}
		}
		else {
			if (sscanf(&prog[j][0], "%" XSTR(MAX_ARG_LEN) "s %" XSTR(MAX_ARG_LEN) "s %" XSTR(MAX_ARG_LEN)
				"s %" XSTR(MAX_ARG_LEN) "s", opcode, arg1, arg2, arg3) != 4) { //parse the line without label
				printf("parse error line %d\n", j);
				return(-1);
			}
		}

		// debug printout; remove in final program	
		//printf("executing with tokens: |%s|%s|%s|%s|%s|\n", label, opcode, arg1, arg2, arg3);

		// find the right function pointer for the opcode
		int i = 0;
		for (i = 0; i < MAX_OPCODE; i++) {
			if (!strcmp(opcode, opcode_str[i])) break;
		}

		if ((*opcode_func[i])(j, &bytecode, opcode, arg1, arg2, arg3)<0) {
			printf("%d: opcode error (assembly: %s %s %s %s)\n", j, opcode, arg1, arg2, arg3);
			return(-1);
		}

		// append bytecode to program
		bytelist[j] = bytecode;

		j++;

	}
	printf("... DONE!\n");
	return(0);
}

/* loading the program into memory */
int load_program() {
	int j = 0;
	FILE *f;

	printf("LOADING PROGRAM ...\n");

	f = fopen("prog.txt", "r");
	while (fgets(&prog[prog_len][0], MAX_LINE_LEN, f) != NULL) {
		prog_len++;
	}

	printf("PROGRAM:\n");
	for (j = 0; j<prog_len; j++) {
		printf("%d: %s", j, &prog[j][0]);
	}

	return(0);
}

int main() {
	if (load_program()<0) 	return(-1);
	if (make_bytecode()<0) 	return(-1);
	if (exec_bytecode()<0) 	return(-1);
	getchar();
	return(0);
}

