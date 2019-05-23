#include <string>
#include <list>
#include <queue>
#include <iostream>
#include "FileHandling.h"
#include "TestCase.h"
using namespace std;

class Vertex{
	public:
		string data;
		Vertex *predecesor;
		list<Vertex*> neighbors;
		bool visited; 
		Vertex(string x) { data = x; }
		std::vector<TestCase> testCaseList;
};


/*Class TestGraph, is a simple graph, where every node is a Vertex Class Object.*/
class TestGraph {
public:
	list<Vertex*> VertexList;
	

	//locate Vertex containg value x
	Vertex * FindVertex(string vertexData);


	Vertex *  AddVertex (string x);
	void AddVertexTestCases(std::string vertexData, std::vector<TestCase> vertexTestCaseList);

	//add directed edge going from x to y
	void AddDirectedEdge(string parent, string child);

	void AddEdge(string parent, string child);

	//display all vertices and who they connect to
	void TestDisplay();

	// traverse the graph with a BreathFirst Algoritm
	std::list<string> BreadthFirstSearch(string rootNode);

private:

	std::list<std::string> BreadthFirst(Vertex *s);
};
