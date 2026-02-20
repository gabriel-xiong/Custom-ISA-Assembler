#include <stdio.h>
#include <stdint.h>
#include <string.h> 
#include <stdlib.h>
#include <errno.h>

/*
Errors to check: 
- cant create output files 
- unknown instruction
- wrong number of operands 
- invalid register
- negative value in unsigned literal
- literal too large (> 12 bits) or invalid literal format (e.g String)
- undefined labels/duplicate labels
- no code directive 
- instruction proceeds code label, or data precedes data label
- invalid data value for data 
*/


// macro translations: 

/*
in rd, rs -> priv rd, rs, r0, 3
out rd, rs -> priv rd, rs, r0, 4
halt -> priv r0 r0 r0 0 
clr rd -> xor rd, rd, rd
push rd -> addi r31, -8
           mov (r31)(0), rd
pop rd -> mov rd, (r31)(0)
          subi r31, -8
ld rd, L -> 
    xor r5, r5, r5
    addi r5, 0
    shftli r5, 12
    addi r5, 0
    shftli r5, 12
    addi r5, 0
    shftli r5, 12
    addi r5, 0
    shftli r5, 12
    addi r5, 259
    shftli r5, 4
    addi r5, 8



*/
// ToDo: validate instructions (register + literal), handle data 

uint64_t codeAddress = 0x2000; 
uint64_t dataAddress = 0x10000; 

typedef struct 
{
    char label[256]; 
    uint64_t memoryAddress; 
} memoryLabels; 

char* intermediatePath; 
FILE *filePath;
FILE *intermediate;

void error(char* errorMessage)
{
    fprintf(stderr, "Error: %s \n", errorMessage);
    if (filePath)
        fclose(filePath); 
    if (intermediate)
    {
        fclose(intermediate); 
        remove(intermediatePath);
    }
   
    exit(1);
}

void trim(char* str) {
    // trim lead 
    char* start = str;
    while (*start == ' ' || *start == '\t') start++;
    
    // trim trail 
    char* end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t')) {
        end--;
    }
    
    // copy back
    int len = end - start + 1;
    memmove(str, start, len);
    str[len] = '\0';
}
typedef struct
{   char operation[32];
    int operationCode; 
    int numParameters; 
    int usesLiteral; 
    int canUseSign; 
} Requirements; 

char labelsToBeAdded [512][256]; 
int stillLeft = 0; 

Requirements requirementTable[] = 
{
    {"add", 0x18, 3, 0, 0},
    {"addi", 0x19, 1, 1, 0},
    {"sub", 0x1a, 3, 0, 0},
    {"subi", 0x1b, 1, 1, 0},
    {"mul", 0x1c, 3, 0, 0},
    {"div", 0x1d, 3, 0, 0},
    {"and", 0x0, 3, 0, 0},
    {"or", 0x1, 3, 0, 0},
    {"xor", 0x2, 3, 0, 0},
    {"not", 0x3, 2, 0, 0},
    {"shftr", 0x4, 3, 0, 0},
    {"shftri", 0x5, 1, 1, 0},
    {"shftl", 0x6, 3, 0, 0},
    {"shftli", 0x7, 1, 1, 0},
    {"br", 0x8, 1, 0, 0},
    {"brr", 0x9, 1, 0, 0},
    {"brr", 0xa, 0, 1, 1},
    {"brnz", 0xb, 2, 0, 0},
    {"call", 0xc, 1, 0, 0},
    {"return", 0xd, 0, 0, 0},
    {"brgt", 0xe, 3, 0, 0},
    {"priv", 0xf, 3, 1, 0},
    {"mov", 0x10, 2, 1, 1},
    {"mov", 0x11, 2, 0, 0},
    {"mov", 0x12, 1, 1, 0},
    {"mov", 0x13, 2, 1, 1},
    {"addf", 0x14, 3, 0,0},
    {"subf", 0x15, 3, 0, 0},
    {"mulf", 0x16, 3, 0, 0},
    {"divf", 0x17, 3, 0, 0},
};

