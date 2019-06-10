#include <string>
#include <list>
#include<set>
#include <queue>
#include <iostream>
#include "FileHandling.h"
#include "TestCase.h"
using namespace std;

class Vertex{
	public:
		char *data;
		Vertex *predecesor;
		list<Vertex*> neighbors;
		bool visited; 
		Vertex(char *x) { 
			data = (char*) malloc(strlen(x) * sizeof(char));
			strcpy(data, x);
		}
		std::vector<TestCase> testCaseList;
};


/*Class TestGraph, is a simple graph, where every node is a Vertex Class Object.*/
class TestGraph {
public:
	list<Vertex*> VertexList;
	
	int compare(unsigned char *a, unsigned char *b, int size);
	//locate Vertex containg value x
	Vertex * FindVertex(char * vertexData);


	Vertex *  AddVertex (char * x);
	void AddVertexTestCases(char * vertexData, std::vector<TestCase> vertexTestCaseList);

	//add directed edge going from x to y
	void AddDirectedEdge(char * parent, char * child);

	void AddEdge(char * parent, char * child);

	//display all vertices and who they connect to
	void TestDisplay();

	// traverse the graph with a BreathFirst Algoritm
	std::list<char *> BreadthFirstSearch(char * rootNode);

private:

	std::list<char *> BreadthFirst(Vertex *s);
};