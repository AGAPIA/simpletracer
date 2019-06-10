#include "TestGraph.h"

int TestGraph::compare(unsigned char *a, unsigned char *b, int size) {
	while(size-- > 0) {
		if ( *a != *b ) { return (*a < *b ) ? -1 : 1; }
		a++; b++;
	}
	return 0;
}

Vertex * TestGraph::FindVertex(char * x) {

	
	for(Vertex * v : VertexList) {
			int cmp = compare((unsigned char *) v->data, (unsigned char *) x, strlen(v->data));
			if (cmp == 0) {
				return v;
			}
		}
		return NULL;
}



Vertex * TestGraph::AddVertex (char * vertexData) {
    // first make sure that the string Vertex is unique
	if(FindVertex(vertexData) != NULL) {
		return NULL;
	}

	// if not, then add a new Vertex in the VertexList
    Vertex *newVertex = new Vertex(vertexData);
	VertexList.push_back(newVertex);
    return newVertex;
}

void TestGraph::AddVertexTestCases(char * vertexData, std::vector<TestCase> VertexTestCaseList) {
	Vertex *Vertex = FindVertex(vertexData);
	if(Vertex == NULL) {
		cout << "Vertex = " << vertexData << " don't exists" << endl;
	}else {
		Vertex->testCaseList = VertexTestCaseList;
	}
}


//add directed edge going from x to y
void TestGraph::AddDirectedEdge(char * parent, char * child) {
	// cautam sa vedem daca le avem, daca nu ele sunt nULL, daca da luam pointerii catre nodul respectiv
	Vertex * xVert = FindVertex(parent);
	Vertex * yVert = FindVertex(child);

	yVert->predecesor = xVert;
	yVert->visited = false;
	xVert->neighbors.push_back(yVert);
}

void TestGraph::AddEdge(char * parent, char * child) {
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


std::list<char *> TestGraph::BreadthFirstSearch(char * start_node) {
		return BreadthFirst(FindVertex(start_node));
}



std::list<char *> TestGraph::BreadthFirst(Vertex *s)	{
    std::list<char *> bf_Result;
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