Requirements* getRequirement(char* instruction, char* operations)
{   //printf("Instruction: %s", instruction);
    trim(instruction); 
    trim(operations); 
    int x = sizeof(requirementTable)/sizeof(Requirements);

    if (strcmp(instruction, "brr") == 0)
    {
        if (operations[0] == 'r')
        {
            for (int i = 0; i < x; i++)
                if (requirementTable[i].operationCode == 0x9)
                    return &requirementTable[i]; 
        }
        else 
        {
            for (int i = 0; i < x; i++)
                if (requirementTable[i].operationCode == 0xa)
                    return &requirementTable[i]; 
        }
    }
    if (strcmp(instruction, "mov") == 0)
    {   /*
        rd, (rs)(L) 0x10
        rd, rs 0x11
        rd, L 0x12
        (rd)(L), rs 0x13
        */
        if (operations[0] == 'r')
        {   
            if (strstr(operations, "(") != NULL)
               {for (int i = 0; i < x; i++)
                if (requirementTable[i].operationCode == 0x10)
                    return &requirementTable[i]; }
            else
                {
                    char* next = operations + 1; 
                    if (strstr(next, ", r") != NULL)
                        {
                             for (int i = 0; i < x; i++)
                                if (requirementTable[i].operationCode == 0x11)
                                     return &requirementTable[i]; 
                        }
                    else 
                        {
                            for (int i = 0; i < x; i++)
                                if (requirementTable[i].operationCode == 0x12)
                                     return &requirementTable[i]; 
                        }
                   
                }
        }
        else
        {
            for (int i = 0; i < x; i++)
                if (requirementTable[i].operationCode == 0x13)
                    return &requirementTable[i];
        }
    }
             
        
    
    
    for (int i = 0; i < x; i++)
    {   //printf("Current requirement: %s\n", requirementTable[i].operation);
        if (strcmp(requirementTable[i].operation, instruction) == 0)
            return &requirementTable[i]; 
    }

   // fprintf(stderr, "getRequirement failed for instruction: [%s]\n", instruction);

    return NULL; 
}
int isMacro(char* line)
{   
    // spaces to make sure that instructions dont accidentally contain macros
    // is this even possible/ also are there possibilities where it messes up
    // whitespaces? 
    return strstr(line, "in ")!= NULL || strstr(line, "out ")!= NULL
    || strstr(line, "clr ")!= NULL || strstr(line, "ld " )!= NULL
    || strstr(line, "push ")!= NULL || strstr(line, "pop ") != NULL || 
    strstr(line, "halt")!= NULL; 
}

int getNumInstructions(char* line)
{   // in out halt clr 
    if (strstr(line, "in ")!= NULL || strstr(line, "out ")!= NULL || 
    strstr(line, "halt")!= NULL || strstr(line, "clr ")!= NULL)
        return 1; 

    if (strstr(line, "push ")!= NULL || strstr(line, "pop ")!= NULL)
        return 2; 
    
    if (strstr(line, "ld ")!= NULL)
        return 12; 

    return 0; 
}

void validateRegister(char* rd)
{   trim(rd); 
    if (rd[0] != 'r')
        {   
            error("Error: invalid register!");  
        }

    char* rd2 = rd + 1; 
    char* end; 
    int val = strtol(rd2, &end, 10);

    if (val < 0 || val > 31 || *end != '\0')
    {
        error("Error: invalid register!");
    }

}

void validateLiteral(char* literal, int negativeAllowed)
{
    trim(literal);

    if (literal[0] == ':')
    {
        error("Error: label cant be used as literal");
    }
    char* end; 
    long val = strtol(literal, &end, 10);

    if (*end != '\0')
    {
        error("Error: invalid literal");
    }

    if (negativeAllowed)
    {
        if (val < -2048 || val > 2047)
        {
            error("Error: literal outside range of representable form");
        }
    }
    else
        {
            if (val < 0 || val > 4095)
            {
                error("Error: literal outside range of representable form");
            }
        }
   
    }



int validLabel(memoryLabels* labels, int numLabels, char* label)
{   
    for (int i = 0; i < numLabels; i++)
    {   
        if (strcmp(labels[i].label, label) == 0) {
                return 1; 
        }
    }
    return 0; 
}

