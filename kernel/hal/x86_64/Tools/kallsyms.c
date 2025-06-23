#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct symbol_entry 
{
	unsigned long address;
	char tp;
	char *symbol;
	int  symbol_length;
};

struct symbol_entry *table;
int size, count;
unsigned long _text, _etext;

int read_symbol(FILE *filp,struct symbol_entry *sym_entry)
{
	char string[100];
	int rc;

	rc = fscanf(filp,"%lx %c %499s\n",&sym_entry->address,&sym_entry->tp, string);
	if(rc != 3) 
	{
		if (rc != EOF)
		{
			fgets(string,100,filp);
		}
		return -1;
	}

	sym_entry->symbol = strdup(string);
	sym_entry->symbol_length = strlen(string)+1;
	return 0;
}

void read_map(FILE *filp)
{
	int i;

	while(!feof(filp)) 
	{
		if(count >= size) 
		{
			size += 100;
			table = realloc(table,sizeof(*table) * size);
		}
		if(read_symbol(filp,&table[count]) == 0)
			count++;
	}
	for(i = 0;i < count;i++) 
	{
		if(strcmp(table[i].symbol,"mm_symbol_text") == 0)
			_text = table[i].address;
		if(strcmp(table[i].symbol,"mm_symbol_etext") == 0)
			_etext = table[i].address;
	}
}

int symbol_valid(struct symbol_entry *sym_entry)
{
	if((sym_entry->address < _text || sym_entry->address > _etext))
		return 0;
	return 1;
}

void write_src(void)
{
	unsigned long last_addr;
	int i,valid = 0;
	long position = 0;

	printf(".section .rodata\n\n");
	printf(".globl dbg_kallsyms_addr\n");
	printf(".align 8\n\n");
	printf("dbg_kallsyms_addr:\n");
	for(i = 0,last_addr = 0;i < count;i++) 
	{
		if(!symbol_valid(&table[i]))
			continue;		
		if(table[i].address == last_addr)
			continue;

		printf("\t.quad\t%#lx\n",table[i].address);
		valid++;
		last_addr = table[i].address;
	}
	putchar('\n');

	printf(".globl dbg_kallsyms_syms_num\n");
	printf(".align 8\n\n");
	printf("dbg_kallsyms_syms_num:\n");
	printf("\t.quad\t%d\n",valid);
	putchar('\n');

	printf(".globl dbg_kallsyms_idx\n");
	printf(".align 8\n\n");
	printf("dbg_kallsyms_idx:\n");
	for(i = 0,last_addr = 0;i < count;i++) 
	{
		if(!symbol_valid(&table[i]))
			continue;		
		if(table[i].address == last_addr)
			continue;

		printf("\t.quad\t%ld\n",position);
		position += table[i].symbol_length;
		last_addr = table[i].address;
	}
	putchar('\n');

	printf(".globl dbg_kallsyms_names\n");
	printf(".align 8\n\n");
	printf("dbg_kallsyms_names:\n");
	for(i = 0,last_addr = 0;i < count;i++) 
	{
		if(!symbol_valid(&table[i]))
			continue;		
		if(table[i].address == last_addr)
			continue;

		printf("\t.asciz\t\"%s\"\n",table[i].symbol);
		last_addr = table[i].address;
	}
	putchar('\n');
}

int main(int argc,char **argv)
{
	read_map(stdin);
	write_src();
	return 0;
}