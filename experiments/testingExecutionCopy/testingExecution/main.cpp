#include "Z3Interpretation.h"
#include "SimpleTracerComunicator.h"
#include <set> 
#include <list> 
#include "TestGraph.h"


#define SIMPLE_TRACER_ARGUMENTS "home/ceachi/testtools/simpletracer/river.tracer/river.tracer -p libfmi.so --annotated --z3 < ~/testtools/river/benchmarking-payload/fmi/sampleinput.txt"

/* PROBLEME
- nu merge sa execut din cmd cu input "A" sau altceva
- vad ca trb sa dau un input mare ca sa mearga

*/

std::vector<TestCase> callSimpleTracer(const char *testInput) {
	printf("now we call SimpleTracer with the Input : %s \n", testInput);
	pid_t child_pid, wpid;
	int status = 0;


	std::vector<TestCase> list;
	SimpleTracerComunicator communicator;

	if((child_pid = fork()) == 0) {
		// child call to simpleTracer
		communicator.writeInputData(testInput);
		int st = system("/home/ceachi/testtools/simpletracer/river.tracer/river.tracer -p libfmi.so --annotated --z3 < ~/testtools/river/benchmarking-payload/fmi/sampleinput.txt");
		exit(0);
	}
	list = communicator.readPipe();
	for(TestCase i : list) {
		printf("Z3_Code_Test : %s \n", i.Z3_code);
	}
	wait(0);
	return list;
	//while((wpid = wait(&status)) > 0);// wait for child process to finish
}

std::vector <string> getall(string k,int req){
    if(k.length()==req) return {k};
    std::vector <string> C;
    std::vector <string> D;
    k.push_back('0');
    D = getall(k,req);
    if(D.size()>0) C.insert(C.end(),D.begin(),D.end());
    k.pop_back();
    k.push_back('1');
    D = getall(k,req);
    if(D.size()>0) C.insert(C.end(),D.begin(),D.end());
    k.pop_back();
    return C;
}

std::vector <string> getAll_TestCases_Combinations(const char *rootInput, std::vector<TestCase> initialTestCases) {
	Z3Interpretation z3Interpretor;
	std::vector <string> newListInputs;

	for(TestCase i : initialTestCases) { 
			cout << i.Z3_code <<endl;
	}

	// get some combinations of {0,1}
    std::vector <string> s = getall("",initialTestCases.size()); // s = all the combination of {0,1}, {00000, 000010...}

	//for each combination of {0,1}
	for (std::vector<string>::const_iterator i = s.begin(); i != s.end(); ++i) {

		string newInput(rootInput);

		int pozition = 0;

		std::map<std::string, int> myMap;
        for (char const &c: *i) { // for each character in every string combination
			if(c == '0') { // make jump_symbol = 0
				std::string new_z3_code(initialTestCases.at(pozition).Z3_code);
				z3Interpretor.setJumpSymbol_zero(new_z3_code);

				// now get the new model results
				myMap = z3Interpretor.getModelResultsInDecimals(new_z3_code);
				// get the new Inputs from this model
				for(map<string,int>::const_iterator it = myMap.begin(); it != myMap.end(); ++it) {
					string operatorTest = (*it).first;
					if(operatorTest.at(0) == 's') { // select only the buffer element s[bufPozition]
						int bufPozition = (int) operatorTest.at(2) - 48;
						newInput[bufPozition] = (*it).second;
					}
				}


			}else if(c == '1') { // make jump_symbol = 1
				std::string new_z3_code(initialTestCases.at(pozition).Z3_code);
				z3Interpretor.setJumpSymbol_one(new_z3_code);
				
				cout <<"Z3_CODE" << endl; cout << new_z3_code <<endl;
				// now get the new model results
				myMap = z3Interpretor.getModelResultsInDecimals(new_z3_code);
				
				// get the new Inputs from this model
				for(map<string,int>::const_iterator it = myMap.begin(); it != myMap.end(); ++it) {
					string operatorTest = (*it).first;
					if(operatorTest.at(0) == 's') {
						int bufPozition = (int) operatorTest.at(2) - 48;//select only the buffer element s[bufPozition]
						newInput[bufPozition] = (*it).second;
					}
				}	
			}
			pozition++;
	    }
		
		newListInputs.push_back(newInput);
    }

	return newListInputs;

}