uint64_t getAddressFromLabel(memoryLabels* labels, int numLabels, char* label)
{
     for (int i = 0; i < numLabels; i++)
    {
        if (strcmp(labels[i].label, label) == 0) {
                return labels[i].memoryAddress; 
        }
     }
    return 0; 
}

// extract bits for LD conversion 
void extractBits(uint64_t val, uint64_t* values)
{
    values[0]= (val >> 52) & 0xFFF;
    values[1] = (val >> 40) & 0xFFF;
    values[2] = (val >> 28) & 0xFFF;
    values[3] = (val >> 16) & 0xFFF;
    values[4] = (val >> 4) & 0xFFF;
    values[5] = val & 0xF;
}

void processLD(char* line, FILE* intermediate, memoryLabels* labels, int numLabels)
{   // ToDo: validate register + literal 
    char parameter[512]; 
    char rd[16]; 
    
    sscanf(line, " ld %[^,], %s", rd, parameter);
    uint64_t val; 
    trim(parameter);
    trim(rd);
    validateRegister(rd); 
    // literal case 
    if (parameter[0] != ':')
    {
        // attempt to read literal 
        char* end;
        errno = 0; 
        val = strtoull(parameter, &end, 10);
        
        if (errno == ERANGE)
            error("out of range");

        // not valid 
        if (*end != '\0') {
            error("Error: invalid literal in ld instruction\n");
        }
    }
    else // label case 
    {   
        char* label = parameter + 1; 
        trim(label); 
      
        if (validLabel(labels, numLabels, label))
        {
            val = getAddressFromLabel(labels, numLabels, label); 
        }
        else
        {   
            error("Error: Label does not have an associated memory address");            
        }
    }

    uint64_t values[6];
    extractBits(val, values); 

    fprintf(intermediate, "\txor %s, %s, %s\n", rd, rd, rd);
    fprintf(intermediate, "\taddi %s, %llu\n", rd, values[0]);
    fprintf(intermediate, "\tshftli %s, 12\n", rd);
    fprintf(intermediate, "\taddi %s, %llu\n", rd, values[1]);
    fprintf(intermediate, "\tshftli %s, 12\n", rd);
    fprintf(intermediate, "\taddi %s, %llu\n", rd, values[2]);
    fprintf(intermediate, "\tshftli %s, 12\n", rd);
    fprintf(intermediate, "\taddi %s, %llu\n", rd, values[3]);
    fprintf(intermediate, "\tshftli %s, 12\n", rd);
    fprintf(intermediate, "\taddi %s, %llu\n", rd, values[4]);
    fprintf(intermediate, "\tshftli %s, 4\n", rd);
    fprintf(intermediate, "\taddi %s, %llu\n", rd, values[5]);
}

void writeMacro(char* line, FILE* intermediate, memoryLabels* labels, int numLabels)
{  
    //Todo: Validate registers + # of arguments 
    if (strstr(line, "halt") != NULL)
    {   if (strlen(line) !=5)
            error("halt too long");

        fprintf(intermediate, "\tpriv r0, r0, r0, 0\n");
    }
    else if (strstr(line, "clr ") != NULL)
    {
        char registerNumber[10];
        sscanf(line, " clr %s", registerNumber);
        trim(registerNumber);
        validateRegister(registerNumber);
        fprintf(intermediate, "\txor %s, %s, %s\n", registerNumber, registerNumber, registerNumber);
    }
    else if (strstr(line, "in ") != NULL)
    {
        char rd[16]; 
        char rs[16]; 
        sscanf(line, " in %[^,], %s", rd, rs);
        trim(rd);
        trim(rs); 
        validateRegister(rd);
        validateRegister(rs);
        fprintf(intermediate, "\tpriv %s, %s, r0, 3\n", rd, rs);
    }
    else if (strstr(line, "out ") != NULL)
    {
        char rd[16]; 
        char rs[16]; 
        sscanf(line, " out %[^,], %s", rd, rs);
        trim(rd);
        trim(rs); 
        validateRegister(rd);
        validateRegister(rs);
        fprintf(intermediate, "\tpriv %s, %s, r0, 4\n", rd, rs);
    }
    else if (strstr(line, "push ") != NULL)
    {  
        char rd[16]; 
        sscanf(line, " push %s", rd);
        trim(rd); 
        validateRegister(rd);
        fprintf(intermediate, "\tmov (r31)(-8), %s\n", rd);
        fprintf(intermediate, "\tsubi r31, 8\n");
     
    }
    else if (strstr(line, "pop ") !=NULL)
    {   
        char rd[16]; 
        sscanf(line, " pop %s", rd);
        trim(rd); 
        validateRegister(rd);
        fprintf(intermediate, "\tmov %s, (r31)(0)\n", rd);
        fprintf(intermediate, "\taddi r31, 8\n");  
    }
    else if (strstr(line, "ld ") != NULL)
    {
        processLD(line, intermediate, labels, numLabels); 
    }
}

