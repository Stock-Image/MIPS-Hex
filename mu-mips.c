#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <ctype.h>

#include "mu-mips.h"

/***************************************************************/
/* Print out a list of commands available                                                                  */
/***************************************************************/
void help() {        
	printf("------------------------------------------------------------------\n\n");
	printf("\t**********MU-MIPS Help MENU**********\n\n");
	printf("sim\t-- simulate program to completion \n");
	printf("run <n>\t-- simulate program for <n> instructions\n");
	printf("rdump\t-- dump register values\n");
	printf("reset\t-- clears all registers/memory and re-loads the program\n");
	printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
	printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
	printf("high <val>\t-- set the HI register to <val>\n");
	printf("low <val>\t-- set the LO register to <val>\n");
	printf("print\t-- print the program loaded into memory\n");
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory                                                                            */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) &&  ( address <= MEM_REGIONS[i].end) ) {
			uint32_t offset = address - MEM_REGIONS[i].begin;
			return (MEM_REGIONS[i].mem[offset+3] << 24) |
					(MEM_REGIONS[i].mem[offset+2] << 16) |
					(MEM_REGIONS[i].mem[offset+1] <<  8) |
					(MEM_REGIONS[i].mem[offset+0] <<  0);
		}
	}
	return 0;
}

/***************************************************************/
/* Write a 32-bit word to memory                                                                                */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
	int i;
	uint32_t offset;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) && (address <= MEM_REGIONS[i].end) ) {
			offset = address - MEM_REGIONS[i].begin;

			MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
			MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
			MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
			MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
		}
	}
}

/***************************************************************/
/* Execute one cycle                                                                                                              */
/***************************************************************/
void cycle() {                                                
	handle_instruction();
	CURRENT_STATE = NEXT_STATE;
	INSTRUCTION_COUNT++;
}

/***************************************************************/
/* Simulate MIPS for n cycles                                                                                       */
/***************************************************************/
void run(int num_cycles) {                                      
	
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped\n\n");
		return;
	}

	printf("Running simulator for %d cycles...\n\n", num_cycles);
	int i;
	for (i = 0; i < num_cycles; i++) {
		if (RUN_FLAG == FALSE) {
			printf("Simulation Stopped.\n\n");
			break;
		}
		cycle();
	}
}

/***************************************************************/
/* simulate to completion                                                                                               */
/***************************************************************/
void runAll() {                                                     
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped.\n\n");
		return;
	}

	printf("Simulation Started...\n\n");
	while (RUN_FLAG){
		cycle();
	}
	printf("Simulation Finished.\n\n");
}

/***************************************************************/ 
/* Dump a word-aligned region of memory to the terminal                              */
/***************************************************************/
void mdump(uint32_t start, uint32_t stop) {          
	uint32_t address;

	printf("-------------------------------------------------------------\n");
	printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
	printf("-------------------------------------------------------------\n");
	printf("\t[Address in Hex (Dec) ]\t[Value]\n");
	for (address = start; address <= stop; address += 4){
		printf("\t0x%08x (%d) :\t0x%08x\n", address, address, mem_read_32(address));
	}
	printf("\n");
}

/***************************************************************/
/* Dump current values of registers to the teminal                                              */   
/***************************************************************/
void rdump() {                               
	int i; 
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
	printf("PC\t: 0x%08x\n", CURRENT_STATE.PC);
	printf("-------------------------------------\n");
	printf("[Register]\t[Value]\n");
	printf("-------------------------------------\n");
	for (i = 0; i < MIPS_REGS; i++){
		printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
	}
	printf("-------------------------------------\n");
	printf("[HI]\t: 0x%08x\n", CURRENT_STATE.HI);
	printf("[LO]\t: 0x%08x\n", CURRENT_STATE.LO);
	printf("-------------------------------------\n");
}

