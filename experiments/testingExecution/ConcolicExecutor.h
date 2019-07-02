#include "TestGraph.h"
#include <bits/stdc++.h> 


#define myFifo "/tmp/fifochannel"// FIFO file path 
#define SIMPLE_TRACER_EXECUTION_RESULTS_FILE "/tmp/sharedFile"
#define MAX_CALL_PER_TEST 3



/* ConcolicExecutor Class is used to call simpleTracer app, and generate a graph with
all the TestCases need to get full coverage */
class ConcolicExecutor {

    private:

        // this variable is used to send different signals between process
        static int msReceived;

    public:
        // this method will call simpleTracer to get the graph with all the TestCases
        TestGraph* GenerateExecutionTree(char * start_input);
    private:

        // this method is used when sending signals between process
        static void SignalHandler(int sig);

        // this method will call simpleTracer app with an initial input.
        std::vector<TestCase> CallSimpleTracer(unsigned char *testInput);

        // this method will generate all the combinations of 000, 001, 010, 011, 100, 101, 110, 111
        // first paramether will be an empty string ""
        // second parameter specify the lenght of the combination, ex req = 2 => 00, 01,  req = 3 => 000 ....
        std::vector <string> GenerateCombinations(string k,int req);

        // this method will take a vector of TestCases, let say s[0], s[1]
        // then using method GenerateCombinations, will generate all combinations of s[0]s[1], being true or false
        // as a result, a vector of new inputs to call simpleTracer
        std::vector <char *> GenerateTestCases(char *rootInput, std::vector<TestCase> initialTestCases);


        // this method will take a vertex as input, then will return the differences between
        // the TestCases inside this vertex, and he's parent inside the TestGraph
        std::vector<TestCase> FindDiferencesWithParent(Vertex *child);


        // with this function we read from the pipe where the dat is send between this process and simpleTracer call
        std::vector<TestCase> getResultsFromSimpleTracer();

        std::vector<TestCase> GetOnlyLoopTestCases(std::vector<TestCase> testCases);



        std::vector<unsigned long> selectDistinctTestCasesByID(std::vector<TestCase> testCases);

        std::vector<TestCase>  selectLastTestCases(std::vector<unsigned long> orderOfInstructionAddress, std::vector<TestCase> testCases);
        std::vector<TestCase>  selectFirstTestCases(std::vector<unsigned long> orderOfInstructionAddress, std::vector<TestCase> testCases);


        static void GetPermutationWithDuplicates(int n,  std::vector<int> output, std::size_t k,  std::vector<std::vector<int>> &rezult);
        static bool compareByIndex(TestCase t1, TestCase t2);
        void GetDuplicateDistinctTestCase(std::vector<TestCase> initialTestCases, std::vector<std::vector<TestCase>> &duplicateTestCases, std::vector<TestCase> &distinctTestCases);


        // this method will print out a full description of a testCase
        void testCase_to_String(TestCase &testCase);


        std::vector <char *> GenerateZ3TestCases(char *rootInput, std::vector<TestCase> initialTestCases);


        void WriteSimpleTracerResultsInsideFolder(std::vector<TestCase> results, char * file_Path);
};