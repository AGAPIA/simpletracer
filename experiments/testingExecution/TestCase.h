
#define MAX_PATH 260 // if you want to change this, you have to change MAX_PATH in simple.tracer coresponding to TestCase.Z3_code
#define MAX_Z3_CODE 10000

struct TestCaseBase {
			unsigned int offset;
			char modName[MAX_PATH];
		};
struct TestCase {
			struct TestCaseBase bbp;
			unsigned int cost;
			unsigned int jumpType;
			unsigned int jumpInstruction;
			unsigned int esp;
			unsigned int nInstructions;
			unsigned int bbpNextSize;
			struct TestCaseBase nextBasicBlockPointer;
			char Z3_code[MAX_Z3_CODE];
	};

