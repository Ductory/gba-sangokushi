#include <stdio.h>
#include <string.h>
#include <stdint.h>

enum CBA_TYPE {
	CBA_CRC,
	CBA_MASTER_CODE,
	CBA_8_BIT_WRITE,
	CBA_OR,
	CBA_SLIDE_CODE,
	CBA_SUPER,
	CBA_AND,
	CBA_IF_TRUE,
	CBA_16_BIT_WRITE,
	CBA_IF_FALSE,
	CBA_IF_GT,
	CBA_IF_LT,
	CBA_IF_KEYS_PRESSED,
	CBA_ADD,
	CBA_IF_AND
};

static uint32_t address;

void build_CBA(uint32_t type, uint32_t addr, uint32_t val)
{
	printf("%X%07X %04X\n", type, addr ? addr : address, val);
	address = 0;
}

void parse_CBA(FILE *fp)
{
	#define SWITCH(x) while (1)
	#define CASE(s) else if (strcmp(ins, s) == 0)
	char ins[8];
	uint32_t addr, val;
	SWITCH(ins) {
		if (fscanf(fp, "%7s", ins) == EOF) break;
		if (0) ;
		CASE("hook") {
			if (fscanf(fp, "%X", &addr) != 1) {
				fputs("invalid. format: hook AAAAAAA\n", stderr);
				break;
			}
			build_CBA(CBA_MASTER_CODE, addr, 0);
		}
		CASE("mov8") {
			if (fscanf(fp, " [%X ] ,%X", &addr, &val) != 2) {
				fputs("invalid. format: mov8 [AAAAAAA], XX\n", stderr);
				break;
			}
			build_CBA(CBA_8_BIT_WRITE, addr, val);
		}
		CASE("mov16") {
			if (fscanf(fp, " [%X ] ,%X", &addr, &val) != 2) {
				fputs("invalid. format: mov16 [AAAAAAA], XXXX\n", stderr);
				break;
			}
			build_CBA(CBA_16_BIT_WRITE, addr, val);
		}
		CASE("and") {
			if (fscanf(fp, " [%X ] ,%X", &addr, &val) != 2) {
				fputs("invalid. format: and [AAAAAAA], XXXX\n", stderr);
				break;
			}
			build_CBA(CBA_AND, addr, val);
		}
		CASE("or") {
			if (fscanf(fp, " [%X ] ,%X", &addr, &val) != 2) {
				fputs("invalid. format: or [AAAAAAA], XXXX\n", stderr);
				break;
			}
			build_CBA(CBA_OR, addr, val);
		}
		CASE("add") {
			if (fscanf(fp, " [%X ] ,%X", &addr, &val) != 2) {
				fputs("invalid. format: add [AAAAAAA], XXXX\n", stderr);
				break;
			}
			build_CBA(CBA_ADD, addr, val);
		}
		CASE("for") {
			uint32_t count, ainc, vinc;
			if (fscanf(fp, "%X : [%X +%X] =%X +%X", &count,
				&addr, &ainc, &val, &vinc) != 5) {
				fputs("invalid. format: for CCCC : [AAAAAAA + IIII] = XXXX + YYYY\n", stderr);
				break;
			}
			build_CBA(CBA_SLIDE_CODE, addr, val);
			printf("%04X%04X %04X\n", vinc, count, ainc);
		}
		CASE("rep") {
			uint8_t b[6];
			if (fscanf(fp, "%X : [%X ] =", &val, &addr) != 2) {
				fputs("invalid. format: rep CCCC : [AAAAAAA] = XX ... .\n", stderr);
				break;
			}
			build_CBA(CBA_SUPER, addr, val >> 1);
			uint32_t i = 0;
			for (; fscanf(fp, "%hhX", &b[i]) == 1; ++i) {
				if (i == 5)
					printf("%02X%02X%02X%02X %02X%02X\n", b[0], b[1], b[2], b[3], b[4], b[5]);
			}
			fscanf(fp, " .");
			for (; i < 6; ++i)
				b[i] = 0;
			printf("%02X%02X%02X%02X %02X%02X\n", b[0], b[1], b[2], b[3], b[4], b[5]);
		}
		CASE("if") {
			uint8_t op, f = 1;
			uint32_t type = -1;
			do {
				if (fscanf(fp, " %c", &op) != 1) break;
				if (op == '[') {
					if (fscanf(fp, "%X ] %c", &addr, &op) != 2) break;
					switch (op) {
						case '=': type = CBA_IF_TRUE; break;
						case '!': type = CBA_IF_FALSE; fscanf(fp, "="); break;
						case '>': type = CBA_IF_GT; break;
						case '<': type = CBA_IF_LT; break;
						case '&': type = CBA_IF_AND; break;
					}
					if (type == -1) break;
					if (fscanf(fp, "%X", &val) != 1) break;
					build_CBA(type, addr, val);
					f = 0;
				} else if (op == 'k') {
					fscanf(fp, " ey");
					if (fscanf(fp, " %c", &op) != 1) break;
					switch (op) {
						case '=': type = 0; break;
						case '&': type = 1; break;
						case '!': type = 2; fscanf(fp, "&"); break;
					}
					if (type == -1) break;
					if (fscanf(fp, "%X", &val) != 1) break;
					printf("%X00000%X0 %04X\n", CBA_IF_KEYS_PRESSED, type, val);
					f = 0;
				}
			} while (0);
			if (f) {
				fputs("invalid. format:\n"
					  "  if [AAAAAAA] op XXXX    op: =,!=,>,<,&\n"
					  "  if key op XXXX          op: =,&,!&\n", stderr);
				break;
			}
		}
		CASE("calc") {
			uint32_t offset, idx;
			if (fscanf(fp, "%X%X%X%X", &addr, &val, &offset, &idx) != 4) {
				fputs("invalid. format: calc AAAAAAA XXXX YYYY ZZZZ\n", stderr);
				break;
			}
			address = addr + val * idx + offset;
		}
		else {
			fputs("invalid instruction\n", stderr);
			break;
		}
	}
	#undef SWITCH
	#undef CASE
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		puts("usage: gen_cba infile");
		return 1;
	}
	FILE *fp;
	if (strcmp(argv[1], "-") == 0)
		fp = stdin;
	else
		fp = fopen(argv[1], "r");
	parse_CBA(fp);
	if (fp != stdin)
		fclose(fp);
	return 0;
}
