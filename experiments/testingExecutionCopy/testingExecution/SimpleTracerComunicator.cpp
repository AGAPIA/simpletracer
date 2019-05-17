#include "SimpleTracerComunicator.h"

std::vector<TestCase> SimpleTracerComunicator::readPipe() {
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
					list.at(count).Z3_code[0] = ' '; // eliminate ';' character
					count++;
				}
		} while (num > 0);

		// PROBLEM: se pare ca mai citeste ultimul element din pipe inca o data
		list.pop_back();
		close(fileDescriptor);

		return list;
}


char * SimpleTracerComunicator::readInputData(char *filePath) {
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





void SimpleTracerComunicator::writeInputData(const char *input) {
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


void SimpleTracerComunicator::testCase_to_String(vector<TestCase>::iterator testCase) {
	unsigned int cost;
			unsigned int jumpType;
			unsigned int jumpInstruction;
			unsigned int esp;
			unsigned int nInstructions;
			unsigned int bbpNextSize;
	printf("bbp -> [offset = {%#08x}  , modName = {%s}] cost ->[%d] , jumpType -> [%d] , jumpInstruction -> [%d], esp -> [%#08x] , nInstructions -> [%#08x], bbpNextSize -> [%#08x], nextBasicBlockPointer -> [offset = {%#08x}  , modname = {%s}], z3_code -> \n %s \n",testCase-> bbp.offset, testCase->bbp.modName,
	testCase->cost, testCase->jumpType, testCase->jumpInstruction,
	testCase->esp, testCase->nInstructions, testCase->bbpNextSize,
	testCase->nextBasicBlockPointer.offset,testCase->nextBasicBlockPointer.modName,
	testCase->Z3_code);

}	