void validateExtra(char* extra)
{
    trim(extra); 
    if (strlen(extra) > 0)
        error("trailing garbage"); 
}
void validateInstruction(char* line)
{   //printf("validating instruction");
    char instruction[64] = {0};
    char operations[512] = {0}; 

    // parse instruction name
    sscanf(line, " %s %[^\n]", instruction, operations);
    trim(instruction); 
    trim(operations); 
    Requirements* requirement = getRequirement(instruction, operations); 

    int commas = 0;
    for (int i = 0; operations[i]; i++) {
        if (operations[i] == ',') commas++;
    }

    int realComma = 0; 

    if (requirement == NULL)
    {   
        error("Error: invalid instruction");
    }

    /*possible combinations of register/literal #:
    3, 0
    1, 1
    2, 0
    2, 1
    1, 0 
    0, 1
    */ 

    if (requirement->numParameters > 0 || requirement->usesLiteral > 0) {
        if (strlen(operations) == 0) {
            error("Error: missing operations");
            realComma = 0; 
        }
    }

    if (requirement->numParameters == 3 && requirement ->usesLiteral == 0)
    {   char r1[16], r2[16], r3[16]; 
        char remaining[256] = ""; 
        int ops = sscanf(operations, " %[^,], %[^,], %s %s", r1, r2, r3, remaining);
        realComma = 2; 
        
        if (ops != 3) {
            error("insufficent number of operations");
        }
        validateExtra(remaining);
        validateRegister(r1);
        validateRegister(r2);
        validateRegister(r3);

        
    }
    else if (requirement->numParameters == 2 && requirement ->usesLiteral == 0)
    {   realComma = 1; 
        char r1[16], r2[16]; 
        char remaining[256] = "";
        int ops = sscanf(operations, " %[^,], %s %s", r1, r2, remaining);
        
        
        if (ops != 2) {
            error("insufficent number of operations");
        }
        
        validateExtra(remaining);
        validateRegister(r1);
        validateRegister(r2);  
    }
    else if (requirement->numParameters == 1 && requirement ->usesLiteral == 0)
    {   realComma = 0; 
        char r1[16]; char remaining[256] = ""; 
        int ops = sscanf(operations, " %s %s", r1, remaining);
        
        
        if (ops != 1) {
            error("insufficent number of operations");
        }
        validateExtra(remaining);
        validateRegister(r1);
        
    }
    else if (requirement->numParameters == 1 && requirement ->usesLiteral == 1)
    {   realComma = 1; 
        char r1[16], literal[16]; char remaining[256] = ""; 
        int ops = sscanf(operations, " %[^,], %s %s", r1, literal, remaining);
        
        
        if (ops != 2) {
            error("insufficent number of operations");
        }
        validateExtra(remaining);
        validateLiteral(literal, requirement->canUseSign); 
        validateRegister(r1);
    } // only occurs in mov case 
    else if (requirement->numParameters == 2 && requirement ->usesLiteral == 1)
    {    
        int open = 0; int closed = 0; 
            for (int i = 0; line[i] != '\0'; i++)
                {
                    if (line[i] == '(')
                        open++; 
                    else if (line[i] == ')')
                        closed++; 
                }
            
            if (open != closed)
                error("bad parantheses");

        realComma = 1; 
        char remaining[256] = ""; 
        // mov cases: mov rd, (rs)(L), mov (rd)(L), rs
        if (operations[0] == '(')
        {   // mov (rd)(L), rs case 
            char r1[16], r2[16], literal[64]; 
            int ops = sscanf(operations, " (%[^)])(%[^)]), %s %s", r1, literal, r2, remaining);
             if (ops != 3) {
            error("insufficent number of operations");
        }
         validateExtra(remaining);
            validateRegister(r1);
            validateRegister(r2);
            validateLiteral(literal, requirement->canUseSign); 
        }
        else
        {   
            // mov rd, (rs)(L) case
            char r1[16], r2[16], literal[64];
            
            int ops = sscanf(operations," %[^,], (%[^)])(%[^)]) %s", r1, r2, literal, remaining);
             if (ops != 3) {
            error("insufficent number of operations");
            
            char *last = strrchr(line, ')');
            last++;
            if (*last != '\0')
                error("please work");
            
        }
            validateExtra(remaining);
            validateRegister(r1);
            validateRegister(r2); 
            validateLiteral(literal, requirement->canUseSign);
        }
    }
    else if (requirement->numParameters == 3 && requirement ->usesLiteral == 1)
    {   realComma = 3; 
        char r1[16], r2[16], r3[16], lit[16]; char remaining[256] = ""; 
        int ops = sscanf(operations, " %[^,], %[^,], %[^,], %s %s", r1, r2, r3, lit, remaining);
        
        if (ops != 4) {
            error("insufficent number of operations");
        }
        validateExtra(remaining);
        validateRegister(r1);
        validateRegister(r2);
        validateRegister(r3);
        validateLiteral(lit, requirement->canUseSign);
    }
    else if (requirement->numParameters == 0 && requirement->usesLiteral == 1)
    {   realComma = 0; 
        char lit[16]; char remaining[256] = ""; 
        int ops = sscanf(operations, " %s %s", lit, remaining);
        if (ops != 1)
            error("insufficient number of parameters");
         validateExtra(remaining);
        validateLiteral(lit, requirement->canUseSign); 
    }
    else if (requirement->numParameters == 0 && requirement-> usesLiteral ==0)
    {   
        realComma = 0; 
        if (strlen(operations)!=0)
            error("insufficent number of parameters");
    }
    else
    {
        error("faulty instruction");
    }

    if (commas != realComma)
        error("bad commas");
}

