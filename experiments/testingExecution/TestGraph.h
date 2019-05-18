#include <string>
#include <list>
#include <queue>
#include <iostream>
#include "SimpleTracerComunicator.h"
using namespace std;

class vertex{
	public:
		string data;
		vertex *predecesor;
		list<vertex*> neighbors;
		bool visited; 
		vertex(string x) { data = x; }
		std::vector<TestCase> testCaseList;
};
class TestGraph {
public:
	list<vertex*> vertexList;
	

	//locate vertex containg value x
	vertex * findVertex(string x) {
		for(vertex * v : vertexList) {
			if (v->data == x)
				return v;
		}
		return NULL;
	}



	vertex *  addVertex (string x);
	void addVertexTestCases(std::string vertex_string, std::vector<TestCase> vertexTestCaseList);

	//add directed edge going from x to y
	void addDirectedEdge(string x, string y);

	void addEdge(string x, string y);

	//display all vertices and who they connect to
	void testDisplay();

	std::list<string> breadthFirstSearch(string start_node);

private:

	std::list<std::string> BF(vertex *s);
};
