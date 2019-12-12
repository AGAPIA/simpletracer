#include "BinFormatConcolic.h"
#include "TextFormat.h"

#include "FileLog.h"

// annotated tracer dependencies
#include "SymbolicEnvironment/Environment.h"
#include "TrackingExecutor.h"
#include "Z3SymbolicExecutor.h"
#include "annotated.tracer.h"
#include "z3.executor/Z3Utils.h"

#include "CommonCrossPlatform/Common.h" //MAX_PAYLOAD_BUF; MAX_PATH

#include "utils.h" //common handlersgg
#include <fcntl.h>   
#include <sys/stat.h> 
#include <sys/types.h>   
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <assert.h>

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
	PRINTF_INFO("Process starting. Entry point: %p\n", entryPoint);
	aFormat->OnExecutionBegin(nullptr);
	at->ctrl->GetModules(mInfo, mCount);

	if (HandlePatchLibrary() < 0) {
		return EXECUTION_TERMINATE;
	}

	if (!ctxInit) {
		revEnv = NewX86RevtracerEnvironment(ctx, at->ctrl);
		regEnv = NewX86RegistersEnvironment(revEnv);

		if (at->trackingMode == TAINTED_INDEX_TRACKING) {
			executor = new TrackingExecutor(regEnv, at->NumSymbolicVariables, aFormat);
		} else {
			executor = new Z3SymbolicExecutor(regEnv, aFormat, at->simplificationMode == SIMPLIFICATION_AT_ROOT);
		}
		executor->SetModuleData(mCount, mInfo);
		regEnv->SetExecutor(executor);
		regEnv->SetReferenceCounting(AddReference, DelReference);

		for (unsigned int i = 0; i < at->NumSymbolicVariables; ++i) {
			char vname[8];

			sprintf(vname, "@%d", i);

			revEnv->SetSymbolicVariable(vname,
					(rev::ADDR_TYPE)(at->payloadBuff + i), 1);
		}

		ctxInit = true;
	}

	if (at->trackingMode != Z3_TRACKING)	
	{
		aFormat->WriteTestName(fileName.c_str());
		logEsp = true;
		logBasicBlockTracking = true;
	}
	else
	{
		logEsp = false;
		logBasicBlockTracking = false;
	}

	return EXECUTION_ADVANCE;
}

unsigned int CustomObserver::ExecutionControl(void *ctx, void *address) {
	rev::BasicBlockInfo bbInfo;

	at->ctrl->GetLastBasicBlockInfo(ctx, &bbInfo);
	executor->OnExecutionControl(bbInfo);

	if (logEsp || logBasicBlockTracking)
	{
		rev::ExecutionRegs regs;
		unsigned int nextSize = 2;
		struct BasicBlockPointer bbp;
		struct BasicBlockPointer bbpNext[nextSize];

		TranslateAddressToBasicBlockPointer(&bbp, (DWORD)bbInfo.address, mCount, mInfo);

		for (unsigned int i = 0; i < nextSize; ++i) {
			TranslateAddressToBasicBlockPointer(bbpNext + i,
					(DWORD)bbInfo.branchNext[i].address, mCount, mInfo);
		}

		struct BasicBlockMeta bbm { bbp, bbInfo.cost, bbInfo.branchType,
					bbInfo.branchInstruction, (unsigned int)-1, bbInfo.nInstructions,
					nextSize, bbpNext };


		if (logEsp) {
			ClearExecutionRegisters(&regs);
			at->ctrl->GetFirstEsp(ctx, regs.esp);
			aFormat->WriteRegisters(regs);
			bbm.esp = regs.esp;
			logEsp = false;
		}

		if (logBasicBlockTracking)
		{
			at->ctrl->GetCurrentRegisters(ctx, &regs);
			bbm.esp = regs.esp;
			aFormat->WriteBasicBlock(bbm);
		}
	}

	return EXECUTION_ADVANCE;
}

