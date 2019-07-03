#include "ConcolicExecutor.h"

#define file_Results "./testOutput.txt"
int main() {
	char *startInput = "XXXXXXXXX";
	ConcolicExecutor *executor = new ConcolicExecutor();
	TestGraph *graph = executor->GenerateExecutionTree(startInput);

	std::list<char *> vertices = graph->BreadthFirstSearch(startInput);
	
	printf("Results are : \n");
	 for(auto i : vertices) {
		  printf("printing : %s \n", i);
	  }
	FileHandling comm;
	comm.writeInputDataTest(file_Results, vertices);
	delete(graph);
	delete(executor);
	

return 0;
}