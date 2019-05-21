#include "TestGraph.h"



Vertex * TestGraph::FindVertex(string x) {
	for(Vertex * v : VertexList) {
			if (v->data == x)
				return v;
		}
		return NULL;
}



Vertex * TestGraph::AddVertex (string vertexData) {
    // first make sure that the string Vertex is unique
	if(FindVertex(vertexData) != NULL) return NULL;

	// if not, then add a new Vertex in the VertexList
    Vertex *newVertex = new Vertex(vertexData);
	VertexList.push_back(newVertex);
    return newVertex;
}

void TestGraph::AddVertexTestCases(std::string vertexData, std::vector<TestCase> VertexTestCaseList) {
	Vertex *Vertex = FindVertex(vertexData);
	if(Vertex == NULL) {
		cout << "Vertex = " << vertexData << " don't exists" << endl;
	}else {
		Vertex->testCaseList = VertexTestCaseList;
	}
}


//add directed edge going from x to y
void TestGraph::AddDirectedEdge(string parent, string child) {
	// cautam sa vedem daca le avem, daca nu ele sunt nULL, daca da luam pointerii catre nodul respectiv
	Vertex * xVert = FindVertex(parent);
	Vertex * yVert = FindVertex(child);

	yVert->predecesor = xVert;
	yVert->visited = false;
	xVert->neighbors.push_back(yVert);
}

void TestGraph::AddEdge(string parent, string child) {
	AddDirectedEdge(parent, child);
}

//display all vertices and who they connect to
void TestGraph::TestDisplay() {
	for(Vertex * v : VertexList) {
		cout << v->data << ": ";
	    for(Vertex * u : v->neighbors) {
			cout << u->data << ", ";
		}
		cout << endl;
	}
}


std::list<string> TestGraph::BreadthFirstSearch(string start_node) {
		return BreadthFirst(FindVertex(start_node));
}



std::list<string> TestGraph::BreadthFirst(Vertex *s)	{
    std::list<string> bf_Result;
	queue<Vertex*> Q;
    for(Vertex * v : VertexList) {
        v->visited = false;
    }

	s->visited = true; 
	Q.push(s); 
	while (!Q.empty()) {
	    Vertex *x = Q.front();
        bf_Result.push_back(x->data);
		Q.pop();
		for( Vertex *v : x->neighbors) {
			if (v->visited == false) {
				v->visited = true;
				Q.push(v);
			}
			
	    }
    }
    return bf_Result;
}



/*



int main() {
	TestGraph graph;

	graph.addVertex("a");
	graph.addVertex("b");
	graph.addVertex("c");
	graph.addVertex("d");
	graph.addVertex("e");
	graph.addVertex("f");

	graph.addEdge("a", "b");
	graph.addEdge("a", "c");
	graph.addEdge("b", "d");
	graph.addEdge("c", "d");
	graph.addEdge("c", "e");
	graph.addEdge("e", "d");
	graph.addEdge("e", "f");

//	cout << "Predecessor" << endl;
//	graph.displayPredecesors();
	graph.breadthFirstSearch("a");

	

	return 0;
}
*/