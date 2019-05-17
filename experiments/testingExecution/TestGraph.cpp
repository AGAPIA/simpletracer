#include "TestGraph.h"

vertex * TestGraph::addVertex (string x) {
    // first make sure that the string vertex is unique
    for(vertex * v : vertexList) {
			if (v->data == x) {
				return nullptr;
		    }
    }
    vertex *newVertex = new vertex(x);
	vertexList.push_back(newVertex);
    return newVertex;
}


//add directed edge going from x to y
void TestGraph::addDirectedEdge(string x, string y) {
	// cautam sa vedem daca le avem, daca nu ele sunt nULL, daca da luam pointerii catre nodul respectiv
	vertex * xVert = findVertex(x);
	vertex * yVert = findVertex(y);

	xVert->neighbors.push_back(yVert);
}

void TestGraph::addEdge(string x, string y) {
	addDirectedEdge(x, y);
	addDirectedEdge(y, x);
}

//display all vertices and who they connect to
void TestGraph::testDisplay() {
	for(vertex * v : vertexList) {
		cout << v->data << ": ";
	    for(vertex * u : v->neighbors) {
			cout << u->data << ", ";
		}
		cout << endl;
	}
}


std::list<string> TestGraph::breadthFirstSearch(string start_node) {
		return BF(findVertex(start_node));
}



std::list<string> TestGraph::BF(vertex *s)	{
    std::list<string> bf_Result;
	queue<vertex*> Q;
    for(vertex * v : vertexList) {
        v->visited = false;
    }

	s->visited = true; 
	Q.push(s); 
	while (!Q.empty()) {
	    vertex *x = Q.front();
        bf_Result.push_back(x->data);
		Q.pop();
		for( vertex *v : x->neighbors) {
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