void handleData(char* line, memoryLabels* labels,  int* numLabels, FILE* intermediate) 
{
   line = line + 1;
   trim(line); 
   
   errno = 0;
   char* end; 
   uint64_t val = strtoull(line, &end, 10); 
   if (errno == ERANGE)
        error("out of range");

   if (*end != '\0')
   {
    error("Error: invalid data value");
    
   }

   fprintf(intermediate, "\t%llu\n", val);
}

void handleCode(char* line, FILE* intermediate, memoryLabels* labels, int numLabels)
{   //printf("handling code");
    if (isMacro(line))
     {
        writeMacro(line, intermediate, labels, numLabels); 
     }
     else
     {  // combine validate instructions 
        validateInstruction(line);
        fprintf(intermediate, "%s\n", line); 
     }
}

void handleLabel(char* line, memoryLabels* labels, int* numLabels, uint64_t address)
{   

    char* start = line +1; 
    start[strcspn(start, "\n")] = '\0';
    trim(start); 

    if (strlen(start) == 0) {
        error("Error: empty label name");
    }

    for (int i = 0; line[i] != '\0'; i++)
    {
        if (start[i] == ' ' || start[i] == '\t')
            error("invalid space");
    }

    for (int i = 0; i < *numLabels; i++) {
        if (strcmp(labels[i].label, start) == 0) {
            error("Error: duplicate label");
        }
    }
    // Store label in symbol table
    labels[*numLabels].memoryAddress = address;
    strcpy(labels[*numLabels].label, start);
    (*numLabels)++;
}
void buildLabelTable(FILE* input, memoryLabels* labels, int* numLabels)
{   
    char line[256];
    char mode[10] = ""; 
    int encounteredCodeBlock = 0; 

    while (fgets(line, sizeof(line), input)) {
         

        line[strcspn(line, "\n")] = '\0';
           
        if (strlen(line) == 0 || line[0] == ';') 
            continue;

        if (strncmp(line, ".code", 5) == 0) {
            strcpy(mode, "code");
            encounteredCodeBlock = 1;
            continue;
        }
        else if (strncmp(line, ".data", 5) == 0)
        {   strcpy(mode, "data");
            continue;

        }
        else if (line[0] == '\t') //instruction 
        {   
            // check data mode 
            if (strncmp(mode, "data", 4) == 0)
            {   
               for (int i = 0; i < stillLeft; i++)
                  handleLabel(labelsToBeAdded[i], labels, numLabels, dataAddress);
                stillLeft = 0; 
                dataAddress += 8; 
            }
            else if (strncmp(mode, "code", 4) == 0) // check code mode 
            {    for (int i = 0; i < stillLeft; i++)
                  handleLabel(labelsToBeAdded[i], labels, numLabels, codeAddress);
                
                  stillLeft =0; 
                if (isMacro(line))
                    codeAddress += 4 * getNumInstructions(line); 
                else 
                    codeAddress += 4; 
            }
            else
            {  //printf("%s", mode);
               error("Error: instruction before code or data\n");
            }
        }
        else if (line[0] == ':')   // handle label case 
        {   
            
           strcpy(labelsToBeAdded[stillLeft++], line);
           if (strncmp(mode, "code", 4) == 0)  
              continue; 
           else if (strncmp(mode, "data", 4) == 0)
              continue; 
           else
                error("faulty label");
          
        }
        else // error 
         {  // printf("%s", mode);
        //     printf("%c\n", line[0]); 
        //     printf("%c\n", line[1]); 
        //     printf("%c\n", line[2]); 
        //     printf("%c\n", line[3]); 
            error("Error: instruction before code or data\n");
            
        }
    }
    if (!encounteredCodeBlock)
    {
        error("Error: no code block encountered\n");
        
    } 
}

