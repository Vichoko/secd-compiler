//
// Created by Vichoko on 20-05-2016.
//

#include <iostream>
#include <fstream>
#include "./sexpr-1.3/src/sexp.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef int boolean;
#define true 1
#define false 0

/* Global variables*/
int instrInd;
char* instrBuffer[BUFSIZ];
int outInd;
char* outBuffer[BUFSIZ];
int funpushindex;
int ifcondindex;

/* Accepted SECD instructions */
char separator[2] = "$";
char endindicator[2] = "%";
char pushstr[100] = "INT_CONST";
char addstr[100] = "ADD";
char substr[100] = "SUB";
char applystr[100] = "APPLY";
char returnstr[100] = "RETURN";
char funstr[100] = "FUN";
char ifstr[100] = "IF0";

/* ARM Assembly standard instructions (Already indented)*/
char pushr0[100] = "    push {r0}";
char pushr4[100] = "	push {r4}";
char popr0[100] = "	pop {r0}";
char popr4[100] = "	pop {r4}";
char popr5[100] = "	pop {r5}";
char addr4r5[100] = "	add r4, r4, r5";
char subr4r5[100] = "	sub r4, r4, r5";

char testr4[100] = "	tst r4, r4";
char bleq[100] = "	bleq ";
char blne[100] = "	blne ";
char b[100] = "	b ";
char blr4str[100] = "	blx r4";
char convention[100] = "	stmfd sp!, {r4, r5, lr}";
char endconvention[100] = "	ldmfd sp!, {r4, r5, pc}";



/* Stores the parsed instructions in the instruction buffer (instrBuffer) */
void visitAll(elt *elt, unsigned int level) {
    if (elt == NULL)
        return;

    /* If the instruction is IF0; then the next lists of instructions should
     * be separated by '$' and ended with '%' for further recognition. */
    boolean isIf0 = false;

    /* If elt contains an atomic value */
    if (elt->val != NULL)
    {
		/* The value is saved in the buffer. */
        instrBuffer[instrInd++] = elt -> val;
		
		/* also if the instruction is IF0, the instruction lists are separated, and the end of the second list is marked too. */
        if (strcmp(elt->val,"IF0") == 0){
            isIf0=true;
            visitAll(elt -> next -> list, level);
            instrBuffer[instrInd++] = separator;
            visitAll(elt -> next -> next -> list, level);
            instrBuffer[instrInd++] = endindicator;
        }
    }

	/* If the value wasn't IF0, it is visited in-order. */
    if (isIf0==false){
        visitAll(elt->list, level+1);
        visitAll(elt->next, level);
    }

}

/* Appends the string in the outBuffer global array.*/
void appendOutBuffer(char* pstring){
    outBuffer[outInd++] = pstring;
}

/* Recieve a pointer to the index (To change this index if needed) and if the instruction shoul'd be compiled or 'pre-compiled'.
	It compiles the SECD instruction pointed by *instructionIndex and appends it to the outbuffer. */
