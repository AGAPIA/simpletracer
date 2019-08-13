#ifndef __ANNOTATED_TRACER__
#define __ANNOTATED_TRACER__

#include "ezOptionParser.h"

#include "BitMap.h"

#include "revtracer/revtracer.h"

#include "basic.observer.h"

#define Z3_TRACKING 0x1
#define TAINTED_INDEX_TRACKING 0x0

#define SIMPLIFICATION_NONE		0x0
 // If below option is used, then whenever we get a Z3 formula we simplify it.
 // This is better for data communication size BUT might be worser because it puts pressure on client (tracer app) to simplify every expression
 // You must test with both options to check which one is better
#define SIMPLIFICATION_AT_ROOT 	0x1


// If none, we export all symbols s[i]... given in the payload. 
// If second option is used, only the symbols really used in the formulas are exported
// The second option theoretically gets better results if SIMPLIFICATION_AT_ROOT is used since
// it could output only the bytes that REALLY play an important (non-redudant) role in the branch expressions
#define TRACKSYMBOLS_NONE 		0x0 
#define TRACKSYMBOLS_ONLY_USED	0x1

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

struct CorpusItemHeader {
	char fName[60];
	unsigned int size;
};

class AnnotatedTracer {
	public:

		bool batched;
		uint8_t trackingMode;
		uint8_t simplificationMode;
		unsigned int varCount;

		ExecutionController *ctrl;

		unsigned char *payloadBuff;
		PayloadHandlerFunc PayloadHandler;

		rev::SymbolicHandlerFunc symb;
		CustomObserver observer;

		unsigned int ComputeVarCount();
		int Run(ez::ezOptionParser &opt);
		void SetSymbolicHandler(rev::SymbolicHandlerFunc symb);

		AnnotatedTracer();
		~AnnotatedTracer();
};

} //namespace at


#endif
