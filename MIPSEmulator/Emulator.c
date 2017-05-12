#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define XSTR(x) STR(x) 
#define STR(x) #x 

#define MAX_PROG_LEN 250
#define MAX_LINE_LEN 50
#define MAX_OPCODE   8 
#define MAX_REGISTER 32 
#define MAX_ARG_LEN  20 

#define ADDR_TEXT    0x00400000 
#define TEXT_POS(a)  ((a==ADDR_TEXT)?(0):(a - ADDR_TEXT)/4)

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

unsigned int registers[MAX_REGISTER] = { 0 };
unsigned int pc = 0;
int pc_real = 0;
unsigned int text[MAX_PROG_LEN] = { 0 };

typedef int(*opcode_function)(unsigned int, unsigned int*, char*, char*, char*, char*);


char prog[MAX_PROG_LEN][MAX_LINE_LEN];
int prog_len = 0;


int print_registers() {
	int i;
	printf("registers:\n");
	for (i = 0; i<MAX_REGISTER; i++) {
		printf(" %d: %d\n", i, registers[i]);
	}
	printf(" Program Counter: 0x%08x\n", pc);
	return(0);
}

int add_imi(unsigned int *bytecode, int imi) {
	if (imi<-32768 || imi>32767) return (-1);
	*bytecode |= (0xFFFF & imi);
	return(0);
}

int add_sht(unsigned int *bytecode, int sht) {
	if (sht<0 || sht>31) return(-1);
	*bytecode |= (0x1F & sht) << 6;
	return(0);
}

int add_reg(unsigned int *bytecode, char *reg, int pos) {
	int i;
	for (i = 0; i<MAX_REGISTER; i++) {
		if (!strcmp(reg, register_str[i])) {
			*bytecode |= (i << pos);
			return(0);
		}
	}
	return(-1);
}

int add_lbl(unsigned int offset, unsigned int *bytecode, char *label) {
	char l[MAX_ARG_LEN + 1];
	int j = 0;
	while (j<prog_len) {
		memset(l, 0, MAX_ARG_LEN + 1);
		sscanf(&prog[j][0], "%" XSTR(MAX_ARG_LEN) "[^:]:", l);
		if (label != NULL && !strcmp(l, label)) return(add_imi(bytecode, j - (offset + 1)));
		j++;
	}
	return (-1);
}

int opcode_nop(unsigned int offset, unsigned int *bytecode, char *opcode, char *arg1, char *arg2, char *arg3) {
	*bytecode = 0;
	return (0);
}

int opcode_add(unsigned int offset, unsigned int *bytecode, char *opcode, char *arg1, char *arg2, char *arg3) {
	*bytecode = 0x20; 				// op,shamt,funct
	if (add_reg(bytecode, arg1, 11)<0) return (-1); 	// destination register
	if (add_reg(bytecode, arg2, 21)<0) return (-1);	// source1 register
	if (add_reg(bytecode, arg3, 16)<0) return (-1);	// source2 register
	return (0);
}

int opcode_addi(unsigned int offset, unsigned int *bytecode, char *opcode, char *arg1, char *arg2, char *arg3) {
	*bytecode = 0x20000000; 				// op
	if (add_reg(bytecode, arg1, 16)<0) return (-1);	// destination register
	if (add_reg(bytecode, arg2, 21)<0) return (-1);	// source1 register
	if (add_imi(bytecode, atoi(arg3))) return (-1);	// constant
	return (0);
}

int opcode_andi(unsigned int offset, unsigned int *bytecode, char *opcode, char *arg1, char *arg2, char *arg3) {
	*bytecode = 0x30000000; 				// op
	if (add_reg(bytecode, arg1, 16)<0) return (-1); 	// destination register
	if (add_reg(bytecode, arg2, 21)<0) return (-1);	// source1 register
	if (add_imi(bytecode, atoi(arg3))) return (-1);	// constant 
	return (0);
}

int opcode_beq(unsigned int offset, unsigned int *bytecode, char *opcode, char *arg1, char *arg2, char *arg3) {
	*bytecode = 0x10000000; 				// op
	if (add_reg(bytecode, arg1, 21)<0) return (-1);	// register1
	if (add_reg(bytecode, arg2, 16)<0) return (-1);	// register2
	if (add_lbl(offset, bytecode, arg3)) return (-1); // jump 
	return (0);
}

int opcode_bne(unsigned int offset, unsigned int *bytecode, char *opcode, char *arg1, char *arg2, char *arg3) {
	*bytecode = 0x14000000; 				// op
	if (add_reg(bytecode, arg1, 21)<0) return (-1); 	// register1
	if (add_reg(bytecode, arg2, 16)<0) return (-1);	// register2
	if (add_lbl(offset, bytecode, arg3)) return (-1); // jump 
	return (0);
}

