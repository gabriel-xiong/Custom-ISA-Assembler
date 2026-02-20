
#define main hw5-asm_main
#include "hw5-asm.c"
#undef main

#include <assert.h>

static int tests_run = 0;
static int tests_passed = 0;


#define test(name) do { \
    tests_run++; \
    printf("  %-35s", #name); \
    name(); \
    tests_passed++; \
    printf("PASS\n"); \
} while(0)

// reset everything before next iteration 
void reset() {
    memset(registers, 0, sizeof(registers));
    memset(memory, 0, totalMemory);
    registers[31] = totalMemory;
    address = 0x1000;
    stillProcessing = 1;
}

// encode instructions easily 
uint32_t encode(uint32_t op, uint32_t rd, uint32_t rs, uint32_t rt, uint32_t L) {
    return (op << 27) | (rd << 22) | (rs << 17) | (rt << 12) | (L & 0xFFF);
}

// check information/register extraction 
void test_extract() {
    uint32_t rd, rs, rt, L;
    extractInformation(encode(0x18, 3, 5, 7, 0xAB), &rd, &rs, &rt, &L);
    assert(rd == 3 && rs == 5 && rt == 7 && L == 0xAB);
}

// check operation extraction 
void test_get_op() {
    uint32_t op;
    getOperation(&op, encode(0x18, 0, 0, 0, 0));
    assert(op == 0x18);
}


// test memory reading 
void test_memoryRead() {
    reset();
    uint32_t instr = encode(0x18, 3, 1, 2, 0);
    for (int i = 0; i < 4; i++) memory[0x1000 + i] = (instr >> (i * 8)) & 0xFF;
    uint32_t result;
    getInstruction(&result, 0x1000);
    assert(result == instr);
}

// test arithemetic operation
void test_add() {
    reset();
    registers[1] = 10; registers[2] = 20;
    processAdd(encode(0x18, 3, 1, 2, 0));
    assert(registers[3] == 30 && address == 0x1004);
}

// test logic operation
void test_and() {
    reset();
    registers[1] = 0xFF; registers[2] = 0x0F;
    processAnd(encode(0x00, 3, 1, 2, 0));
    assert(registers[3] == 0x0F);
}

// test shifts 
void test_shift() {
    reset();
    registers[1] = 1; registers[2] = 4;
    processShftl(encode(0x06, 3, 1, 2, 0));
    assert(registers[3] == 16);
}

// test control branching 
void test_branch() {
    reset();
    registers[5] = 0x2000; registers[1] = 42;
    processBrnz(encode(0x0B, 5, 1, 0, 0));
    assert(address == 0x2000);
}

// test data movement 
void test_mov() {
    reset();
    registers[1] = 0x2000;
    registers[2] = 0xCAFEBABE;
    processMovWrite(encode(0x13, 1, 2, 0, 0));
    address = 0x1000;
    processMovLoad(encode(0x10, 3, 1, 0, 0));
    assert(registers[3] == 0xCAFEBABE);
}

// test floating point operations 
void test_float() {
    reset();
    double a = 1.5, b = 2.5;
    memcpy(&registers[1], &a, sizeof(double));
    memcpy(&registers[2], &b, sizeof(double));
    processAddf(encode(0x14, 3, 1, 2, 0));
    double result;
    memcpy(&result, &registers[3], sizeof(double));
    assert(result == 4.0);
}

// test call and return methods 
void test_call_return() {
    reset();
    registers[5] = 0x2000;
    processCall(encode(0x0C, 5, 0, 0, 0));
    assert(address == 0x2000);
    processReturn(encode(0x0D, 0, 0, 0, 0));
    assert(address == 0x1004);
}

// main method
int main(void) {
    printf("unit tests");
    test(test_extract);
    test(test_get_op);
    test(test_memoryRead);
    test(test_add);
    test(test_and);
    test(test_shift);
    test(test_branch);
    test(test_mov);
    test(test_float);
    test(test_call_return);

    printf("%d/%d passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}