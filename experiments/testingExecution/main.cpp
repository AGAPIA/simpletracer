#include "ConcolicExecutor.h"





void writeInputDataTest(std::list<string> inputs) {
	const char * pathToFile = "/home/ceachi/testtools/simpletracer/experiments/testingExecution/testOutput.txt";
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

int main() {
	string startInput = "XXXXXXX";
	ConcolicExecutor *executor = new ConcolicExecutor();
	TestGraph *graph = executor->GenerateExecutionTree(startInput);

	std::list<string> vertices = graph->BreadthFirstSearch(startInput);
	for(string i : vertices) {
		cout << i << endl;

	}

	writeInputDataTest(vertices);
	delete(graph);
return 0;
}
