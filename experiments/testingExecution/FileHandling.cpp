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





void FileHandling::writeInputData(unsigned char *input) {
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



void FileHandling::writeInputDataTest(const char* pathToFile, std::list<char *> inputs) {
  FILE *fout = fopen(pathToFile, "w");
  if(fout != NULL) {
	  for(auto i : inputs) {
		  fprintf(fout, "%s\n", i);
	  }
  }
  fclose(fout);
}