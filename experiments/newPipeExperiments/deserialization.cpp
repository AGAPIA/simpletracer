#include<iostream>
#include<string>
#include <fstream>
#include <list>
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

	std::list<TestCase2> testList;

    FILE *infile; 
    struct TestCase2 input; 
      
    // Open person.dat for reading 
    infile = fopen ("./sharedFile", "r"); 
    if (infile == NULL)  { 
        fprintf(stderr, "\nError opening file\n"); 
    } 
      
    // read file contents till end of file 
    while(fread(&input, sizeof(struct TestCase2), 1, infile)) 
        printf("test.cost = %d \n", input.cost);
  
    // close file 
    fclose (infile); 
return 0;
}