/***************************************************************/
/* Read a command from standard input.                                                               */  
/***************************************************************/
void handle_command() {                         
	char buffer[20];
	uint32_t start, stop, cycles;
	uint32_t register_no;
	int register_value;
	int hi_reg_value, lo_reg_value;

	printf("MU-MIPS SIM:> ");

	if (scanf("%s", buffer) == EOF){
		exit(0);
	}

	switch(buffer[0]) {
		case 'S':
		case 's':
			runAll(); 
			break;
		case 'M':
		case 'm':
			if (scanf("%x %x", &start, &stop) != 2){
				break;
			}
			mdump(start, stop);
			break;
		case '?':
			help();
			break;
		case 'Q':
		case 'q':
			printf("**************************\n");
			printf("Exiting MU-MIPS! Good Bye...\n");
			printf("**************************\n");
			exit(0);
		case 'R':
		case 'r':
			if (buffer[1] == 'd' || buffer[1] == 'D'){
				rdump();
			}else if(buffer[1] == 'e' || buffer[1] == 'E'){
				reset();
			}
			else {
				if (scanf("%d", &cycles) != 1) {
					break;
				}
				run(cycles);
			}
			break;
		case 'I':
		case 'i':
			if (scanf("%u %i", &register_no, &register_value) != 2){
				break;
			}
			CURRENT_STATE.REGS[register_no] = register_value;
			NEXT_STATE.REGS[register_no] = register_value;
			break;
		case 'H':
		case 'h':
			if (scanf("%i", &hi_reg_value) != 1){
				break;
			}
			CURRENT_STATE.HI = hi_reg_value; 
			NEXT_STATE.HI = hi_reg_value; 
			break;
		case 'L':
		case 'l':
			if (scanf("%i", &lo_reg_value) != 1){
				break;
			}
			CURRENT_STATE.LO = lo_reg_value;
			NEXT_STATE.LO = lo_reg_value;
			break;
		case 'P':
		case 'p':
			print_program(); 
			break;
		default:
			printf("Invalid Command.\n");
			break;
	}
}

