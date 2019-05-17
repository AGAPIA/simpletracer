#include <stdio.h> 
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h> 
#include <unistd.h> 
#include <stdlib.h>
#include <vector>
#include "z3/include/z3.h"
#include <fstream>

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

struct ExecPath {
	char *input;
	std::vector<TestCase> list_TestCase;
};



std::vector<TestCase> readPipe() {
	std::vector<TestCase> list;
	int num, fileDescriptor;
	printf("parent - waiting for writers...\n");
		if ((fileDescriptor = open(myFifo, O_RDONLY)) < 0)
			perror("parent - open");
		// reading the struct
		int count = 0;
		do {
				TestCase x;
				if ((num = read(fileDescriptor, &x, sizeof(TestCase))) < 0) {

					perror("parent - read");
				}else {
					list.push_back(x);
					//list.at(count).Z3_code[0] = ' '; // eliminate ';' character
					count++;
				}
		} while (num > 0);
		close(fileDescriptor);

		return list;
}


char * readInputData(char *filePath) {
	FILE *fileStream; 
	char *buffer = new char((100 * sizeof(char)));
	fileStream = fopen (filePath, "r"); 
	if(fileStream == NULL) {
		printf("file %s not found", filePath);
	}else {
		fgets (buffer, 100, fileStream); 

	}
	fclose(fileStream);
	return buffer;
}



void getModel() {
	Z3_config config = Z3_mk_config();
    Z3_context context = Z3_mk_context(config);
	Z3_solver solver = Z3_mk_solver(context);
	const char *result2 = "(declare-fun jump_symbol () (_ BitVec 1)) (declare-fun |s[0]| () (_ BitVec 8)) (assert (= (ite (= |s[0]| (_ bv65 8)) (_ bv1 1) (_ bv0 1)) jump_symbol))";

	 Z3_ast fs = Z3_parse_smtlib2_string(context, result2, 0, 0, 0, 0, 0, 0);
	 Z3_solver_assert(context, solver, fs);
	 int result_solver = Z3_solver_check ((Z3_context) context, (Z3_solver) solver);
	Z3_model model = Z3_solver_get_model ((Z3_context) context,  (Z3_solver) solver);
	printf("Model : %s\n", Z3_model_to_string ((Z3_context) context,  model));
	
	unsigned n = Z3_model_get_num_consts(context, model);
		for(unsigned i = 0; i < n; i++) {
			//unsigned Z3_api rezultat = Z3_model_get_num_funcs_decl(context, model, i);
			Z3_func_decl fd = Z3_model_get_const_decl(context, model, i);

			if (Z3_model_has_interp(context, model, fd))
			{
				Z3_ast s = Z3_model_get_const_interp(context, model, fd);
				printf("%s \n", Z3_ast_to_string(context, s));
			}
				
  			//Z3_ast litle_assert =  Z3_model_get_func_interp(context, model, fd);
			printf("%s \n ",Z3_func_decl_to_string (context, fd));
		}
}


void writeInputData(char *input) {
	// open a file in write mode.
   ofstream outfile;
   outfile.open(INPUT_FILE_PATH);
   if(outfile.is_open()) {
	   printf("%s", "Writing to the file");
       outfile << input;
   }else {
	   printf("can't open file %s", input);
   }
   outfile.close();

}

int main() {

	
    	
	std::vector<TestCase> list;
	std::vector<ExecPath> historyofExecution;

	pid_t pid = fork();


	if(pid == 0) {
		writeInputData("ABABAB");
		int status = system("/home/ceachi/testtools/simpletracer/river.tracer/river.tracer -p libfmi.so --annotated --z3 < ~/testtools/river/benchmarking-payload/fmi/sampleinput.txt");
	}else {
		list = readPipe();
		wait(0);

		printf("%s ", list[0].Z3_code);
		
		ExecPath newExecPath;
		newExecPath.input = readInputData("/home/ceachi/testtools/river/benchmarking-payload/fmi/sampleinput.txt");
		newExecPath.list_TestCase = list;
		historyofExecution.push_back(newExecPath);

		printf("haleluia");
		//getModel();
}
return 0;
}
