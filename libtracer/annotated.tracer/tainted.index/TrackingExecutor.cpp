#include "SymbolicEnvironment/Environment.h"

#include "TrackingExecutor.h"
#include "TaintedIndex.h"
#include "annotated.tracer.h"
#include "revtracer/river.h"
#include <stdlib.h>
#include <stdio.h>

#include "utils.h"

#define MAX_DEPS 20

TrackingExecutor::TrackingExecutor(sym::SymbolicEnvironment *e,
		unsigned int NumSymbolicVariables, AbstractFormat *aFormat)
	: SymbolicExecutor(e), NumSymbolicVariables(NumSymbolicVariables), aFormat(aFormat) {
	ti = new TaintedIndex();
}

void *TrackingExecutor::CreateVariable(const char *name, DWORD size) {
	unsigned int source = atoi(name + 2);
	DWORD sizeInBits = size << 3;
	if (source < NumSymbolicVariables) {
		aFormat->WriteTaintedIndexPayload(ti->GetIndex(), source, sizeInBits);
	} else {
		fprintf(stderr, "Error: Wrong index: I[%lu]\n", ti->GetIndex());
	}
	DWORD res = ti->GetIndex();
	ti->NextIndex();
	return (void *)res;
}

void *TrackingExecutor::MakeConst(DWORD value, DWORD bits) {
	DWORD res = ti->GetIndex();
	aFormat->WriteTaintedIndexConst(res, value, bits);
	ti->NextIndex();
	return (void *)res;
}

void *TrackingExecutor::ExtractBits(void *expr, DWORD lsb, DWORD size) {
	DWORD index = (DWORD)expr;

	aFormat->WriteTaintedIndexExtract(ti->GetIndex(), index, lsb, size);
	DWORD res = ti->GetIndex();
	ti->NextIndex();
	return (void *)res;
}

void *TrackingExecutor::ConcatBits(void *expr1, void *expr2) {
	DWORD index1 = (DWORD)expr1;
	DWORD index2 = (DWORD)expr2;
	unsigned int operands[] = {
		index1, index2
	};

	aFormat->WriteTaintedIndexConcat(ti->GetIndex(), operands);
	DWORD res = ti->GetIndex();
	ti->NextIndex();
	return (void *)res;
}

static const unsigned char flagList[] = {
		RIVER_SPEC_FLAG_CF,
		RIVER_SPEC_FLAG_PF,
		RIVER_SPEC_FLAG_AF,
		RIVER_SPEC_FLAG_ZF,
		RIVER_SPEC_FLAG_SF,
		RIVER_SPEC_FLAG_OF
};

static const char flagNames[6][3] = {
	"CF", "PF", "AF", "ZF", "SF", "OF"
};

static const int flagCount = sizeof(flagList) / sizeof(flagList[0]);

void TrackingExecutor::SetOperands(RiverInstruction *instruction, DWORD index) {
	for (int i = 0; i < 4; ++i) {
		if (RIVER_SPEC_MODIFIES_OP(i) & instruction->specifiers) {
			env->SetOperand(i, (void *)index);
		}
	}

	for (int i = 0; i < flagCount; ++i) {
		if (flagList[i] & instruction->modFlags) {
			env->SetFlgValue(flagList[i], (void *)index);
		}
	}
}

void TrackingExecutor::UnsetOperands(RiverInstruction *instruction) {
	for (int i = 0; i < 4; ++i) {
		if (RIVER_SPEC_MODIFIES_OP(i) & instruction->specifiers) {
			env->UnsetOperand(i);
		}
	}

	for (int i = 0; i < flagCount; ++i) {
		if (flagList[i] & instruction->modFlags) {
			env->UnsetFlgValue(flagList[i]);
		}
	}
}

void TrackingExecutor::Execute(RiverInstruction *instruction) {
	//printf("[%08lx] %02x\n", instruction->instructionAddress, instruction->opCode);

	Operands ops;
	memset(&ops, 0, sizeof(ops));
	unsigned int trk = 0;
	DWORD lastOp = -1;

	for (int i = 0; i < 4; ++i) {
		struct OperandInfo opInfo = {
			.opIdx = (BYTE)i,
			.fields = 0
		};
		if (true == (ops.useOp[i] = env->GetOperand(opInfo))) {
			if (opInfo.fields & OP_HAS_SYMBOLIC) {
				ops.operands[i] = (DWORD)opInfo.symbolic;
				trk++;
				lastOp = ops.operands[i];
			} else {
				ops.operands[i] = -1;
			}
		}
	}

	for (int i = 0; i < flagCount; ++i) {
		struct FlagInfo flagInfo = {
			.opIdx = flagList[i],
			.fields = 0
		};
		if (true == (ops.useFlag[i] = env->GetFlgValue(flagInfo))) {
			if (flagInfo.fields & OP_HAS_SYMBOLIC) {
				ops.flags[i] = (DWORD)flagInfo.symbolic;
				trk++;
				lastOp = ops.flags[i];
			} else {
				ops.flags[i] = -1;
			}
		}
	}

	// clear registers means losing the reference to the
	// tacked memory location. We unset it
	if (0x30 <= instruction->opCode && instruction->opCode <= 0x33) {
		if (ops.operands[0] == ops.operands[1]) {
			UnsetOperands(instruction);
			return;
		}
	}

	if (trk == 0) {
		UnsetOperands(instruction);
		return;
	}

	DWORD index = 0;
	unsigned int depsSize = 0;
	unsigned int deps[MAX_DEPS];
	unsigned int dest = 0;
	unsigned int flags = 0;
	if (trk) {
		if (1 == trk) {
			index = lastOp;
		} else {
			dest = ti->GetIndex();

			for (int i = 0; i < flagCount; ++i) {
				if (ops.useFlag[i] && ops.flags[i] != -1) {
					deps[depsSize] = ops.flags[i];
					depsSize += 1;
					flags |= (1 << i);
				}
			}

			for (int i = 0; i < 4; ++i) {
				if (ops.useOp[i] && ops.operands[i] != -1) {
					deps[depsSize] = ops.operands[i];
					depsSize += 1;
				}
			}

			BasicBlockPointer bbp;
			TranslateAddressToBasicBlockPointer(&bbp,
					instruction->instructionAddress, mCount, mInfo);
			aFormat->WriteTaintedIndexExecute(dest, bbp, flags,
					depsSize, deps);
			index = ti->GetIndex();
			ti->NextIndex();
		}

		SetOperands(instruction, index);
	} else {
		//unset operands if none are symbolic
		UnsetOperands(instruction);
	}
}