int opcode_srl(unsigned int offset, unsigned int *bytecode, char *opcode, char *arg1, char *arg2, char *arg3) {
	*bytecode = 0x2; 					// op
	if (add_reg(bytecode, arg1, 11)<0) return (-1);   // destination register
	if (add_reg(bytecode, arg2, 16)<0) return (-1);   // source1 register
	if (add_sht(bytecode, atoi(arg3))<0) return (-1);// shift 
	return(0);
}

int opcode_sll(unsigned int offset, unsigned int *bytecode, char *opcode, char *arg1, char *arg2, char *arg3) {
	*bytecode = 0; 					// op
	if (add_reg(bytecode, arg1, 11)<0) return (-1);	// destination register
	if (add_reg(bytecode, arg2, 16)<0) return (-1); 	// source1 register
	if (add_sht(bytecode, atoi(arg3))<0) return (-1);// shift 
	return(0);
}

const char *opcode_str[] = { "nop", "add", "addi", "andi", "beq", "bne", "srl", "sll" };
opcode_function opcode_func[] = { &opcode_nop, &opcode_add, &opcode_addi, &opcode_andi, &opcode_beq, &opcode_bne, &opcode_srl, &opcode_sll };

int make_bytecode() {
	unsigned int bytecode;
	int j = 0;
	int i = 0;

	char label[MAX_ARG_LEN + 1];
	char opcode[MAX_ARG_LEN + 1];
	char arg1[MAX_ARG_LEN + 1];
	char arg2[MAX_ARG_LEN + 1];
	char arg3[MAX_ARG_LEN + 1];

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
				"s %" XSTR(MAX_ARG_LEN) "s", label, opcode, arg1, arg2, arg3) < 2) { //parse the line with label
				printf("ERROR: parsing line %d\n", j);
				return(-1);
			}
		}
		else {
			if (sscanf(&prog[j][0], "%" XSTR(MAX_ARG_LEN) "s %" XSTR(MAX_ARG_LEN) "s %" XSTR(MAX_ARG_LEN)
				"s %" XSTR(MAX_ARG_LEN) "s", opcode, arg1, arg2, arg3) < 1) { //parse the line without label
				printf("ERROR: parsing line %d\n", j);
				return(-1);
			}
		}

		for (i = 0; i<MAX_OPCODE; i++) {
			if (!strcmp(opcode, opcode_str[i]) && ((*opcode_func[i]) != NULL)) {
				if ((*opcode_func[i])(j, &bytecode, opcode, arg1, arg2, arg3)<0) {
					printf("ERROR: line %d opcode error (assembly: %s %s %s %s)\n", j, opcode, arg1, arg2, arg3);
					return(-1);
				}
				else {
					printf("0x%08x 0x%08x\n", ADDR_TEXT + 4 * j, bytecode);
					text[j] = bytecode;
					break;
				}
			}
			if (i == (MAX_OPCODE - 1)) {
				printf("ERROR: line %d unknown opcode\n", j);
				return(-1);
			}
		}
		j++;
	}

	printf("... DONE!\n");
	return(0);
}

int get_immediate(unsigned int byte) {
	int im = byte & 0x0000FFFF;
	int isNeg = byte & 0x00008000;

	if (isNeg) {
		im |= 0xFFFF0000;
	}

	return im;
}

