#include <vector>
#include <stdio.h> 
#include <fstream>
#include <stdlib.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h> 
#include <unistd.h> 

using namespace std;


#define myFifo "/tmp/fifochannel"// FIFO file path 
#define MAX_PATH 260 // if you want to change this, you have to change MAX_PATH in simple.tracer coresponding to TestCase.Z3_code
#define MAX_Z3_CODE 10000
#define INPUT_FILE_PATH "/home/ceachi/testtools/river/benchmarking-payload/fmi/sampleinput.txt"

struct TestCaseBase {
			unsigned int offset;
			char modName[MAX_PATH];
		};
struct TestCase {
			struct TestCaseBase bbp;
			unsigned int cost;
			unsigned int jumpType;
			unsigned int jumpInstruction;
			unsigned int esp;
			unsigned int nInstructions;
			unsigned int bbpNextSize;
			struct TestCaseBase nextBasicBlockPointer;
			char Z3_code[MAX_Z3_CODE];
	};

struct ExecutionTest {
	char *input;
	std::vector<TestCase> list_TestCase;
};


class SimpleTracerComunicator {
	public :
			std::vector<TestCase> readPipe();
			char * readInputData(char *filePath);
			void writeInputData(const char *input);
			void testCase_to_String(vector<TestCase>::iterator testCase);
};