int compileInstruction(int* instructionIndex, boolean isPrecompilation = false){
    int i = *instructionIndex;
	
    /* If instruction = INT_CONST */
    if (strcmp(instrBuffer[i], pushstr) == 0){
        /* If the next thing in buffer is a number */
        if (isdigit( *instrBuffer[++i] )) {
            char *movr4 = (char *) malloc(sizeof(char) * 100);

            stpcpy(movr4, "	mov	r4, #");
            strcat(movr4, instrBuffer[i]);
            appendOutBuffer(movr4);

            /* push {r4} */
            appendOutBuffer(pushr4);

            /* Step into the next instr */
            i++;
        }
        else{/* Compilation eror */
            std::cout << "COMPILE ERROR: " << instrBuffer[i] << " is not a Integer number.\n";
            return -2;
        }
    }

        /* IF instruction = ADD */
    else if (strcmp(instrBuffer[i], addstr) == 0){
        appendOutBuffer(popr4);
        appendOutBuffer(popr5);
        appendOutBuffer(addr4r5);
        appendOutBuffer(pushr4);
        i++;
    }

        /* IF instruction = SUB */
    else if (strcmp(instrBuffer[i], substr) == 0){
        appendOutBuffer(popr4);
        appendOutBuffer(popr5);
        appendOutBuffer(subr4r5);
        appendOutBuffer(pushr4);
        i++;
    }
        /* IF instruction = IF0  */
    else if (strcmp(instrBuffer[i], ifstr)==0){
        i++;
        appendOutBuffer(popr4);
        appendOutBuffer(testr4);

        /* Label construction*/
        char* blcond1 = (char*) malloc(sizeof(char)*100);
        char condindexstr[5] = "";
        condindexstr[0] =  ifcondindex++ + '0';
        strcpy(blcond1, blne);
        strcat(blcond1, "L");
        strcat(blcond1, &condindexstr[0]);
        appendOutBuffer(blcond1);

        /* Compiles the If r4 == 0 block. */
        while (strcmp(instrBuffer[i], "$")!=0){
            compileInstruction(&i);
            if (i >= instrInd){
                std::cout << "Error in IF0 Instruction. Support for nested if0 not exist.\n";
                return -4;
            }
        }
        /* This code is reached when instrBuffer[i]='$'; pass to the next instruct.*/
        i++;

        /* This arm instruction block finishes with a jump to the 'continue' block.*/
        char* bcontinue = (char*) malloc(sizeof(char)*100);
        condindexstr[0] =  ifcondindex++ + '0';
        strcpy(bcontinue, b);
        strcat(bcontinue, "L");
        strcat(bcontinue, &condindexstr[0]);
        appendOutBuffer(bcontinue);

        /* Then the if r4 != 0 arm block. */
        char* lbcond1 = (char*) malloc(sizeof(char)*100);
        condindexstr[0] =  (ifcondindex - 2) + '0';
        stpcpy(lbcond1, "L");
        strcat(lbcond1, &condindexstr[0]);
        strcat(lbcond1, ":");
        appendOutBuffer(lbcond1);

        while (strcmp(instrBuffer[i], "%")){
            compileInstruction(&i);
            if (i >= instrInd){
                std::cout << "Error in IF0 Instruction. Support for nested if0 not exist.\n";
                return -4;
            }
        }
        /* This code is reached when instrBuffer[i]='%'; pass to the next instruct.*/
        i++;

        /* Then the if continuation of the if arm instructions. */
        char* lbcontinue = (char*) malloc(sizeof(char)*100);
        condindexstr[0] =  (ifcondindex - 1) + '0';
        stpcpy(lbcontinue, "L");
        strcat(lbcontinue, &condindexstr[0]);
        strcat(lbcontinue, ":");
        appendOutBuffer(lbcontinue);
    }

        /* IF instrucction = FUN and this instance is called for compilation process; push pointer to it. */
    else if (strcmp(instrBuffer[i], funstr) == 0 && isPrecompilation == false){
        /* Label construction*/
        char * armlabel = (char*) malloc(sizeof(char)*100);
        char funpushindexstr[5] = "";
        funpushindexstr[0] =  funpushindex++ + '0';
        stpcpy(armlabel, "F");
        strcat(armlabel, &funpushindexstr[0]);

        /* Load the pointer to the label into r4 */
        char* ldrstr = (char*) sexp_malloc(sizeof(char)*100);
        stpcpy(ldrstr, "	ldr	r4, =");
        strcat(ldrstr, armlabel);
        appendOutBuffer(ldrstr);

        /* push {r4} */
        appendOutBuffer(pushr4);

        /* Skip instructions until return */
        while (strcmp(instrBuffer[i], returnstr) != 0){
            i++;
        }
        /* This code is reached when instruction is return; go to next instruction */
        i++;
    }

        /* IF instrucction = APPLY and this instance is called for compilation process; pop arg and fun; bl fun; push returned value */
    else if (strcmp(instrBuffer[i], applystr) == 0 && isPrecompilation == false){
        //r0 contain argument.
        appendOutBuffer(popr0);
        //r4 contain FUN pointer
        appendOutBuffer(popr4);
        // bl r4
        appendOutBuffer(blr4str);
        //r0 contain returned value; push it
        appendOutBuffer(pushr0);
        i++;
    }

    else{ /* Compilation eror */
        std::cout << "COMPILE ERROR: " << instrBuffer[i] << " not recognised as valid instruction.\n";
        return -1;
    }
	/* Update value of the index */
    *instructionIndex = i;

}

