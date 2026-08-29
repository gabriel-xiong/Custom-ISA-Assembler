#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#define totalMemory 524288

int stillProcessing;
uint64_t registers[32];
uint64_t address;
uint8_t memory[totalMemory];

void error()
{
    fprintf(stderr, "Simulation error\n");
    exit(1);
}

void extractInformation(uint32_t instruction, uint32_t *rd, uint32_t *rs, uint32_t *rt, uint32_t *L) {
    *rd = (instruction >> 22) & 0x1F;
    *rs = (instruction >> 17) & 0x1F;
    *rt = (instruction >> 12) & 0x1F;
    *L  = instruction & 0xFFF;

}

int64_t getSignedVersion(uint32_t val) {
    if (val & 0x800)  // if bit 11 is set
        return (int64_t)(val | 0xFFFFFFFFFFFFF000ULL); // fill first 52 bits with zeros
    else
        return (int64_t)val;
}

void processAnd(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);
    registers[rd] = registers[rs] & registers[rt];
    address += 4;
}

void processOr(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);
    registers[rd] = registers[rs] | registers[rt];
    address += 4;
}

void processXor(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);
    registers[rd] = registers[rs] ^ registers[rt];
    address += 4;
}

void processNot(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);
    registers[rd] = ~ registers[rs];
    address += 4;
}

void processShftr(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);
    registers[rd] = registers[rs] >> registers[rt];
    address += 4;
}

void processShftri(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);
    registers[rd] = registers[rd] >> literal;
    address += 4;
}

void processShftl(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);
    registers[rd] = registers[rs] << registers[rt];
    address += 4;
}

void processShftli(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);
    registers[rd] = registers[rd] << literal;
    address += 4;
}

void processBr(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);
    address = registers[rd];
}

void processBrr(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);
    address = address + registers[rd];
}

void processBrrLiteral(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);
    address = address + getSignedVersion(literal);
}

void processBrnz(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);
    if (registers[rs] == 0)
        address +=4;
    else
        address = registers[rd];
}

void processCall(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);

    uint64_t stack = registers[31] - 8; uint64_t returnAddy = address + 4;
    if (stack + 8 > totalMemory)
        error();

    for (int i = 0; i < 8; i++)
        memory[stack + i] = (uint8_t) (returnAddy >> (i*8));

    address = registers[rd];
}

void processReturn(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);

    uint64_t stack = registers[31] - 8; uint64_t returnAddy = 0;
    if (stack + 8 > totalMemory)
        error();

     for (int i = 0; i < 8; i++)
        returnAddy |= (uint64_t)memory[stack + i] << (i * 8);
    address = returnAddy;
}

void processBrgt(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);
    if ((int64_t) registers[rs] <= (int64_t) registers[rt]) {
        address += 4;
    } else {
        address = registers[rd];
    }
}

void processPriv(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);
    if (literal == 0x0)
        stillProcessing = 0;
    else if (literal == 0x3)
        {
            if (registers[rs] != 0)
            {
                address += 4;
                return;
            }

            char check[64];
            if (scanf("%63s", check) != 1)
                error();

            for (int i = 0; check[i]; i++) {
                 if (check[i] < '0' || check[i] > '9')
                    error();
            }
            errno = 0;
            char *end;
            unsigned long long input = strtoull(check, &end, 10);
            if (errno == ERANGE || *end != '\0')
                 error();

            registers[rd] = (uint64_t)input;
            address += 4;

        }
    else if (literal == 0x4)
    {

        if (registers[rd] == 1)
        {
           printf("%" PRIu64 "\n", registers[rs]);
        }
        else if (registers[rd] == 3)
           printf("%c", (char)registers[rs]);


        address +=4;

    }
    else
    {
        error();
    }
}

void processMovLoad(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);
    uint64_t value = 0;
    // check for underflow
    int64_t signedAdd = (int64_t)registers[rs] + getSignedVersion(literal);
    if (signedAdd + 8 > totalMemory || signedAdd < 0)
        error();

    uint64_t newAdd = (int64_t) signedAdd;



    for (int i = 0; i < 8; i++)
        value |= (uint64_t) memory[newAdd + i] << (i*8);

    registers[rd] = value;
    address += 4;
}

void processMovRead(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);
    registers[rd] = registers[rs];
    address += 4;
}

void processMovLiteral(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);
    registers[rd] = (registers[rd] & 0xFFFFFFFFFFFFF000ULL) | (literal & 0xFFF);
    address += 4;
}

void processMovWrite(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);

    // check for underflow
    int64_t signedAdd = (int64_t)registers[rd] + getSignedVersion(literal);
    if (signedAdd + 8 > totalMemory || signedAdd < 0)
        error();

    uint64_t newAdd = (int64_t) signedAdd;
    if (newAdd + 8 > totalMemory)
        error();

    for (int i = 0; i < 8; i++)
        memory[newAdd + i] = (uint8_t) (registers[rs] >> (i*8));

    address += 4;
}