/***************************************************************/
/* reset registers/memory and reload program                                                    */
/***************************************************************/
void reset() {   
	int i;
	/*reset registers*/
	for (i = 0; i < MIPS_REGS; i++){
		CURRENT_STATE.REGS[i] = 0;
	}
	CURRENT_STATE.HI = 0;
	CURRENT_STATE.LO = 0;
	
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
	
	/*load program*/
	load_program();
	
	/*reset PC*/
	INSTRUCTION_COUNT = 0;
	CURRENT_STATE.PC =  MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/***************************************************************/
/* Allocate and set memory to zero                                                                            */
/***************************************************************/
void init_memory() {                                           
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		MEM_REGIONS[i].mem = malloc(region_size);
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
}

/**************************************************************/
/* load program into memory                                                                                      */
/**************************************************************/
void load_program() {                   
	FILE * fp;
	char indata[100];
	char* temp;
	int i,j,word;
	uint32_t address;
	uint32_t bin;

	/* Open program file. */
	fp = fopen(prog_file, "r");
	if (fp == NULL) {
		printf("Error: Can't open program file %s\n", prog_file);
		exit(-1);
	}

	/* Read in the program. */
	word = 0;
	i = 0;
	while( fscanf(fp, "%[^\n]s\n", indata) != EOF ) {
		printf("read in string: %s\n",indata);
		temp = strtok (indata," ,.-");
		bin = find_mips(temp);
		printf("writing 0x%08x\n",bin);
		while(temp != NULL)
		{
			printf("read in string: %s\n",temp);
			temp = strtok (NULL, " ,.-");
			find_reg
		}
		for(int k=0; k < 10; k++) indata[k] = 0;	//reset the array
		address = MEM_TEXT_BEGIN + i;
		mem_write_32(address, word);
//		printf("writing 0x%08x into address 0x%08x (%d)\n", word, address, address);
		i += 4;
	}
	PROGRAM_SIZE = i/4;
	printf("Program loaded into memory.\n%d words written into memory.\n\n", PROGRAM_SIZE);
	fclose(fp);
}

/************************************************************/
/* decode and execute instruction                                                                     */ 
/************************************************************/
void handle_instruction()
{
	/*IMPLEMENT THIS*/
	/* execute one instruction at a time. Use/update CURRENT_STATE and and NEXT_STATE, as necessary.*/




	/*
	 * Read in mips values and send them to the proper functions to executed*/


}

/*
 *Reads in the mips functions*/
uint32_t find_mips(char* word)
{
	uint32_t inst = 0;
	for(int i=0; i < strlen(word); i++) word[i] = tolower(word[i]);
	if(strcmp("add",word) == 0)
	{
		inst |= (0b000000 << 26);
		inst |= (0b100000 << 5);
		inst = R_type(inst);
	}
	if(strcmp("addu",word) == 0)
	{
		inst |= (0b000000 << 26);
		inst |= (0b010101 << 5);
	}
	if(strcmp("addi",word) == 0)
	{
		inst |= (0b001000 << 5);
	}
	if(strcmp("addiu",word) == 0)
	{
		inst |= (0b001001 << 5);
	}
	if(strcmp("sub",word) == 0)
	{
		inst |= (0b000000 << 26);
		inst |= (0b010110 << 5);
	}
	if(strcmp("subu",word) == 0)
	{
		inst |= (0b000000 << 26);
		inst |= (0b010111 << 5);
	}
	if(strcmp("mult",word) == 0)
	{
		inst |= (0b000000 << 5);
		inst |= (0b010010 << 5);
	}
	if(strcmp("multu",word) == 0)
	{
		inst |= (0b000000 << 26);
		inst |= (0b010011 << 5);
	}
	if(strcmp("div",word) == 0)
	{
		inst |= (0b000000 << 26);
		inst |= (0b011010 << 5);
	}
	if(strcmp("divu",word) == 0)
	{
		inst = (inst << 26) | 0b000000;
		inst = (inst << 5) | 0b011011;
	}
	if(strcmp("and",word) == 0)
	{
		inst = (inst << 26) | 0b000000;
		inst = (inst << 5) | 0b100100;
	}
	if(strcmp("andi",word) == 0)
	{
		inst = (inst << 26) | 0b001100;
	}
	if(strcmp("or",word) == 0)
	{
		inst = (inst << 26) | 0b000000;
		inst = (inst << 5) | 0b100101;
	}
	if(strcmp("ori",word) == 0)
	{
		inst = (inst << 26) | 0b001101;
	}
	if(strcmp("xor",word) == 0)
	{
		inst = (inst << 26) | 0b000000;
		inst = (inst << 5) | 0b100110;
	}
	if(strcmp("xori",word) == 0)
	{
		inst = (inst << 26) | 0b001110;
	}
	if(strcmp("nor",word) == 0)
	{
		inst = (inst << 26) | 0b000000;
		inst = (inst << 5) | 0b100111;
	}
	if(strcmp("slt",word) == 0)
	{
		inst = (inst << 26) | 0b000000;
		inst =(inst << 5) | 0b101010;
	}
	if(strcmp("slti",word) == 0)
	{
		inst = (inst << 26) | 0b001010;
	}
	if(strcmp("sll",word) == 0)
	{
		inst = (inst << 26) | 0b000000;
		inst = (inst << 5) | 0b000000;
	}
	if(strcmp("srl",word) == 0)
	{
		inst = (inst << 26) | 0b000000;
		inst = (inst << 5) | 0b000010;
	}
	if(strcmp("sra",word) == 0)
	{
		inst = (inst << 5) | 0b000011;
	}
	if(strcmp("lw",word) == 0)
	{
		inst = (inst << 26) | 0b100011;
	}
	if(strcmp("lb",word) == 0)
	{
		inst = (inst << 26) | 0b100000;
	}
	if(strcmp("lh",word) == 0)
	{
		inst = (inst << 26) | 0b100001;
	}
	if(strcmp("lui",word) == 0)
	{
		inst = (inst << 26) | 0b001111;
	}
	if(strcmp("sw",word) == 0)
	{
		inst = (inst << 26) | 0b101011;
	}
	if(strcmp("sb",word) == 0)
	{
		inst = (inst << 26) | 0b101000;
	}
	if(strcmp("sh",word) == 0)
	{
		inst = (inst << 26) | 0b101001;
	}
	if(strcmp("mfhi",word) == 0)
	{
		inst = (inst << 26) | 0b000000;
		inst = (inst << 5) | 0b001010;
	}
	if(strcmp("mflo",word) == 0)
	{
		inst = (inst << 26) | 0b000000;
		inst = (inst << 5) | 0b001100;
	}
	if(strcmp("mthi",word) == 0)
	{
		inst = (inst << 26) | 0b000000;
		inst = (inst << 5) | 0b010001;
	}
	if(strcmp("mtlo",word) == 0)
	{
		inst = (inst << 26) | 0b000000;
		inst = (inst << 5) | 0b010011;
	}
	if(strcmp("beq",word) == 0)
	{
		inst = (inst << 26) | 0b000100;
	}
	if(strcmp("bne",word) == 0)
	{
		inst = (inst << 26) | 0b000101;
	}
	if(strcmp("blez",word) == 0)
	{
		inst = (inst << 26) | 0b000110;
	}
	if(strcmp("bltz",word) == 0);
	if(strcmp("bgez",word) == 0);
	if(strcmp("bgtz",word) == 0)
	{
		inst = (inst << 26) | 0b000111;
	}
	if(strcmp("j",word) == 0)
	{
		inst = (inst << 26) | 0b10;
	}
	if(strcmp("jr",word) == 0)
	{
		inst = (inst << 26) | 0b000000;
		inst = (inst << 5) | 0b001000;
	}
	if(strcmp("jal",word) == 0)
	{
		inst = (inst << 26) | 0b000011;
	}
	if(strcmp("jalr",word) == 0);
	return inst;
}

uint32_t find_reg(char* word)
{
	if(strcmp("$zero",word)) return 0;
	if(strcmp("$at",word)) return 1;
	if(strcmp("$v0",word)) return 2;
	if(strcmp("$v1",word)) return 3;
	if(strcmp("$a0",word)) return 4;
	if(strcmp("$a1",word)) return 5;
	if(strcmp("$a2",word)) return 6;
	if(strcmp("$a3",word)) return 7;
	if(strcmp("$t0",word)) return 8;
	if(strcmp("$t1",word)) return 9;
	if(strcmp("$t2",word)) return 10;
	if(strcmp("$t3",word)) return 11;
	if(strcmp("$t4",word)) return 12;
	if(strcmp("$t5",word)) return 13;
	if(strcmp("$t6",word)) return 14;
	if(strcmp("$t7",word)) return 15;
	if(strcmp("$s0",word)) return 16;
	if(strcmp("$s1",word)) return 17;
	if(strcmp("$s2",word)) return 18;
	if(strcmp("$s3",word)) return 19;
	if(strcmp("$s4",word)) return 20;
	if(strcmp("$s5",word)) return 21;
	if(strcmp("$s6",word)) return 22;
	if(strcmp("$s7",word)) return 23;
	if(strcmp("$t8",word)) return 24;
	if(strcmp("$t9",word)) return 25;
	if(strcmp("$k0",word)) return 26;
	if(strcmp("$k1",word)) return 27;
	if(strcmp("$gp",word)) return 28;
	if(strcmp("$sp",word)) return 29;
	if(strcmp("$fp",word)) return 30;
	if(strcmp("$ra",word)) return 31;
	return 0;
}

uint32_t R_type()
{

}
uint32_t I_type()
{

}
uint32_t J_type()
{

}
uint32_t FR_type()
{

}
uint32_t FI_type()
{

}

/************************************************************/
/* Initialize Memory                                                                                                    */ 
/************************************************************/
void initialize() { 
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in MIPS assembly format)    */ 
/************************************************************/
void print_program(){
	int i;
	uint32_t addr;
	
	for(i=0; i<PROGRAM_SIZE; i++){
		addr = MEM_TEXT_BEGIN + (i*4);
		printf("[0x%x]\t", addr);
		print_instruction(addr);
	}
}

/************************************************************/
/* Print the instruction at given memory address (in MIPS assembly format)    */
/************************************************************/
void print_instruction(uint32_t addr){
	/*IMPLEMENT THIS*/
}

/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {                              
	printf("\n**************************\n");
	printf("Welcome to MU-MIPS SIM...\n");
	printf("**************************\n\n");
	
	if (argc < 2) {
		printf("Error: You should provide input file.\nUsage: %s <input program> \n\n",  argv[0]);
		exit(1);
	}

	strcpy(prog_file, argv[1]);
	initialize();
	load_program();
	help();
	while (1){
		handle_command();
	}
	return 0;
}