void processFile(FILE* filePath, FILE* intermediate, char* intermediatePath, 
memoryLabels* labels, int numLabels)
{
    char line[256];
    char mode[10] = ""; 

    while (fgets(line, sizeof(line), filePath)) {
        line[strcspn(line, "\n")] = '\0';

        if (strlen(line) == 0 || line[0] == ';') 
            continue;
      

        if (strncmp(line, ".code", 5) == 0) { // code mode 
            if (strncmp(mode, "code", 4) == 0)
                continue; 
            strcpy(mode, "code");
            fprintf(intermediate, ".code\n");
            continue;
        }
        else if (strncmp(line, ".data", 5) == 0) // data mode 
        {   if (strncmp(mode, "data", 4) == 0)
                continue; 
            strcpy(mode, "data");
            fprintf(intermediate, ".data\n");
            continue;
        }
        else if (line[0] == '\t') // tab mode 
        { 
            if (strncmp(mode, "data", 4) == 0)
            {   
                handleData(line, labels, &numLabels, intermediate);
            }
            else if (strncmp(mode, "code", 4) == 0)
            {   
                handleCode(line, intermediate, labels, numLabels);
            }
            else
            {
                error("Faulty tab");
            }
        }
        // handle label case 
        else if (line[0] == ':')
        {   
            continue; 
        }
        else 
        {
            error("Invalid instruction"); 
        }
    }

    fclose(filePath);
    fclose(intermediate); 
    
}

int parseRegister(char* reg) {
    trim(reg);
    return atoi(reg + 1);
}

int parseLiteral(char* literal, int signedInt) {
    trim(literal);
    long val = strtol(literal, NULL, 10);
    
    // convert to 2's complemnt 
    if (signedInt && val < 0) {
        val = (1 << 12) + val;  
    }
    
    return (int)(val & 0xFFF); 
}


void translateData(char* line, FILE* output)
{   char* tabLess = line + 1; 
    char* endptr;
    errno = 0; 
    uint64_t value = strtoull(tabLess, &endptr, 10);
    if (errno == ERANGE)
        error("out of range");
    fwrite(&value, sizeof(uint64_t), 1, output);
}

