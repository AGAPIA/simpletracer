#include "ConcolicExecutor.h"
#include "Z3Interpretation.h"

#include <spawn.h>
#include <sys/wait.h>

std::vector<TestCase> ConcolicExecutor::getResultsFromSimpleTracer() {
	/*
		How it works:
			- we get the results from execution of simpleTracer app inside the file SIMPLE_TRACER_EXECUTION_RESULTS_FILE
			- we read them and return results as a TestCase struct

	*/
		std::vector<TestCase> list;
		FILE *infile; 
    	struct TestCase input; 
		infile = fopen (SIMPLE_TRACER_EXECUTION_RESULTS_FILE, "r"); 
		if (infile == NULL)  { 
        	fprintf(stderr, "\nError opening file\n"); 
   		 }

		int count = 0;
		while(fread(&input, sizeof(struct TestCase), 1, infile)) {
			list.push_back(input);
			list.at(count).Z3_code[0] = ' '; // there is an empty space, and we eliminate it
			count++;
		}
		return list;
}


std::vector<TestCase> ConcolicExecutor::CallSimpleTracer(unsigned char *testInput) {

	/* How this function works:
	- we call simple tracer with the testInput parameter
	- then we obtain the results from simple tracer and return as TestCases
	*/

	printf("now we call SimpleTracer with the Input : %s \n", testInput);
    std::vector<TestCase> simpleTracerResultsList;
	
	printf("System call to RIVER is starting \n");
	

	// preparing the command with the testInput to execute simpleTracer app
	std::string command("python -c 'print \"");
	command.append((char *)testInput);
	command.append("\"' | river.tracer -p libfmi.so --annotated --z3");


	// here we execute simple-tracer with the command that we build
	int st = system(command.c_str());

	printf("System call to RIVER finished \n");	
	simpleTracerResultsList = this->getResultsFromSimpleTracer();
	

    WriteSimpleTracerResultsInsideFolder(simpleTracerResultsList, "./trace.simple.txt");
	
/*
	for(TestCase i : simpleTracerResultsList) {
		testCase_to_String(i);
	}
*/
	//std::system("clear"); // this is just for clearing the console
	return simpleTracerResultsList;
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


void ConcolicExecutor::GetPermutationWithDuplicates(int n,  std::vector<int> output, std::size_t k,  std::vector<std::vector<int>> &rezult) {
    // for example: n = 4 k = 3   111 112 113 114 121 122 123....
    if( output.size() == k ) {
         rezult.push_back(output);
    }else {
        for(std::size_t i = 1; i <= n; ++i) {
           
            std::vector<int> temp_output = output ; // we do not want to modify output
            temp_output.push_back(i) ;
            GetPermutationWithDuplicates( n, temp_output, k, rezult);
        }
    }
 }
bool ConcolicExecutor::compareByIndex(TestCase t1, TestCase t2) {
		char *bufIndexValue1 = strstr(t1.Z3_code, "s[");// for example s[1] => bufIndexValue = 1 from Z3_code
		char *bufIndexValue2 = strstr(t2.Z3_code, "s[");
		if(bufIndexValue1 != nullptr && bufIndexValue2 != nullptr) {
				if(bufIndexValue1[2] == bufIndexValue2[2]) {
					return true;
				}else return false;

		}else {
			printf("There is no s[i] inside Z3_code");
		}
}
void ConcolicExecutor::GetDuplicateDistinctTestCase(std::vector<TestCase> initialTestCases, std::vector<std::vector<TestCase>> &duplicateTestCases, std::vector<TestCase> &distinctTestCases) {
	duplicateTestCases.push_back(vector<TestCase>()); // push an empty vector to initialize this type of structure
	
	std::sort(initialTestCases.begin(), initialTestCases.end(), compareByIndex); // sort array initialTestCase by TestCase.Z3_Code

	if(initialTestCases.size() == 0 || initialTestCases.size() == 1) {
		distinctTestCases.push_back(initialTestCases[0]);
	}

	bool previousTainted = false;
	int j = 0; // j = means the s[j] pair of duplicates in matrix
	for(std::size_t i = 0; i < initialTestCases.size(); ++i) {

		if(i == initialTestCases.size() - 1) {
			if(previousTainted == true) {
				duplicateTestCases[j].push_back(initialTestCases[i]);
				break;
			}else {
				if(duplicateTestCases[j].size() == 0) duplicateTestCases.erase(duplicateTestCases.begin() + j); // if you have only 1 pari s[j] of duplicates, in the matrix you will get the last s[j] empty
				distinctTestCases.push_back(initialTestCases[i]);
				break;
			}
		}

		if(compareByIndex(initialTestCases[i], initialTestCases[i + 1]) == true) {
				duplicateTestCases[j].push_back(initialTestCases[i]);
				previousTainted = true;
		}else {
			if(previousTainted == true) {
				duplicateTestCases[j].push_back(initialTestCases[i]);
				j++;
				duplicateTestCases.push_back(vector<TestCase>());
				previousTainted = false;
			}else {
				distinctTestCases.push_back(initialTestCases[i]);
			}
		}
	}
}


// this method will take a vector of TestCases, let say s[0], s[1]
// then using method GenerateCombinations, will generate all combinations of s[0]s[1], being true or false
// as a result, a vector of new inputs to call simpleTracer
std::vector <char *> ConcolicExecutor::GenerateTestCases(char *rootInput, std::vector<TestCase> initialTestCases) {
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
	std::vector <char *> newListInputs;

	// get some combinations of {0,1}
    std::vector <string> s = GenerateCombinations("",initialTestCases.size()); // s = all the combination of {0,1}, {00000, 000010...}

	printf("Afisam testele peste care facem combinatii \n");
	for(TestCase t : initialTestCases) {
		testCase_to_String(t);
	}

	//for each combination of {0,1}
	for (std::vector<string>::const_iterator i = s.begin(); i != s.end(); ++i) {

		// we crate a copy of the inputString
		//string newInput(rootInput);
		char *newInput = (char*)malloc (strlen(rootInput) * sizeof (char));
		strcpy(newInput, rootInput);

		// position means, the position of element inside std::vector<TestCase>
		int pozition = 0;
   
		std::map<std::string, int> myMap;
      for (char const &c: *i) { // for each character in every string combination
			if(c == '0') { // make jump_symbol = 0
				std::string new_z3_code(initialTestCases.at(pozition).Z3_code); // we take the Z3_Code form a testcase at position = ?


				if(new_z3_code.find("jump_symbol") != string::npos) {

					z3Interpretor.setJumpSymbol_zero(new_z3_code); // we set jump_symbol = 0 to that Z3_code
				}else {
					// usualy there is address_symbol, RIver make some optimization in assembly
					// for now we ignore it
					// usualy, when you see address_symbol, and you have a loop, means that your C code, inside the if statement,
					// don't depent of a global variable
					continue;
				}


				// now get the new model results
				myMap = z3Interpretor.getModelResultsInDecimals(new_z3_code);

				if(myMap.size() == 0) {
					// this means that Z3_CODE return unsat
					printf("THE FOLLOWING Z3 CODE is unsat : \n  %s \n", new_z3_code.c_str());
					continue;
				}

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

				if(new_z3_code.find("jump_symbol") != string::npos) {

					z3Interpretor.setJumpSymbol_one(new_z3_code); // we set jump_symbol = 1 to that Z3_code
				}else {
					// usualy there is address_symbol, RIver make some optimization in assembly
					// for now we ignore it
					continue;
				}

				
				
				cout <<"Z3_CODE" << endl; cout << new_z3_code <<endl;
				// now get the new model results
				myMap = z3Interpretor.getModelResultsInDecimals(new_z3_code);

				if(myMap.size() == 0) {
					// this means that Z3_CODE return unsat
					printf("THE FOLLOWING Z3 CODE is unsat : \n  %s \n", new_z3_code.c_str());
					continue;
				}
				
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



std::vector <char *> ConcolicExecutor::GenerateZ3TestCases(char *rootInput, std::vector<TestCase> initialTestCases) {

    if(initialTestCases.size() == 0) {
			return {};
	}

	std::vector<std::vector<TestCase>> matrixDuplicateTestCases; // we hold a Matrix with DuplicateTestCases (not by Id, but for Z3_code string)
	// matrixDuplicateTestCases will look like: matrixDuplicateTestCases[i] --> will have only the TestCases with index i, s[i] for example i = 0, s[0] s[0]
	std::vector<TestCase> distinctTestCases;
	GetDuplicateDistinctTestCase(initialTestCases, matrixDuplicateTestCases, distinctTestCases); // from initialTestCases, we get the duplicates and the distinct ones

	if(matrixDuplicateTestCases.size() == 1 && matrixDuplicateTestCases[0].size() == 0) {
		// this means that we did not found any TestCases duplicate(not by id) inisde initialTestCases
		return GenerateTestCases(rootInput, initialTestCases); // so we generate new TestCases based on distinct ones
	}


	// this is a particulare case:
	if(matrixDuplicateTestCases.size() == 1) { // for example, you have the situation with TestCases s[0] s[0] s[1], so there are only 1 pair of duplicates
		std::vector <char *> newInputTests;
			for(auto testCase : matrixDuplicateTestCases[0]) { // for each duplicate TestCases  ( matrix will have only 1 in this case. so it's [0])
				std::vector<TestCase> newTestCases; // we create a new TestCase vector
				newTestCases.push_back(testCase);//push the first one, then second one on next iteration
				// combine each of them with the distinct TestCases

				if(distinctTestCases.size() != 0) {
					for(auto test : distinctTestCases) {
						newTestCases.push_back(test);
					}
				}
				//get the new results
				//char *newRootInput;
				//strcpy(newRootInput, rootInput);
				std::vector <char *> newRezults = GenerateTestCases(rootInput, newTestCases);
				for(auto newTest : newRezults) {
					newInputTests.push_back(newTest);
				}
				newTestCases.clear();	
		}
		return newInputTests;
	}

	// after you get the duplicates, find the max number of duplications
	// for example: a[1] have 4 elements, a[2] have 5 , so you find max = 5
	int max = 0;
	for(int i = 0; i < matrixDuplicateTestCases.size(); i++) {
		if(matrixDuplicateTestCases[i].size() > max) {
			max = matrixDuplicateTestCases[i].size();
		}
	}

// with the max you found, we generate all the permutations
// generate all permutations of n k for example: n = 2 k = 3 111 112 121 122..
	int n = max;
	int k = matrixDuplicateTestCases.size();
	std::vector<std::vector<int>> permutationPairWithDuplicates;
	GetPermutationWithDuplicates(n, {}, k, permutationPairWithDuplicates);

// erase unnecesary permutations
	for(int i = 0; i < permutationPairWithDuplicates.size(); i++) {
		for(int j = 0; j < permutationPairWithDuplicates[i].size(); j++) {
			if(permutationPairWithDuplicates[i][j] > permutationPairWithDuplicates[j].size()) {
				permutationPairWithDuplicates.erase(permutationPairWithDuplicates.begin() + i);
				i--;
				break;
			}
		}
	}


	std::vector <char *> newInputTests;
	if(permutationPairWithDuplicates.size() != 0) { // daca exista duplicate, inseamna ca aici avem combinatiile cum sa le luam pe rand
		std::vector<TestCase> newTestCases; // first you make a tem vector of TestCases

		// iterate over each combination of Pair duplicate from matrix
		for(int i = 0; i < permutationPairWithDuplicates.size(); i++) {

			for(int j = 0; j < permutationPairWithDuplicates[i].size(); j++) {
				//matrixDuplicateTestCases[j][rezult[i][j] - 1] inseamna s[0] combinat
				newTestCases.push_back(matrixDuplicateTestCases[j][permutationPairWithDuplicates[i][j] - 1]);
			}

			// now add all the distinct TestCases
			if(distinctTestCases.size() != 0) {
				for(auto test : distinctTestCases) {
					newTestCases.push_back(test);
				}
			}

			// generate the TestCases based on the new TestCases combinations
			std::vector <char *> newRezults = GenerateTestCases(rootInput, newTestCases);
			// push the result into a bigger structure, that will be the return on this function
			for(auto newTest : newRezults) {
				newInputTests.push_back(newTest);
			}
			newTestCases.clear();
		}
	}else if(distinctTestCases.size() != 0) {

		// first push the new TestCases
		std::vector<TestCase> newTestCases;
			for(auto test : distinctTestCases) {
					newTestCases.push_back(test);
			}
		char *newRootInput;
		strcpy(newRootInput, rootInput);
		std::vector <char *> newRezults = GenerateTestCases(newRootInput, newTestCases);
		for(auto newTest : newRezults) {
			newInputTests.push_back(newTest);
		}
		newTestCases.clear();
	}
	return newInputTests;

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

std::vector<unsigned long> ConcolicExecutor::selectDistinctTestCasesByID(std::vector<TestCase> testCases) {
	set<unsigned long> mySet;
	std::vector<unsigned long> orderOfInstructionAddress;
	for( unsigned i = 0; i <  testCases.size(); ++i ) {
		if(mySet.count(testCases[i].instructionAddress) == 0) {
			// instructionAddress not in set
			orderOfInstructionAddress.push_back(testCases[i].instructionAddress);
		}
		mySet.insert( testCases[i].instructionAddress);
	}

	printf("Order of Instructions selected : \n");
	for(unsigned long i : orderOfInstructionAddress) {
		printf("Instruction set :  %08lx \n", i);
	}

	return orderOfInstructionAddress;
}


std::vector<TestCase>  ConcolicExecutor::selectLastTestCases(std::vector<unsigned long> orderOfInstructionAddress, std::vector<TestCase> testCases) {
	std::vector<TestCase> newTestCase;

	for(std::vector<unsigned long>::iterator it = orderOfInstructionAddress.begin(); it != orderOfInstructionAddress.end(); ++it) {
        int count  = 0;
        vector<TestCase>::reverse_iterator aux;
        
        for (vector<TestCase>::reverse_iterator i = testCases.rbegin(); i != testCases.rend(); ++i ) { 
            if((*i).instructionAddress == (*it)) {
                aux = i;
                count++;
            }
            
            if(count >=2) {
                break;
            }
        }
        
        if(count == 2 || count == 1) {
            newTestCase.push_back((*aux));
        }
    }

	return newTestCase;
}



std::vector<TestCase>  ConcolicExecutor::selectFirstTestCases(std::vector<unsigned long> orderOfInstructionAddress, std::vector<TestCase> testCases) {
	std::vector<TestCase> newTestCase;

	for(std::vector<unsigned long>::iterator it = orderOfInstructionAddress.begin(); it != orderOfInstructionAddress.end(); ++it) {
        int count  = 0;
        vector<TestCase>::iterator aux;
        
        for (vector<TestCase>::iterator i = testCases.begin(); i != testCases.end(); ++i ) { 
            if((*i).instructionAddress == (*it)) {
                aux = i;
                count++;
            }
            
            if(count >=2) {
                break;
            }
        }
        
        if(count == 2 || count == 1) {
            newTestCase.push_back((*aux));
        }
    }

	return newTestCase;
}

std::vector<TestCase> ConcolicExecutor::GetOnlyLoopTestCases(std::vector<TestCase> testCases) {

	/*
		How this method works:
		- first we select only the distinct TestCases ( remember that every TestCase has an unique ID)
		- imagine the situation when you have a loop and get a lot of results
		- now we select only the last results, and print them in the console

	*/

	std::vector<unsigned long> orderOfInstructionAddress = selectDistinctTestCasesByID(testCases);
	std::vector<TestCase> lastTestCases =  selectLastTestCases(orderOfInstructionAddress, testCases);
	std::vector<TestCase> firstTestCases =  selectFirstTestCases(orderOfInstructionAddress, testCases);

/*	
	cout << "Selecting only the first Test Cases" << endl;
	for(TestCase i : firstTestCases) {
		testCase_to_String(i);
	}
*/
	cout << "Selecting only the last Test Cases" << endl;
	for(TestCase i : lastTestCases) {
		testCase_to_String(i);
	}
	


	return lastTestCases;
}


bool checkNumberOfExecutionsPerTest(std::map<unsigned long, int> &instructionAddressMap, std::vector<TestCase> newTestCases, std::vector<TestCase> differentTestCases ) {
	bool alreadyExecutedToManyTimes = false;
	if(instructionAddressMap.size() == 0 && differentTestCases.size() == 0) {
			for(TestCase i : newTestCases) {
				instructionAddressMap.insert(std::pair<unsigned long, int>(i.instructionAddress, 0));	
			}
		}
	for(TestCase i : differentTestCases) {
				bool found = false;		
				for(map<unsigned long,int>::iterator it = instructionAddressMap.begin(); it != instructionAddressMap.end(); ++it) {
					if((*it).first == i.instructionAddress) {
						found = true;
						if((*it).second >= MAX_CALL_PER_TEST) {
							alreadyExecutedToManyTimes = true;
							printf ("TestCase : already executed to many times (this means that we skip this node test):  test - %08lx: \n", (*it).first);
						}else {
						(*it).second++;
						}
					}
				}

				if(found == false) {
					instructionAddressMap.insert(std::pair<unsigned long, int>(i.instructionAddress, 0));	
				}
			}
}

/*
This method is the root of this application. It will generate the Graph in a Bf way
*/
TestGraph* ConcolicExecutor::GenerateExecutionTree(char * start_input) {

	/* How this method works
	- it works like a BF-algorithm
	- it will have a child Process, that every time a signal will trigger, it will call simpleTracer
	- at the end of this method, the child process will be killed
	*/

    TestGraph *graph = new TestGraph(); // first we will create an empty graph
	Vertex *root = graph->AddVertex(start_input); // we add as the root node, the first start_input, example : XXXX

	queue<Vertex*> Q; // queue like BF-Search
	root->visited = true; // mark first node as visited
	Q.push(root); // push root node in queue


	std::map<unsigned long, int> instructionAddressMap; // this is a map, that remembers how many times a TestCase with unique id, has been executed. Imagine the situation when you have a loop.




	while(!Q.empty()) {
		Vertex *x = Q.front();
		std::vector<TestCase> newTestCases = CallSimpleTracer((unsigned char *) x->data); // call SimpleTracer with input and get results
		graph->AddVertexTestCases(x->data, GetOnlyLoopTestCases(newTestCases));// add results to the root (x-data) node of the execution graph, but on results after calling SimpleTracer we implement some filters
		std::vector<TestCase> differentTestCases = FindDiferencesWithParent(x);// find the TestCases between this vertex and he's parent
		
		cout << "DIFERENCES WITH THE PARENT " <<endl;
		for(TestCase i : differentTestCases) {
				testCase_to_String(i); cout << endl;
					//printf("Z3_Code_Test : %s \n", i.Z3_code);
		}

		if(checkNumberOfExecutionsPerTest(instructionAddressMap, newTestCases, differentTestCases) == true) {		
				Q.pop();
				continue;
		}

		std::vector <char *> combinations = GenerateZ3TestCases(x->data, differentTestCases); // generate all the combinations with the new TestCases
	 	cout << "combinations with the string : " << x->data << endl;
		for(char * i : combinations) {
			cout << i << " ";
		} cout<<endl;

		// for every combinations of strings, we create new vertex and add them ass childs for vertex x
		for(char * it : combinations) {
			Vertex *child = graph->AddVertex(it);
			if(child != nullptr) {
				graph->AddEdge(x->data, child->data);
			}else {
				delete[] it;
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


void ConcolicExecutor::WriteSimpleTracerResultsInsideFolder(std::vector<TestCase> results, char * file_Path) {
	FILE *fout = fopen(file_Path, "w");
		if(fout != NULL) {
			for(auto i : results) {
				fprintf(fout, "bbp -> [offset = {%#08x}  , modName = {%s}] cost ->[%d] , jumpType -> [%d] , jumpInstruction -> [%d], esp -> [%#08x] , nInstructions -> [%#08x], bbpNextSize -> [%#08x], nextBasicBlockPointer -> [offset = {%#08x}  , modname = {%s}], \n test - %08lx \n z3_code -> \n %s \n",i.bbp.offset, i.bbp.modName,
					i.cost, i.jumpType, i.jumpInstruction,
					i.esp, i.nInstructions, i.bbpNextSize,
					i.nextBasicBlockPointer.offset,i.nextBasicBlockPointer.modName,
					i.instructionAddress,
					i.Z3_code);
			}
		}
	 fclose(fout);
}