void processAddf(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);
    double x, y;

    memcpy(&x, &registers[rs], sizeof(double));
    memcpy(&y, &registers[rt], sizeof(double));

    double result = x + y;
    memcpy(&registers[rd], &result, sizeof(double));
    address += 4;
}

void processSubf(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);

    double x, y;

    memcpy(&x, &registers[rs], sizeof(double));
    memcpy(&y, &registers[rt], sizeof(double));

    double result = x - y;
    memcpy(&registers[rd], &result, sizeof(double));
    address += 4;
}

void processMulf(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);

    double x, y;

    memcpy(&x, &registers[rs], sizeof(double));
    memcpy(&y, &registers[rt], sizeof(double));

    double result = x * y;
    memcpy(&registers[rd], &result, sizeof(double));

    address += 4;
}

void processDivf(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);

    double x, y;

    memcpy(&x, &registers[rs], sizeof(double));
    memcpy(&y, &registers[rt], sizeof(double));

    if (y == 0)
        error();
    double result = x / y;
    memcpy(&registers[rd], &result, sizeof(double));

    address += 4;
}

void processAdd(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);
    registers[rd] = registers[rs] + registers[rt];
    address += 4;
}

void processAddi(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);
    registers[rd] = registers[rd] + literal;
    address += 4;
}

void processSub(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);
    registers[rd] = registers[rs] - registers[rt];
    address += 4;
}

void processSubi(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);
    registers[rd] = registers[rd] - literal;
    address += 4;
}

void processMul(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);
    registers[rd] = (uint64_t)((int64_t)registers[rs] * (int64_t)registers[rt]);
    address += 4;
}

void processDiv(uint32_t instruction) {
    uint32_t rd, rs, rt, literal;
    extractInformation(instruction, &rd, &rs, &rt, &literal);

    if ((int64_t)registers[rt] == 0)
        error();
    registers[rd] = (uint64_t)((int64_t)registers[rs] / (int64_t)registers[rt]);
    address += 4;
}


void (*instructionList[32])(uint32_t) = {
    processAnd,
    processOr,
    processXor,
    processNot,
    processShftr,
    processShftri,
    processShftl,
    processShftli,
    processBr,
    processBrr,
    processBrrLiteral,
    processBrnz,
    processCall,
    processReturn,
    processBrgt,
    processPriv,
    processMovLoad,
    processMovRead,
    processMovLiteral,
    processMovWrite,
    processAddf,
    processSubf,
    processMulf,
    processDivf,
    processAdd,
    processAddi,
    processSub,
    processSubi,
    processMul,
    processDiv,
    NULL,/* placeholders for instructions 31/32*/
    NULL,
};





void validateOperation(uint32_t operationCode)
{
    if (operationCode >= 32 || instructionList[operationCode] == NULL)
        error();
}

void getInstruction(uint32_t *instruction, uint64_t addy)
{
    if (totalMemory - 4 < addy)
        error();
    *instruction = 0;
    for (int i = 0; i < 4; i++)
        *instruction |= (uint32_t) memory[addy + i] << (8*i); // shift bits into instruction
}

void getOperation(uint32_t* operation, uint32_t instruction)
{
    *operation = (instruction >> 27) & 0x1F;
}

void validateMemory(uint64_t code_start, uint64_t code_size, uint64_t data_start, uint64_t data_size)
{
    if (data_start < code_start + code_size || code_start + code_size > totalMemory || data_start +
    data_size > totalMemory)
        error();
}


int main(int argc, char * argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Invalid tinker filepath\n");
        exit(1);
    }

    FILE *filePath = fopen(argv[1], "rb");
    if (!filePath) {
        fprintf(stderr, "Invalid tinker filepath\n");
        exit(1);
    }

    // Start every program with cleared registers and memory.
    memset(registers, 0, sizeof(registers));
    memset(memory, 0, totalMemory);

    // initialize other values
     stillProcessing = 1;
    registers[31] = totalMemory;

    uint64_t head[5];

    // read header
    if (fread(head, sizeof(uint64_t), 5, filePath) != 5)
        error();

    uint64_t code_start = head[1];
    uint64_t code_size  = head[2];
    uint64_t data_start = head[3];
    uint64_t data_size  = head[4];

    validateMemory(code_start, code_size, data_start, data_size);

    if (fread(memory + code_start, 1, code_size, filePath) != code_size)
        error();
    if (fread(memory + data_start, 1, data_size, filePath) != data_size)
        error();

    address = code_start;

    fclose(filePath);

    while (stillProcessing)
    {
        uint32_t instruction; uint32_t operationCode;
        getInstruction(&instruction, address);
        getOperation(&operationCode, instruction);
        validateOperation(operationCode);
        instructionList[operationCode](instruction);

    }



    return 0;
}