int main()
{
    std::cout << "SECD INSTRUCTION TO ARM ASSEMBLER\n\n";

    char linebuf[BUFSIZ];
    FILE *fp;
    char *status;
    sexp_t *sx;

    /* Loads input file*/
    std::cout << "Opening file...\n";
    fp = fopen("input.txt","r+");

    /* Init. Indexes*/
    std::cout << "ARM Instruction Buffer (outBuffer) created...\n";
    std::cout << "SECD Instruction Buffer (instrBuffer) created...\n";
    instrInd = 0;
    outInd = 0;

    status = fgets(linebuf,BUFSIZ,fp);
    /* if not EOF and status was NULL, something bad happened. */
    if (status != linebuf) {
        printf("Error encountered on fgets.\n");
        exit(EXIT_FAILURE);
    }

    sx = parse_sexp(linebuf,BUFSIZ);

    std::cout << "Filling (SECD) instruction buffer...\n";
    visitAll(sx->list, 1);
    std::cout << "	done!\n";

    /******************************************** START creating Output Buffer*/
	
    /* Define the IF0 label index in 0 for compilation process. */
    ifcondindex = 0;

    /* Load the header to the output buffer.*/
    char textzone[100]=".text";
    char sglobal[100] = ".global main";
    char loadprintf[100] = ".extern printf";

    appendOutBuffer(textzone);
    appendOutBuffer(sglobal);
    appendOutBuffer(loadprintf);

    /* pre-compilation process: Compiles functions ((FUN) instructions) */
	/* Define the FUN label index in 0 for pre-compilation process. */
    int funindex = 0;
    std::cout << "Starting function compilation (pre-compilation)...\n";

    /* PRE-COMPPILING: Iterates over the instruction buffer and compiles functions. */
	int i;
    for (i = 0; i != instrInd; ) {
        /* If instruction = FUN */
        if (strcmp(instrBuffer[i++], funstr) == 0){
            /* Label construction*/
            char* armlabel = (char*) malloc(sizeof(char)*100);
            char funindexstr[5] = "";
			funindexstr[0] =  funindex++ + '0';
            stpcpy(armlabel, "F");
            strcat(armlabel, &funindexstr[0]);
            strcat(armlabel, ":");
            appendOutBuffer(armlabel);

            /* Appends: ldmfd sp!, {r4, r5, pc} */
            appendOutBuffer(convention);
            //Push the argument in r0
            appendOutBuffer(pushr0);

            /* Compile the FUN until a return appears */
            while (strcmp(instrBuffer[i], returnstr) != 0){
                compileInstruction(&i, true);

                /* This condition captures the case that no return instr. is found */
                if (i > instrInd){
                    std::cout << "COMPILE ERROR: No return found in FUN.";
                    return -3;
                }
            }
            /* This block is reached when a return instruction is found. Append the return ARM mechanism. */
            appendOutBuffer(popr0);
            appendOutBuffer(endconvention);
        }
    }
    std::cout << "  done!\n";
    //ENDED PRECOMPILATION
	//Started main compilation
    char smain[12] = "main:";
    appendOutBuffer(smain);
    appendOutBuffer(convention);

    std::cout << "Starting effective compilation...\n";
	
	/* Define the FUN label index in 0 for compilation process. 
		note: The order functions should be stacked is the same order when they're defined, 
			so this index is related to funindex but not the same. And it's refered inside compileFunction because is global.*/
    funpushindex = 0;
    for (i = 0; i != instrInd;){
        compileInstruction(&i, false);
    }
	std::cout << " done!\n";
    /* Pop the result of the operation*/
	appendOutBuffer(popr4);
	
	/* Prepare printf(char* string, int result) call */
	char printedstring[100] = "	ldr r0, =string";
	char resultreg[100] = "	mov r1, r4";
	char invokeprintf[100] = "	bl printf";
	appendOutBuffer(printedstring);
	appendOutBuffer(resultreg);
	appendOutBuffer(invokeprintf);
	appendOutBuffer(endconvention);
	
	/* Data section containing first printf param*/
	char datasection[100] = ".data";
	char stringlabel[100] = "string:";
	char msg[100] = "	.asciz \"The result is: %d\\n\"";
	
	appendOutBuffer(datasection);
	appendOutBuffer(stringlabel);
	appendOutBuffer(msg);
	
	
	/* Now the outBuffer is ready with all instructions, export it to output file */
	std::cout << "Starting exporting outBuffer to \"outfile.s\"...\n";
	FILE *f = fopen("outfile.s", "w");
	
	i = 0;
	while (i != outInd){
		fprintf(f, "%s\n", outBuffer[i++]);
	}

	std::cout << "	done!\n";
	destroy_sexp(sx);
    fclose(fp);

    return 0;
}