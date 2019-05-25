#include "ConcolicExecutor.h"



int main() {
	string startInput = "XXX";
	ConcolicExecutor *executor = new ConcolicExecutor();
	TestGraph *graph = executor->GenerateExecutionTree(startInput);

	std::list<string> vertices = graph->BreadthFirstSearch(startInput);
	

	FileHandling comm;
	comm.writeInputDataTest("/home/ceachi/testtools/simpletracer/experiments/testingExecution/testOutput.txt", vertices);
	delete(graph);
	delete(executor);

return 0;
}
