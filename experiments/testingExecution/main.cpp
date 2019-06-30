#include "ConcolicExecutor.h"


/*


Remarci:
cand rulez cu sXXXX genereaza 6 TestCase dar la combination am doar 5
*/


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
	comm.writeInputDataTest("./testOutput.txt", vertices);
	delete(graph);
	delete(executor);
	

return 0;
}