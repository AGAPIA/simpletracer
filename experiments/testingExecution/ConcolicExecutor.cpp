#include "ConcolicExecutor.h"
#include "Z3Interpretation.h"



// used for signals between process
int ConcolicExecutor::msReceived = 0;

// used for signals between process
void ConcolicExecutor::SignalHandler(int sig) {
    if (sig == SIGUSR1) {
        msReceived = 1;
    }
}


std::vector<TestCase> ConcolicExecutor::readPipe() {
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

std::vector<TestCase> ConcolicExecutor::CallSimpleTracer(pid_t child_pid, const char *testInput) {

	/* How this function works:
	- we know that child_pid, is a pid from a process that call simpleTracerAppliation with an testInput
	- here we specify what input to use to call simpleTracer
	- after that we signal this process to execute simpleTracer and write output in a pipe
	- then we read the results from the pipe in the parent


	*/

	printf("now we call SimpleTracer with the Input : %s \n", testInput);
    std::vector<TestCase> list;
	FileHandling communicator;
	communicator.writeInputData(testInput);
	kill(child_pid,SIGUSR1);// send signal to child process
	list = this->readPipe();


	for(TestCase i : list) {
		//printf("Z3_Code_Test : %s \n", i.Z3_code);
		testCase_to_String(i);
	}
	

	return list;
}

/*
how to call: k = "", req = 1,2,3...
will return all the combinations of 000, 001, 010...
*/
std::vector <string> ConcolicExecutor::GenerateCombinations(string k,int req) {
    if(k.length()==req) return {k};
        std::vector <string> C;
        std::vector <string> D;
        k.push_back('0');
        D = GenerateCombinations(k,req);
        if(D.size()>0) C.insert(C.end(),D.begin(),D.end());
        k.pop_back();
        k.push_back('1');
        D = GenerateCombinations(k,req);
        if(D.size()>0) C.insert(C.end(),D.begin(),D.end());
        k.pop_back();
        return C;
}

// this method will take a vector of TestCases, let say s[0], s[1]
// then using method GenerateCombinations, will generate all combinations of s[0]s[1], being true or false
// as a result, a vector of new inputs to call simpleTracer
std::vector <string> ConcolicExecutor::GenerateTestCases(const char *rootInput, std::vector<TestCase> initialTestCases) {
	/*
		How this method works: 
		- first it will take some TestCases as inputs, for example s[0], s[1]
		- then we generate all the combinations with them for example 00, 01, 10, 11
		- then we generate newTestCases with them based on the combinations,
		- if 0 then, s[0] will have jum_symbol = 0, then we get the model
		- after we get the model, we get the new values for s[0]. s[1], and we modify the string rootInput
		that generate the initialTestCases in the first place

		- at the end of this method, we will get a vector of new inputs based of the combinations of s[0], s[1]...



	*/
    if(initialTestCases.size() == 0) {
			return {};
	}
	Z3Interpretation z3Interpretor;
	std::vector <string> newListInputs;

	// get some combinations of {0,1}
    std::vector <string> s = GenerateCombinations("",initialTestCases.size()); // s = all the combination of {0,1}, {00000, 000010...}

	printf("Afisam testele peste care facem combinatii \n");
	for(TestCase t : initialTestCases) {
		testCase_to_String(t);
	}

	//for each combination of {0,1}
	for (std::vector<string>::const_iterator i = s.begin(); i != s.end(); ++i) {

		// we crate a copy of the inputString
		string newInput(rootInput);

		// position means, the position of element inside std::vector<TestCase>
		int pozition = 0;
   
		std::map<std::string, int> myMap;
      for (char const &c: *i) { // for each character in every string combination
			if(c == '0') { // make jump_symbol = 0
				std::string new_z3_code(initialTestCases.at(pozition).Z3_code); // we take the Z3_Code form a testcase at position = ?
				z3Interpretor.setJumpSymbol_zero(new_z3_code); // we set jump_symbol = 0 to that Z3_code

				// now get the new model results
				myMap = z3Interpretor.getModelResultsInDecimals(new_z3_code);
				// get the new Inputs from this model
				for(map<string,int>::const_iterator it = myMap.begin(); it != myMap.end(); ++it) {
					string operatorTest = (*it).first;
					if(operatorTest.at(0) == 's') { // select only the buffer element s[bufPozition]
						if((*it).second == 0) continue; // means that the value is null so we keep original value
						int bufPozition = (int) operatorTest.at(2) - 48;
						newInput[bufPozition] = (*it).second; // modify the copy of InputString, sot that newInput[poz] = value of s[i]
					}
				}

	// the same principle apply when we encounter 1
			}else if(c == '1') { // make jump_symbol = 1
				std::string new_z3_code(initialTestCases.at(pozition).Z3_code);
				z3Interpretor.setJumpSymbol_one(new_z3_code);
				
				cout <<"Z3_CODE" << endl; cout << new_z3_code <<endl;
				// now get the new model results
				myMap = z3Interpretor.getModelResultsInDecimals(new_z3_code);
				
				// get the new Inputs from this model
				for(map<string,int>::const_iterator it = myMap.begin(); it != myMap.end(); ++it) {
					string operatorTest = (*it).first;
					if(operatorTest.at(0) == 's') {// select only the buffer element s[bufPozition]
						if((*it).second == 0) continue;// means that the value is null so we keep original value
						int bufPozition = (int) operatorTest.at(2) - 48;//select only the buffer element s[bufPozition]
						
						newInput[bufPozition] = (*it).second;
					}
				}	
			}
			pozition++;
	    }
		
		// after we have a new string, we put it inside a list that will be returned at the end
		newListInputs.push_back(newInput);
    }

	return newListInputs;

}
std::vector<TestCase> ConcolicExecutor::FindDiferencesWithParent(Vertex *child) {

	/*
	- This method will take a vertex (node) from a TestGraph
	- we know that each vertex has a list of TestCases
	- we compare input vertex  TestCases with he's parent TestCases
	- and return those TestCases that are different

	*/
    	Vertex *parent = child->predecesor;
		if(parent == NULL) {
			return child->testCaseList;
		}

		std::vector<TestCase> differences;


		for(int i = 0; i < child->testCaseList.size(); i++) {
			bool found = false;
			for(int j = 0; j < parent->testCaseList.size(); j++) {
					char *child_z3_code = child->testCaseList.at(i).Z3_code;
					char *parent_z3_code = parent->testCaseList.at(j).Z3_code;

					int compare = strcmp(child_z3_code, parent_z3_code);

					printf("NOW WE COMPARE parent with child : \n");
					printf("parent_z3_code \n %s \n", parent_z3_code);
					printf("child z3_code : \n %s \n", child_z3_code);

					if(compare == 0) {
						found = true;
						continue;
					}
				}
				if(found == false) {
					differences.push_back(child->testCaseList.at(i));
				}
		}

		return differences;


}


std::vector<TestCase> ConcolicExecutor::GetOnlyLoopTestCases(std::vector<TestCase> testCases) {
	set<unsigned long> mySet;
	std::vector<unsigned long> orderOfInstructionAddress;
	for( unsigned i = 0; i <  testCases.size(); ++i ) {
		if(mySet.count(testCases[i].instructionAddress) == 0) {
			// instructionAddress not in set
			orderOfInstructionAddress.push_back(testCases[i].instructionAddress);
		}
		mySet.insert( testCases[i].instructionAddress);
	}

	for(unsigned long i : orderOfInstructionAddress) {
		printf("Instruction set :  %08lx \n", i);
	}



	// TO DO: momentan eu iau cu primele valori gasite, ideea este sa iau cu ultimile
	// sau cu una de la mijloc

	std::vector<TestCase> newTestCase;

	int cnt = 0;
	while(cnt < orderOfInstructionAddress.size()) {

		for(TestCase j : testCases) {
			if(cnt >= orderOfInstructionAddress.size()) break;
			if(j.instructionAddress == orderOfInstructionAddress[cnt]) {
				newTestCase.push_back(j);
				cnt++;
			}
		}
	}


	return newTestCase;
}

/*
This method is the root of this application. It will generate the Graph in a Bf way

*/
TestGraph* ConcolicExecutor::GenerateExecutionTree(string start_input) {

	/* How this method works
	- it works like a BF-algorithm
	- it will have a child Process, that every time a signal will trigger, it will call simpleTracer
	- at the end of this method, the child process will be killed

	*/

    TestGraph *graph = new TestGraph();
	Vertex *root = graph->AddVertex(start_input);

	queue<Vertex*> Q;
	root->visited = true;
	Q.push(root);


	pid_t child_pid, wpid;
	signal(SIGUSR1,this->SignalHandler);
	if((child_pid = fork()) == 0) {
		int parent_pid = getppid();
			while(1) {
					while (!msReceived);
					int st = system("/home/ceachi/testtools/simpletracer/river.tracer/river.tracer -p libfmi.so --annotated --z3 < ~/testtools/river/benchmarking-payload/fmi/sampleinput.txt");
					msReceived = 0;
				}
	}



	while(!Q.empty()) {
		Vertex *x = Q.front();
		std::vector<TestCase> newTestCases = CallSimpleTracer(child_pid, x->data.c_str());
		graph->AddVertexTestCases(x->data, GetOnlyLoopTestCases(newTestCases)); // add the TestCases after the call of simpleTracer, and add them to the vertex
		std::vector<TestCase> differentTestCases = FindDiferencesWithParent(x); // find the TestCases between this vertex and he's parent
		cout << "DIFERENCES WITH THE PARENT " <<endl;
		for(TestCase i : differentTestCases) {
				testCase_to_String(i); cout << endl;
					//printf("Z3_Code_Test : %s \n", i.Z3_code);
		}


		std::vector <string> combinations = GenerateTestCases(x->data.c_str(), differentTestCases); // generate all the combinations with the new TestCases

	 cout << "combinations with the string : " << x->data.c_str() << endl;
		for(string i : combinations) {
			cout << i << " ";
		} cout<<endl;

		// for every combinations of strings, we create new vertex and add them ass childs for vertex x
		for(string it : combinations) {
			Vertex *child = graph->AddVertex(it);
			if(child != nullptr) {
				graph->AddEdge(x->data, child->data);
			}
		}

		cout <<endl;
		cout << x-> data << endl;
		Q.pop();


		// here we mark the vertex that we already visited
		for( Vertex *v : x->neighbors) {
			if (v->visited == false) {
				v->visited = true;
				Q.push(v);
			}
			
	    }
	}
	kill(child_pid, SIGKILL);


	return graph;
}



void ConcolicExecutor::testCase_to_String(TestCase &testCase) {
	unsigned int cost;
			unsigned int jumpType;
			unsigned int jumpInstruction;
			unsigned int esp;
			unsigned int nInstructions;
			unsigned int bbpNextSize;
	printf("bbp -> [offset = {%#08x}  , modName = {%s}] cost ->[%d] , jumpType -> [%d] , jumpInstruction -> [%d], esp -> [%#08x] , nInstructions -> [%#08x], bbpNextSize -> [%#08x], nextBasicBlockPointer -> [offset = {%#08x}  , modname = {%s}], \n test - %08lx \n z3_code -> \n %s \n",testCase.bbp.offset, testCase.bbp.modName,
	testCase.cost, testCase.jumpType, testCase.jumpInstruction,
	testCase.esp, testCase.nInstructions, testCase.bbpNextSize,
	testCase.nextBasicBlockPointer.offset,testCase.nextBasicBlockPointer.modName,
	testCase.instructionAddress,
	testCase.Z3_code);

}	