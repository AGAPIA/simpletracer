#ifndef __ANNOTATED_TRACER__
#define __ANNOTATED_TRACER__

#include "ezOptionParser.h"

#include "BitMap.h"

#include "revtracer/revtracer.h"

#include "basic.observer.h"
#include "Logger.h"

#define Z3_TRACKING 0x1
#define TAINTED_INDEX_TRACKING 0x0

#define SIMPLIFICATION_NONE		0x0
 // If below option is used, then whenever we get a Z3 formula we simplify it.
 // This is better for data communication size BUT might be worser because it puts pressure on client (tracer app) to simplify every expression
 // You must test with both options to check which one is better
#define SIMPLIFICATION_AT_ROOT 	0x1

namespace at {

typedef int(*PayloadHandlerFunc)();

class AnnotatedTracer;

class CustomObserver : public BasicObserver {
	public:
		AnnotatedTracer *at;

		sym::SymbolicEnvironment *regEnv;
		sym::SymbolicEnvironment *revEnv;
		sym::SymbolicExecutor *executor;

		virtual unsigned int ExecutionBegin(void *ctx, void *entryPoint);
		virtual unsigned int ExecutionControl(void *ctx, void *address);
		virtual unsigned int ExecutionEnd(void *ctx);
		virtual unsigned int TranslationError(void *ctx, void *address);

		virtual void setCurrentExecutedBasicBlockDesc(const void* basicBlockInfo);

		CustomObserver(AnnotatedTracer *at);
		~CustomObserver();
};

enum FlowOpCode
{
	E_NEXTOP_CLOSE,
	E_NEXTOP_TASK
};

#define TERMINATION_PAYLOAD_MARKER -1

struct CorpusItemHeader {
	char fName[60];
	unsigned int size;
};

class AnnotatedTracer {
	public:

		bool batched;
		bool flowMode = false;
		uint8_t simplificationMode;
		uint8_t trackingMode = Z3_TRACKING;
		unsigned int NumSymbolicVariables;

		ExecutionController *ctrl;

		unsigned char *payloadBuff;
		PayloadHandlerFunc PayloadHandler;

		rev::SymbolicHandlerFunc symb;
		CustomObserver observer;

		unsigned int ComputeNumSymbolicVariables();
		int Run(ez::ezOptionParser &opt);
		void SetSymbolicHandler(rev::SymbolicHandlerFunc symb);

		AnnotatedTracer();
		~AnnotatedTracer();
};

} //namespace at


#endif
