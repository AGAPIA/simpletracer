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
#include<list>

using namespace std;

#define INPUT_FILE_PATH "/home/ceachi/testtools/river/benchmarking-payload/fmi/sampleinput.txt"
class FileHandling {
	public :
			char * readInputData(char *filePath);
			void writeInputData(unsigned char *input);
			void writeInputDataTest(const char* filePath, std::list<char *> inputs);
};