TestGraph* BF(string input) {
	TestGraph *graph = new TestGraph();
	vertex *root = graph->addVertex(input);

	queue<vertex*> Q;
	root->visited = true;
	Q.push(root);

	while(!Q.empty()) {
		vertex *x = Q.front();
		std::vector<TestCase> newTestCases = callSimpleTracer(x->data.c_str());
		std::vector <string> combinations = getAll_TestCases_Combinations(x->data.c_str(), newTestCases);

		for(string it : combinations) {
			vertex *child = graph->addVertex(it);
			if(child != nullptr) {
				graph->addEdge(x->data, child->data);
			}
		}

		cout <<endl;
		cout << x-> data << endl;
		Q.pop();

		for( vertex *v : x->neighbors) {
			if (v->visited == false) {
				v->visited = true;
				Q.push(v);
			}
			
	    }
	}

	return graph;
}


void writeInputDataTest(string input) {
   ofstream outfile;
   outfile.open("/home/ceachi/testtools/simpletracer/experiments/testingExecution/testOutput.txt");
   if(outfile.is_open()) {
	   printf("%s", "Writing to the file");
       outfile << input <<endl;
   }else {
	   printf("can't open file %s", input);
   }
   outfile.close();
}

int main() {

	string startInput = "XXXXXXXXXXXXXXXXXXXXXXXXXX";
	TestGraph *graph = BF(startInput);
	std::list<string> vertices = graph->breadthFirstSearch(startInput);
	for(string i : vertices) {
		cout << i << endl;
		//writeInputDataTest(i);
	}
	//graph->testDisplay();
	/*

	std::vector<TestCase> list_of_TestCases = callSimpleTracer("XXXXXXXXXXXXXXXXXXXXXXXXXX");
	ExecutionTest execTest;
	//strcpy(execTest.input, "A");
	execTest.input = "XXXXXXXXXXXXXXXXXXXXXXXXXX";
	execTest.list_TestCase = list_of_TestCases;

	std::vector <string>  newExecutionTest  = getAll_TestCases_Combinations(execTest.input, execTest.list_TestCase);
	for(auto i : newExecutionTest) {
		printf("%s \n", i.c_str());
	}
	printf("ceva");
	*/

  /*  
	std::vector<TestCase> list;
	//std::vector<ExecPath> historyofExecution;

	Z3Interpretation z3Interpretor;
	SimpleTracerComunicator communicator;

	pid_t pid = fork();


	if(pid == 0) {
		communicator.writeInputData("XXXXXXXXXXXXXXXXXXXXXXXXXX");
		int status = system("/home/ceachi/testtools/simpletracer/river.tracer/river.tracer -p libfmi.so --annotated --z3 < ~/testtools/river/benchmarking-payload/fmi/sampleinput.txt");
	}else {
		list = communicator.readPipe();
		wait(0);


		printf("%s ", list[0].Z3_code);
		printf("%s ", list[1].Z3_code);
		printf("%s ", list[2].Z3_code);
		
		//ExecPath newExecPath;
		//newExecPath.input = communicator.readInputData("/home/ceachi/testtools/river/benchmarking-payload/fmi/sampleinput.txt");
		//newExecPath.list_TestCase = list;
	//	historyofExecution.push_back(newExecPath);
		
		for(std::vector<TestCase>::iterator testCase = list.begin(); testCase <= list.end(); testCase++) {
			communicator.testCase_to_String(testCase);
			std::string z3_code(testCase->Z3_code);
			//z3Interpretor.setJumpSymbol_one(z3_code);

			// get model results
			//std::map<std::string, int>  model = z3Interpretor.getModelResultsInDecimals(z3_code);
			std::map<std::string, int> false_branch_model = z3Interpretor.get_Values_For_False_Branch(z3_code);
			std::map<std::string, int> true_branch_model = z3Interpretor.get_Values_For_True_Branch(z3_code);


		}
	


	}
	*/
return 0;
}