int exec_byte(unsigned int byte) {
	if (byte == 0x00000000) {
		printf("\tNO OP");
		return 0;
	}

	// let's seperate out the opcode of the byte
	unsigned int opcode = byte & 0xFC000000;
	//printf("\tOpcode: 0x%08x\n", opcode);

	// what's the opcode?
	if (opcode == 0x00000000) {
		printf("\tR-Type ALU instruction - ");

		// what's the func code?
		unsigned int funccode = byte & 0x0000003F;

		// extract rt, rs and rd
		unsigned int rt = byte & 0x001F0000;
		unsigned int rs = byte & 0x03E00000;
		unsigned int rd = byte & 0x0000F800;

		rt = rt >> 16;
		rs = rs >> (16 + 5);
		rd = rd >> 11;

		// extract shift
		int shift = byte & 0x000007E0;

		shift = shift >> 6;

		if (funccode == 0x00000020) {
			printf("ADD\n");
			printf("\trt=%s, rs=%s, rd=%s\n", register_str[rt], register_str[rs], register_str[rd]);

			printf("\tPutting %i into register %s\n", registers[rt] + registers[rs], register_str[rd]);
			registers[rd] = registers[rt] + registers[rs];
		}
		else if (funccode == 0x00000000) {
			printf("SLL\n");
			printf("\trt=%s, rd=%s, shift=%i\n", register_str[rt], register_str[rd], shift);
			printf("\tPutting %i in register %s\n", registers[rt] << shift, register_str[rd]);

			registers[rd] = registers[rt] << shift;
		}
		else if (funccode == 0x00000002) {
			printf("SRL\n");
			printf("\trt=%s, rd=%s, shift=%i\n", register_str[rt], register_str[rd], shift);
			printf("\tPutting %i in register %s\n", registers[rt] >> shift, register_str[rd]);

			registers[rd] = registers[rt] >> shift;
		}
	}
	else if (opcode == 0x20000000) {
		printf("\tI-Type ADDI\n");

		// extract the immediate
		int i = get_immediate(byte);

		// extract rt and rs
		unsigned int rt = byte & 0x001F0000;
		unsigned int rs = byte & 0x03E00000;

		rt = rt >> 16;
		rs = rs >> (16 + 5);

		printf("\trt=%s, rs=%s\n", register_str[rt], register_str[rs]);

		// contents of rs
		int j = registers[rs];

		// fill in rt with the result
		registers[rt] = j + i;

		printf("\tPutting %i into register %s\n", j + i, register_str[rt]);
	}
	else if (opcode == 0x30000000) {
		printf("\tI-Type ANDI\n");

		// extract the immediate
		int i = get_immediate(byte);

		// extract rt and rs
		unsigned int rt = byte & 0x001F0000;
		unsigned int rs = byte & 0x03E00000;

		rt = rt >> 16;
		rs = rs >> (16 + 5);

		printf("\trt=%s, rs=%s\n", register_str[rt], register_str[rs]);

		// contents of rs
		int j = registers[rs];

		// fill in rt with the result
		registers[rt] = j & i;

		printf("\tPutting %i into register %s\n", j & i, register_str[rt]);
	}
	else if (opcode == 0x10000000) {
		printf("\tI-Type BEQ\n");

		// extract the immediate
		int jmp = get_immediate(byte);
		printf("\tJump location: %i\n", jmp);

		// extract rs and rt
		unsigned int rt = byte & 0x001F0000;
		unsigned int rs = byte & 0x03E00000;

		rt = rt >> 16;
		rs = rs >> (16 + 5);

		printf("\trt=%s, rs=%s\n", register_str[rt], register_str[rs]);

		// contents of rs and rt
		int i = registers[rt];
		int j = registers[rs];

		if (j == i) {
			printf("\tJumping by %i\n", jmp);
			return jmp;
		}
	}
	else if (opcode == 0x14000000) {
		printf("\tI-Type BNE\n");

		// extract the immediate
		int jmp = get_immediate(byte);
		printf("\tJump location: %i\n", jmp);

		// extract rs and rt
		unsigned int rt = byte & 0x001F0000;
		unsigned int rs = byte & 0x03E00000;

		rt = rt >> 16;
		rs = rs >> (16 + 5);

		printf("\trt=%s, rs=%s\n", register_str[rt], register_str[rs]);

		// contents of rs and rt
		int i = registers[rt];
		int j = registers[rs];

		if (j != i) {
			printf("\tJumping by %i\n", jmp);
			return jmp;
		}
	}

	return 0;
}

int exec_bytecode() {
	printf("EXECUTING PROGRAM ...\n");

	for (pc_real = 0; pc_real < prog_len; pc_real++) {
		pc = ADDR_TEXT + (4 * pc_real);
		unsigned int byte = text[pc_real];

		printf("Instruction %i (0x%08x): 0x%08x\n", pc_real, pc, byte);

		pc_real += exec_byte(byte);
	}

	//here goes the code to run the byte code

	print_registers(); // print out the state of registers at the end of execution

	printf("... DONE!\n");
	return(0);

}


int load_program() {
	int j = 0;
	FILE *f;

	printf("LOADING PROGRAM ...\n");

	f = fopen("prog.txt", "r");

	if (f == NULL) {
		printf("ERROR: program not found\n");
	}

	while (fgets(&prog[prog_len][0], MAX_LINE_LEN, f) != NULL) {
		prog_len++;
	}

	printf("PROGRAM:\n");
	for (j = 0; j<prog_len; j++) {
		printf("%d: %s", j, &prog[j][0]);
	}

	printf("... DONE!\n");

	return(0);
}


int main() {
	if (load_program()<0) 	return(-1);
	if (make_bytecode()<0) 	return(-1);
	if (exec_bytecode()<0) 	return(-1);

	getchar();

	return(0);
}

