#include<iostream>
#include<string>
#include <fstream>

using namespace std;

#define MAX_PATH 260

struct TestCaseBase {
			unsigned int offset;
			char modName[MAX_PATH];
		};

struct TestCase2 {
			struct TestCaseBase bbp;
			unsigned int cost;
			unsigned int jumpType;
			unsigned int jumpInstruction;
			unsigned int esp;
			unsigned int nInstructions;
			unsigned int bbpNextSize;
			struct TestCaseBase nextBasicBlockPointer;
			unsigned long instructionAddress;
			char Z3_code[10000];
};


int main() {

	TestCase2 test;
	
	test.cost = 20;
	test.jumpType = 100;
	test.jumpInstruction = 200;
	test.esp = 300;
	test.nInstructions = 700;
	test.bbpNextSize = 900;

	TestCase2 test2;

	test2.cost = 30;
	test2.jumpType = 40;
	test2.jumpInstruction = 50;
	test2.esp = 60;
	test2.nInstructions = 70;
	test2.bbpNextSize = 80;
	test2.instructionAddress = 90;
	
    FILE *outfile; 
    // open file for writing 
    outfile = fopen ("./sharedFile", "w"); 
    if (outfile == NULL)  { 
        fprintf(stderr, "\nError opend file\n"); 
    } 
  
     
    // write struct to file 
    fwrite (&test, sizeof(struct TestCase2), 1, outfile); 
    fwrite (&test2, sizeof(struct TestCase2), 1, outfile); 
      
    if(fwrite != 0)  
        printf("contents to file written successfully !\n"); 
    else 
        printf("error writing file !\n"); 
  
    // close file 
    fclose (outfile); 
return 0;
}

