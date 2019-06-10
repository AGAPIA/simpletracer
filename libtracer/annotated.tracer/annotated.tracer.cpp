#include "BinFormat.h"
#include "TextFormat.h"

#include "FileLog.h"

// annotated tracer dependencies
#include "SymbolicEnvironment/Environment.h"
#include "TrackingExecutor.h"
#include "Z3SymbolicExecutor.h"
#include "annotated.tracer.h"

#include "CommonCrossPlatform/Common.h" //MAX_PAYLOAD_BUF; MAX_PATH

#include "utils.h" //common handlers

//for PIPE
#include <stdio.h> 
#include <stdlib.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h>

//....

#ifdef _WIN32
#include <Windows.h>
#define GET_LIB_HANDLER2(libname) LoadLibraryA((libname))
#else
#define GET_LIB_HANDLER2(libname) dlopen((libname), RTLD_LAZY)

#endif

// setup static values for BitMap and TrackingExecutor

int BitMap::instCount = 0;

const unsigned int TrackingExecutor::flagValues[] = {
	RIVER_SPEC_FLAG_OF, RIVER_SPEC_FLAG_OF,
	RIVER_SPEC_FLAG_CF, RIVER_SPEC_FLAG_CF,
	RIVER_SPEC_FLAG_ZF, RIVER_SPEC_FLAG_ZF,
	RIVER_SPEC_FLAG_ZF | RIVER_SPEC_FLAG_CF, RIVER_SPEC_FLAG_ZF | RIVER_SPEC_FLAG_CF,
	RIVER_SPEC_FLAG_SF, RIVER_SPEC_FLAG_SF,
	RIVER_SPEC_FLAG_PF, RIVER_SPEC_FLAG_PF,
	RIVER_SPEC_FLAG_OF | RIVER_SPEC_FLAG_SF, RIVER_SPEC_FLAG_OF | RIVER_SPEC_FLAG_SF,
	RIVER_SPEC_FLAG_OF | RIVER_SPEC_FLAG_SF | RIVER_SPEC_FLAG_ZF, RIVER_SPEC_FLAG_OF | RIVER_SPEC_FLAG_SF | RIVER_SPEC_FLAG_ZF,
};

typedef void __stdcall SH(void *, void *, void *);

void AddReference(void *ref) {
}

void DelReference(void *ref) {
}