void convertEverythingElse(Requirements* requirement, uint32_t* curr, char *operators)
{
    // 3 registers, no literal
    if (requirement->numParameters == 3 && requirement->usesLiteral == 0) {
        char r1[16], r2[16], r3[16];
        sscanf(operators, " %[^,], %[^,], %s", r1, r2, r3);
        
        int num1 = parseRegister(r1);
        int num2 = parseRegister(r2);
        int num3 = parseRegister(r3);
        
        *curr |= (num1 & 0x1F) << 22;
        *curr |= (num2 & 0x1F) << 17;
        *curr |= (num3 & 0x1F) << 12;
    }
    else if (requirement->numParameters == 2 && requirement->usesLiteral == 0) {
        char r1[16], r2[16];
        sscanf(operators, " %[^,], %s", r1, r2);
        
        int num1 = parseRegister(r1);
        int num2 = parseRegister(r2);
        
        *curr |= (num1 & 0x1F) << 22;
        *curr |= (num2 & 0x1F) << 17;
    }
    else if (requirement->numParameters == 1 && requirement->usesLiteral == 0) {
        char r1[16];
        sscanf(operators, " %s", r1);
        
        int num1 = parseRegister(r1);
       
        *curr |= (num1 & 0x1F) << 22;
    }
    else if (requirement->numParameters == 0 && requirement->usesLiteral == 0) {
    }
    else if (requirement->numParameters == 3 && requirement->usesLiteral == 1) {
        char r1[16], r2[16], r3[16], literal[16];
        sscanf(operators, " %[^,], %[^,], %[^,], %s", r1, r2, r3, literal);
        
        int num1 = parseRegister(r1);
        int num2 = parseRegister(r2);
        int num3 = parseRegister(r3);
        
      
        *curr |= (num1 & 0x1F) << 22;
        *curr |= (num2 & 0x1F) << 17;
        *curr |= (num3 & 0x1F) << 12;

        int literalNum = parseLiteral(literal, requirement->canUseSign); 

        *curr |= (literalNum & 0xFFF); 
        
    }
    else if (requirement->numParameters == 2 && requirement->usesLiteral == 1) {
        
        // mov cases: mov rd, (rs)(L), mov (rd)(L), rs
        if (operators[0] == '(')
        {   // mov (rd)(L), rs case 
            char r1[16], r2[16], literal[64]; 
            int ops = sscanf(operators, " (%[^)])(%[^)]), %s", r1, literal, r2);
             
        int num1 = parseRegister(r1);
        int num2 = parseRegister(r2);

        *curr |= (num1 & 0x1F) << 22;
        *curr |= (num2 & 0x1F) << 17;
       
        int literalNum = parseLiteral(literal, requirement->canUseSign); 
        *curr |= (literalNum & 0xFFF); 
        }
        else
        {
            // mov rd, (rs)(L) case
            char r1[16], r2[16], literal[64];
            int ops = sscanf(operators," %[^,], (%[^)])(%[^)])", r1, r2, literal);
            
           int num1 = parseRegister(r1);
           int num2 = parseRegister(r2);

           *curr |= (num1 & 0x1F) << 22;
           *curr |= (num2 & 0x1F) << 17;
       
            int literalNum = parseLiteral(literal, requirement->canUseSign); 
            *curr |= (literalNum & 0xFFF); 
        }
    }
    else if (requirement->numParameters == 1 && requirement->usesLiteral == 1) {
        
        char r1[16], literal[16];
        sscanf(operators, " %[^,], %s", r1, literal);
        
        int num1 = parseRegister(r1);
        *curr |= (num1 & 0x1F) << 22;
       
        int literalNum = parseLiteral(literal, requirement->canUseSign); 
        *curr |= (literalNum & 0xFFF); 
    }
    else if (requirement->numParameters == 0 && requirement->usesLiteral == 1) {
        char literal[16]; 
        sscanf(operators, " %s", literal);
        int literalNum = parseLiteral(literal, requirement->canUseSign); 
        *curr |= (literalNum & 0xFFF); 
    }
}
void translateCode(char* line, FILE* output)
{  
   // extract instruction type + operators using sscanf  
    char instruction[32] = {0}, operation[256] = {0}; 
    sscanf(line, "%s %[^\n]", instruction, operation);

    Requirements* requirement = getRequirement(instruction, operation); 
    if (requirement == NULL)
        error("invalid requirement");

    uint32_t encodedInstruction = 0; 
    encodedInstruction |= (requirement->operationCode & 0x1F) << 27; 
    
    convertEverythingElse(requirement, &encodedInstruction, operation); 


   // casework on number of operators; note  opcode  │   rd    │   rs    │   rt    │   literal    │
                                    //     │ 5 bits  │ 5 bits  │ 5 bits  │ 5 bits  │   12 bits   

   fwrite(&encodedInstruction, sizeof(uint32_t), 1, output);
}
void passTwo(char* intermediatePath, char* outputPath)
{
    FILE* intermediate = fopen(intermediatePath, "r");
    FILE* output = fopen(outputPath, "wb");  

    if (!output || !intermediate)
        error("file could not be opened");

    uint64_t head[5];
    head[0] = 0;
    head[1] = 0x2000;
    head[2] = codeAddress - 0x2000;
    head[3] = 0x10000;
    head[4] = dataAddress - 0x10000;
    fwrite(head, sizeof(uint64_t), 5, output);


    char line[512]; 
    char mode[16];
    int count = 0; 

    // code first 
    while (fgets(line, sizeof(line), intermediate)) 
    {   count++; 
        //printf("%s", line); 
        line[strcspn(line, "\n")] = '\0';
        
        if (strlen(line) == 0)
            continue; 
        
        if (strncmp(line, ".data", 5) == 0)
            {
                strcpy(mode, "data");
                continue; 
            }
        else if (strncmp(line, ".code", 5)== 0)
            {   
                strcpy(mode, "code");
                continue; 
            }
        else if (line[0] == '\t')
            {
                if (strcmp(mode, "data") == 0)
                    continue; 
                else if(strcmp(mode, "code") == 0)
                    translateCode(line, output); 
                else
                    error("faulty tab");
            }
        else
           { //error("were cooked bro");
              //printf("Line: %s", line); 
              //printf("%d", count);
              exit(1); }
    }
    
    rewind(intermediate);
    while (fgets(line, sizeof(line), intermediate)) 
    {   count++; 
        //printf("%s", line); 
        line[strcspn(line, "\n")] = '\0';
        
        if (strlen(line) == 0)
            continue; 
        
        if (strncmp(line, ".data", 5) == 0)
            {
                strcpy(mode, "data");
                continue; 
            }
        else if (strncmp(line, ".code", 5)== 0)
            {   
                strcpy(mode, "code");
                continue; 
            }
        else if (line[0] == '\t')
            {
                if (strcmp(mode, "data") == 0)
                    translateData(line, output);
                else if(strcmp(mode, "code") == 0)
                    continue;
                else
                    error("faulty tab");
            }
        else
           { //error("were cooked bro");
             // printf("Line: %s", line); 
             // printf("%d", count);
              exit(1); }
    }

    fclose(intermediate);
 //   remove(intermediatePath);
    fclose(output);
    
    

}
int main(int argc, char * argv[])
{   
    if (argc != 3)
    {
        error("Error: invalid number of arguments");
    }

    // initialize requirements 
    memoryLabels labels[256];
    int numLabels = 0; 

    char* inputPath = argv[1]; 
    intermediatePath = "temp.tk"; 
    char* outputPath = argv[2]; 

    filePath = fopen(inputPath, "r");
    intermediate = fopen(intermediatePath, "w");
    // remember to wipe file if error ? 
    if (!filePath || !intermediate)
    {
        error("Error: file could not be opened");
    }

    buildLabelTable(filePath, labels, &numLabels);
    rewind(filePath);  
    processFile(filePath, intermediate, intermediatePath, labels, numLabels); 

    passTwo(intermediatePath, outputPath);
   // remove(intermediatePath);
    return 0; 
}