unsigned int CustomObserver::ExecutionEnd(void *ctx) 
{
	if (at->batched) {
		CorpusItemHeader header;
		if ((1 == fread(&header, sizeof(header), 1, stdin)) &&
				(header.size == fread(at->payloadBuff, 1, header.size, stdin))) {
			PRINTF_INFO("Using %s as input file\n", header.fName);

			aFormat->WriteTestName(header.fName);
			return EXECUTION_RESTART;
		}

		return EXECUTION_TERMINATE;
	} 
	else if (at->flowMode) 
	{
		// When we get here, an execution was finished so now let's send the buffered output to the server
		aFormat->OnExecutionEnd();
		aFormat->sendBufferedContentThroughSocket();

		PRINTF_INFO("On flow mode restart\n");
		FlowOpCode nextOp = E_NEXTOP_TASK;
		int payloadSize = 0;
		size_t res1 = fread(&payloadSize, sizeof(int), 1, aFormat->inputSocketStream);
		if (res1 != 1) 
		{
			//printf("Cannot read next opcode or payload size\n");
			exit(1);
		} 
		else 
		{
			if (payloadSize == TERMINATION_PAYLOAD_MARKER)
			{
				nextOp = E_NEXTOP_CLOSE;
			}

				PRINTF_INFO("NNext op code %d. Payload size %d\n" , nextOp, payloadSize);
			}

			if (nextOp == E_NEXTOP_TASK)
			{
				PRINTF_INFO("### Executing a new task\n");
				const int inputStreamSize = ReadFromFile(aFormat->inputSocketStream, at->payloadBuff, payloadSize);
				assert(inputStreamSize == payloadSize); // TODO: what if input is too large to be sent in a chunk ? :) Fix this
				at->NumSymbolicVariables = inputStreamSize;
				PRINTF_INFO("###Finished executing the task\n");
				aFormat->OnExecutionBegin(nullptr);
				return EXECUTION_RESTART;				
			}	
			else
			{
				PRINTF_INFO("Stopping\n");
				at->flowMode = false;
				return EXECUTION_TERMINATE;
			}		
	}
	else {
		aFormat->OnExecutionEnd();
		return EXECUTION_TERMINATE;
	}
}

unsigned int CustomObserver::TranslationError(void *ctx, void *address) {
	PRINTF_INFO("Translation Error issued at address %p\n", address);
	auto direction = ExecutionEnd(ctx);
	if (direction == EXECUTION_RESTART) {
		PRINTF_INFO("Restarting after issue\n");
	}
	PRINTF_INFO("Translation error. Exiting ...\n");
	exit(1);
	return direction;
}

