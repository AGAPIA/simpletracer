#include "FileHandling.h"


char * FileHandling::readInputData(char *filePath) {
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





void FileHandling::writeInputData(const char *input) {
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



void FileHandling::writeInputDataTest(const char* pathToFile, std::list<string> inputs) {
   ofstream outfile;
   outfile.open(pathToFile, std::ofstream::out | std::ofstream::trunc); // open and clear previos data
   if(outfile.is_open()) {
	   printf("%s", "Writing to the file");

	   for(string i : inputs) {
      	 outfile << i <<endl;
	   }
   }else {
	   printf("can't open file %s", pathToFile);
   }
   outfile.close();
}