namespace at {

unsigned int CustomObserver::ExecutionBegin(void *ctx, void *entryPoint) {
	printf("Process starting\n");
	at->ctrl->GetModules(mInfo, mCount);

	if (HandlePatchLibrary() < 0) {
		return EXECUTION_TERMINATE;
	}

	if (!ctxInit) {
		revEnv = NewX86RevtracerEnvironment(ctx, at->ctrl);
		regEnv = NewX86RegistersEnvironment(revEnv);

		if (at->trackingMode == TAINTED_INDEX_TRACKING) {
			executor = new TrackingExecutor(regEnv, at->varCount, aFormat);
		} else {
			executor = new Z3SymbolicExecutor(regEnv, aFormat);
		}
		executor->SetModuleData(mCount, mInfo);
		regEnv->SetExecutor(executor);
		regEnv->SetReferenceCounting(AddReference, DelReference);

		for (unsigned int i = 0; i < at->varCount; ++i) {
			char vname[8];

			sprintf(vname, "s[%d]", i);

			revEnv->SetSymbolicVariable(vname,
					(rev::ADDR_TYPE)(at->payloadBuff + i), 1);
		}

		ctxInit = true;
	}

	aFormat->WriteTestName(fileName.c_str());
	logEsp = true;

	return EXECUTION_ADVANCE;
}

unsigned int CustomObserver::ExecutionControl(void *ctx, void *address) {
	rev::ExecutionRegs regs;
	rev::BasicBlockInfo bbInfo;

	at->ctrl->GetLastBasicBlockInfo(ctx, &bbInfo);

	unsigned int nextSize = 2;
	struct BasicBlockPointer bbp;
	struct BasicBlockPointer bbpNext[nextSize];

	TranslateAddressToBasicBlockPointer(&bbp, (DWORD)bbInfo.address, mCount, mInfo);

	for (unsigned int i = 0; i < nextSize; ++i) {
		TranslateAddressToBasicBlockPointer(bbpNext + i,
				(DWORD)bbInfo.branchNext[i].address, mCount, mInfo);
	}

	if (logEsp) {
		ClearExecutionRegisters(&regs);
		at->ctrl->GetFirstEsp(ctx, regs.esp);
		aFormat->WriteRegisters(regs);
		logEsp = false;
	}

	at->ctrl->GetCurrentRegisters(ctx, &regs);
	struct BasicBlockMeta bbm { bbp, bbInfo.cost, bbInfo.branchType,
			bbInfo.branchInstruction, regs.esp, bbInfo.nInstructions,
			nextSize, bbpNext };
	aFormat->WriteBasicBlock(bbm);
	aFormat->setBasicBlockMeta_to_TestCase(bbm);

	return EXECUTION_ADVANCE;
}

unsigned int CustomObserver::ExecutionEnd(void *ctx) {
	if (at->batched) {
		CorpusItemHeader header;
		if ((1 == fread(&header, sizeof(header), 1, stdin)) &&
				(header.size == fread(at->payloadBuff, 1, header.size, stdin))) {
			std::cout << "Using " << header.fName << " as input file." << std::endl;

			aFormat->WriteTestName(header.fName);
			return EXECUTION_RESTART;
		}
		return EXECUTION_TERMINATE;
	} else {

    int num, fileDescriptor;
	#define  myfifo  "/tmp/fifochannel"  // FIFO file path 
	mode_t fifo_mode = S_IRUSR | S_IWUSR;
	#define MaxZs              250   /* max microseconds to sleep */

/*
// first we must check if fifo exists -> if yes we remove it
	struct stat stats;
    if ( stat( myfifo, &stats ) < 0 )
    {
        if ( errno != ENOENT )          // ENOENT is ok, since we intend to delete the file anyways
        {
            perror( "stat failed" );    // any other error is a problem
            return( -1 );
        }
    }
    else                                // stat succeeded, so the file exists
    {
        if ( unlink( myfifo ) < 0 )   // attempt to delete the file
        {
            perror( "unlink failed" );  // the most likely error is EBUSY, indicating that some other process is using the file
        }else {
			printf("fifo file unlinked \n");
		}
    }
*/
/*
    if ( mkfifo( myfifo, fifo_mode ) < 0 ) // attempt to create a brand new FIFO {
        perror( "mkfifo failed" );
*/
/*
	if(mkfifo(myfifo, 0666) < 0) {
		perror("mkfifo failed");
	}
	*/
	//sending the list of testCases through pipe


/*
	printf("Child (RIVER) will now open fifo and wait for a reader \n");
	if ((fileDescriptor = open(myfifo, O_CREAT | O_WRONLY)) < 0)
           perror("child - open");
	 printf("child(river) - got a reader \n");
	 

	int numerber_Of_TestCases_Send_throw_pipe = 0;
	 for (auto i = executor->list_TestCase.begin(); i != executor->list_TestCase.end(); i++) {
	// serialize your testcase
		struct TestCaseBase {
			unsigned int offset;
			char modName[MAX_PATH];
		};
		struct TestCase2 {
			struct TestCaseBase bbp;
			unsigned int cost;
			unsigned int jumpType;
			unsigned int jumpInstruction;
			unsigned int esp;
			unsigned int nInstructions;
			unsigned int bbpNextSize;
			struct TestCaseBase nextBasicBlockPointer;
			unsigned long instructionAddress;
			char Z3_code[10000];
		};

		TestCase2 testcase2;
		testcase2.bbp.offset = i->bbp.offset;
		strcpy(testcase2.bbp.modName, i->bbp.modName);
		testcase2.cost = i->cost;
		testcase2.jumpType = i->jumpType;
		testcase2.jumpInstruction = i->jumpInstruction;
		testcase2.esp = i->esp;
		testcase2.nInstructions = i->nInstructions;
		testcase2.bbpNextSize = i->bbpNextSize;

		testcase2.nextBasicBlockPointer.offset = i->nextBasicBlockPointer.offset;
		strcpy(testcase2.nextBasicBlockPointer.modName, i->nextBasicBlockPointer.modName);
		testcase2.instructionAddress = i->instructionAddress;
		char *z3_code = i->Z3_code;
		strcpy(testcase2.Z3_code, z3_code);

		// send serialized testcase
		if ((num = write(fileDescriptor, &testcase2, sizeof(TestCase2))) < 0) {

			perror("child - write");
		}else {
			numerber_Of_TestCases_Send_throw_pipe++;
			printf("CHILD(RIVER) have send an element into the pipe \n");
		}
		usleep((rand() % MaxZs) + 1);  


	}

	close(fileDescriptor);
	printf("CHILD(RIVER) process closed pipe file descriptor \n");
	unlink(myfifo);
	printf("CHILD(RIVER) process unlinked the fifo \n");
	printf("CHILD(RIVER) process sent throw pipe a number of %i elements \n", numerber_Of_TestCases_Send_throw_pipe);
	 
	*/
	return EXECUTION_TERMINATE;
	}
}

unsigned int CustomObserver::TranslationError(void *ctx, void *address) {
	printf("Error issued at address %p\n", address);
	auto direction = ExecutionEnd(ctx);
	if (direction == EXECUTION_RESTART) {
		printf("Restarting after issue\n");
	}
	printf("Translation error. Exiting ...\n");
	exit(1);
	return direction;
}

CustomObserver::CustomObserver(AnnotatedTracer *at) {
	ctxInit = false;

	regEnv = nullptr;
	revEnv = nullptr;
	executor = nullptr;

	this->at = at;
}

CustomObserver::~CustomObserver() {
}

unsigned int AnnotatedTracer::ComputeVarCount() {
	if (payloadBuff == nullptr)
		return 1;

	return ReadFromFile(stdin, payloadBuff, MAX_PAYLOAD_BUF);
}

void AnnotatedTracer::SetSymbolicHandler(rev::SymbolicHandlerFunc symb) {
	this->symb = symb;
}

AnnotatedTracer::AnnotatedTracer()
	: batched(false), varCount(1), ctrl(nullptr), payloadBuff(nullptr),
	PayloadHandler(nullptr), observer(this)
{ }

AnnotatedTracer::~AnnotatedTracer()
{}

int AnnotatedTracer::Run(ez::ezOptionParser &opt) {
	uint32_t executionType = EXECUTION_INPROCESS;

	if (opt.isSet("--extern")) {
		executionType = EXECUTION_EXTERNAL;
	}

	ctrl = NewExecutionController(executionType);

	std::string fModule;
	opt.get("-p")->getString(fModule);
	std::cout << "Using payload " << fModule.c_str() << std::endl;
	if (executionType == EXECUTION_EXTERNAL)
		std::cout << "Starting " << ((executionType == EXECUTION_EXTERNAL) ? "extern" : "internal") << " tracing on module " << fModule << "\n";

	if (executionType == EXECUTION_INPROCESS) {
		LIB_T hModule = GET_LIB_HANDLER2(fModule.c_str());
		if (nullptr == hModule) {
			std::cout << "Payload not found" << std::endl;
			return 0;
		}

		payloadBuff = (unsigned char *)LOAD_PROC(hModule, "payloadBuffer");
		PayloadHandler = (PayloadHandlerFunc)LOAD_PROC(hModule, "Payload");

		if ((nullptr == payloadBuff) || (nullptr == PayloadHandler)) {
			std::cout << "Payload imports not found" << std::endl;
			return 0;
		}
	}


	if (opt.isSet("--binlog")) {
		observer.binOut = true;
	}

	if (opt.isSet("--z3")) {
		trackingMode = Z3_TRACKING;
	} else {
		trackingMode = TAINTED_INDEX_TRACKING;
	}

	std::string fName;
	opt.get("-o")->getString(fName);
	std::cout << "Writing " << (observer.binOut ? "binary" : "text") << " output to " << fName.c_str() << std::endl;

	FileLog *flog = new FileLog();
	flog->SetLogFileName(fName.c_str());
	observer.aLog = flog;

	if (observer.binOut) {
		observer.aFormat = new BinFormat(observer.aLog);
	} else {
		observer.aFormat = new TextFormat(observer.aLog);
	}

	if (opt.isSet("-m")) {
		opt.get("-m")->getString(observer.patchFile);
	}

	if (executionType == EXECUTION_INPROCESS) {
		ctrl->SetEntryPoint((void*)PayloadHandler);
	} else if (executionType == EXECUTION_EXTERNAL) {
		wchar_t ws[MAX_PATH];
		std::mbstowcs(ws, fModule.c_str(), fModule.size() + 1);
		std::cout << "Converted module name [" << fModule << "] to wstring [";
		std::wcout << std::wstring(ws) << "]\n";
		ctrl->SetPath(std::wstring(ws));
	}

	ctrl->SetExecutionFeatures(TRACER_FEATURE_SYMBOLIC);

	ctrl->SetExecutionObserver(&observer);
	ctrl->SetSymbolicHandler(symb);

	if (opt.isSet("--batch")) {
		batched = true;
		FILE *f = freopen(NULL, "rb", stdin);
		if (f == nullptr) {
			std::cout << "stdin freopen failed" << std::endl;
		}

		while (!feof(stdin)) {
			CorpusItemHeader header;
			if ((1 == fread(&header, sizeof(header), 1, stdin)) &&
					(header.size == fread(payloadBuff, 1, header.size, stdin))) {
				std::cout << "Using " << header.fName << " as input file." << std::endl;

				observer.fileName = header.fName;
				varCount = header.size;

				ctrl->Execute();
				ctrl->WaitForTermination();
			}
		}

	} else {
		varCount = ComputeVarCount();

		observer.fileName = "stdin";

		ctrl->Execute();
		ctrl->WaitForTermination();
	}

	DeleteExecutionController(ctrl);
	ctrl = NULL;
	return 0;
}

} // namespace at