void CustomObserver::setCurrentExecutedBasicBlockDesc(const void* basicBlockInfo)
{
	rev::BasicBlockInfo* bbInfo = (rev::BasicBlockInfo*)basicBlockInfo;
	if (executor)
		executor->currentBasicBlockExecuted = *bbInfo;
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

unsigned int AnnotatedTracer::ComputeNumSymbolicVariables() {
	if (payloadBuff == nullptr)
		return 1;

	return ReadFromFile(stdin, payloadBuff, MAX_PAYLOAD_BUF);
}

void AnnotatedTracer::SetSymbolicHandler(rev::SymbolicHandlerFunc symb) {
	this->symb = symb;
}

AnnotatedTracer::AnnotatedTracer()
	: batched(false), NumSymbolicVariables(1), ctrl(nullptr), payloadBuff(nullptr),
	PayloadHandler(nullptr), observer(this)
{ }

AnnotatedTracer::~AnnotatedTracer()
{}

int AnnotatedTracer::Run(ez::ezOptionParser &opt) 
{
	uint32_t executionType = EXECUTION_INPROCESS;

	// Init the log system
	{
		FILE *f = stdout;
		if (opt.isSet("--logfile"))
		{
			std::string logFile;
			opt.get("--logfile")->getString(logFile);
			//printf("log file is %s\n", logFile.c_str());

			if (logFile != "stdout")
			{
				f = fopen(logFile.c_str(), "w");
				if (f == nullptr)
				{
					fprintf(stderr, "ERROR: Can't open the requested log file %s\n", logFile.c_str());
				}
			}

			// It is the utils responsability to close the file if needed.
			Z3Utils::initLoggingSystem(f);
		}
	}

	if (opt.isSet("--extern")) {
		executionType = EXECUTION_EXTERNAL;
	}

	ctrl = NewExecutionController(executionType);

	std::string fModule;
	opt.get("-p")->getString(fModule);

	PRINTF_INFO("Using payload %s\n", fModule.c_str());
	if (executionType == EXECUTION_EXTERNAL)
		PRINTF_INFO("Starting %s tracing on module %s\n" , ((executionType == EXECUTION_EXTERNAL) ? "extern" : "internal"), fModule);

	if (executionType == EXECUTION_INPROCESS) {
		LIB_T hModule = GET_LIB_HANDLER2(fModule.c_str());
		if (nullptr == hModule) {
			PRINTF_INFO("Payload not found\n");
			return 0;
		}

		payloadBuff = (unsigned char *)LOAD_PROC(hModule, "payloadBuffer");
		PayloadHandler = (PayloadHandlerFunc)LOAD_PROC(hModule, "Payload");

		if ((nullptr == payloadBuff) || (nullptr == PayloadHandler)) {
			PRINTF_INFO("Payload imports not found\n");
			return 0;
		}
	}

	simplificationMode = SIMPLIFICATION_NONE;
	if (opt.isSet("--z3")) 
	{
		trackingMode = Z3_TRACKING;
		if (opt.isSet("--exprsimplify"))
		{
			simplificationMode = SIMPLIFICATION_AT_ROOT;
		}
	} 
	else 
	{
		trackingMode = TAINTED_INDEX_TRACKING;
	}

	if (opt.isSet("--binlog")) 
	{
		observer.binOut = true;
	}

	bool isBinBuffered  = opt.isSet("--binbuffered");
	FileLog *flog = nullptr;
	if (opt.isSet("--flow"))
	{
		observer.binOut = true;
		isBinBuffered = true;

		// No log file here, will write to server through socket
	}
	else
	{
		std::string fName;
		opt.get("-o")->getString(fName);
		PRINTF_INFO("WRiting %s output to %s\n", (observer.binOut ? "binary" : "text"), fName.c_str());

		flog = new FileLog();
		flog->SetLogFileName(fName.c_str());
	}
	

	if (observer.binOut) 
	{
		observer.aFormat = new BinFormatConcolic(flog);
	} 
	else 
	{
		observer.aFormat = new TextFormat(flog);
	}

	if (opt.isSet("-m")) {
		opt.get("-m")->getString(observer.patchFile);
	}

	if (executionType == EXECUTION_INPROCESS) {
		ctrl->SetEntryPoint((void*)PayloadHandler);
	} else if (executionType == EXECUTION_EXTERNAL) {
		wchar_t ws[MAX_PATH];
		std::mbstowcs(ws, fModule.c_str(), fModule.size() + 1);
		PRINTF_INFO("Converted module name [ %s ] to wstring [ %s ]\n", std::wstring(ws));
		ctrl->SetPath(std::wstring(ws));
	}

	ctrl->SetExecutionFeatures(TRACER_FEATURE_SYMBOLIC);

	ctrl->SetExecutionObserver(&observer);
	ctrl->SetSymbolicHandler(symb);

	if (opt.isSet("--batch")) {
		batched = true;
		FILE *f = freopen(NULL, "rb", stdin);
		if (f == nullptr) {
			PRINTF_INFO("stdin freopen failed\n");
		}

		while (!feof(stdin)) {
			CorpusItemHeader header;
			if ((1 == fread(&header, sizeof(header), 1, stdin)) &&
					(header.size == fread(payloadBuff, 1, header.size, stdin))) {
				PRINTF_INFO("Using %s as input file\n", header.fName); 

				observer.fileName = header.fName;
				NumSymbolicVariables = header.size;

				ctrl->Execute();
				ctrl->WaitForTermination();
			}
		}

	} 
	else if (opt.isSet("--flow")) 
	{
		if (!opt.isSet("--addrName"))
		{
			perror("you must set the addrName with flow option !");
			exit(1);
		}

		flowMode = true;
		// Input protocol [[task_op | payload size | payload- if taskOp == E_NEXT_OP_TASK]+ ]
		// Expecting the size of each task first then the stream of tasks

    	
		// Create a socket end for tracer and connect it to the server app
		//--------
		int tracerSocket = -1;
   		if ((tracerSocket = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) 
		{
        	perror("client: socket");
        	exit(1);
		}

		struct sockaddr_un saun;
		saun.sun_family = AF_UNIX;

		std::string serverAddress;
		opt.get("--addrName")->getString(serverAddress);
		strcpy(saun.sun_path, serverAddress.c_str());
		const int len = sizeof(saun.sun_family) + strlen(saun.sun_path);
		if (connect(tracerSocket, (struct sockaddr *)&saun, len) < 0) {
			perror("client: connect");
			exit(1);
		}
		//-------------

		// Open the socket as a read stream
    	observer.aFormat->inputSocketStream = fdopen(tracerSocket, "rb");
		observer.aFormat->outputSocket 		= tracerSocket;

		// flowMode may be modified in ExecutionEnd
		while (flowMode) 
		{
			FlowOpCode nextOp = E_NEXTOP_TASK;
			int payloadSize = 0;
			size_t res1 = fread(&payloadSize, sizeof(int), 1, observer.aFormat->inputSocketStream);
			if (res1 != 1) 
			{
				//printf("Cannot read next opcode or payload size\n");
				exit(1);
			} 
			else 
			{
				if (payloadSize == TERMINATION_PAYLOAD_MARKER)
				{
					nextOp = E_NEXTOP_CLOSE;
				}
				PRINTF_INFO("NNext op code %d. Payload size %d\n" , nextOp, payloadSize);
			}
			
			if (nextOp == E_NEXTOP_TASK)
			{
				PRINTF_INFO("### Executing a new task\n");
				const int inputStreamSize = ReadFromFile(observer.aFormat->inputSocketStream, payloadBuff, payloadSize);
				assert(inputStreamSize == payloadSize); // TODO: what if input is too large to be sent in a chunk ? :) Fix this
				NumSymbolicVariables = inputStreamSize;
				ctrl->Execute();
				ctrl->WaitForTermination();
				PRINTF_INFO("###Finished executing the task\n");
			}
			else if (nextOp == E_NEXTOP_CLOSE) 
			{
				PRINTF_INFO("Stopping\n");
				break;
			}			
		}
	}
	else {
		NumSymbolicVariables = ComputeNumSymbolicVariables();

		observer.fileName = "stdin";

		ctrl->Execute();
		ctrl->WaitForTermination();
	}

	DeleteExecutionController(ctrl);
	ctrl = NULL;

	return 0;
}

} // namespace at
