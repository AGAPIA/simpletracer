#include "ConcolicExecutor.h"



int main() {
	char *startInput = "XXX";
	ConcolicExecutor *executor = new ConcolicExecutor();
	TestGraph *graph = executor->GenerateExecutionTree(startInput);

	std::list<char *> vertices = graph->BreadthFirstSearch(startInput);
	
/*
	FileHandling comm;
	comm.writeInputDataTest("/home/ceachi/testtools/simpletracer/experiments/testingExecution/testOutput.txt", vertices);
	delete(graph);
	delete(executor);
	*/

return 0;
}