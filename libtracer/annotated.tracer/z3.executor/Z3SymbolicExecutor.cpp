#include <unordered_set>
#include "Z3SymbolicExecutor.h"
#include "CommonCrossPlatform/Common.h"


#include <assert.h>
#include <string.h>

#include "utils.h"
#include "Z3Utils.h"

#define MAX_BENCHMARK_NAME 256

const unsigned char Z3SymbolicExecutor::flagList[] = {
	RIVER_SPEC_FLAG_CF,
	RIVER_SPEC_FLAG_PF,
	RIVER_SPEC_FLAG_AF,
	RIVER_SPEC_FLAG_ZF,
	RIVER_SPEC_FLAG_SF,
	RIVER_SPEC_FLAG_OF,
	RIVER_SPEC_FLAG_DF
};

bool Z3SymbolicExecutor::CheckSameSort(unsigned size, Z3_ast *ops) {
	unsigned sortSize = 0xffffffff;
	for (unsigned int i = 0; i < size; ++i) {
		unsigned tempSortSize = Z3_get_bv_sort_size(context,
				Z3_get_sort(context, ops[i]));
		if (sortSize == 0xffffffff) {
			sortSize = tempSortSize;
		} else if (tempSortSize != sortSize) {
			return false;
		}
	}
	return true;
}

void Z3SymbolicExecutor::InitLazyFlagsOperands(
		struct SymbolicOperandsLazyFlags *solf,
		struct SymbolicOperands *ops) {
	for (int i = 0; i < opCount; ++i) {
		solf->svBefore[i] = ops->sv[i];
	}
	for (int i = 0; i < flagCount; ++i) {
		solf->svfBefore[i] = ops->svf[i];
	}
}

void *Z3SymbolicExecutor::CreateVariable(const char *name, nodep::DWORD size) {
	Z3_symbol s = Z3_mk_string_symbol(context, name);
	Z3_sort srt;
	Z3_ast zro;

	switch (size) {
	case 1 :
		srt = byteSort;
		zro = zero8;
		break;
	case 2 :
		srt = wordSort;
		zro = zero16;
		break;
	case 4 :
		srt = dwordSort;
		zro = zero32;
		break;
	default :
		return nullptr;
	};

	Z3_ast ret = Z3_mk_const(context, s, srt);

/*
	Z3_ast l1[2] = {
		Z3_mk_bvule(
			context,
			Z3_mk_int(context, 'a', srt),
			ret
		),
		Z3_mk_bvuge(
			context,
			Z3_mk_int(context, 'z', srt),
			ret
		)
	};

	Z3_ast l2[2] = {
		Z3_mk_eq(
			context,
			zro,
			ret
		),
		Z3_mk_and(
			context,
			2,
			l1
		)
	};

	Z3_ast cond = Z3_mk_or(
		context,
		2,
		l2
	);

	Z3_solver_assert(context, solver, cond);
	PRINTF_SYM("(assert %s)\n\n", Z3_ast_to_string(context, cond));
	*/

	symIndex++;

	PRINTF_SYM("var %p <= %s\n", ret, name);
	PRINT_AST(context, ret);
	return ret;
}

void *Z3SymbolicExecutor::MakeConst(nodep::DWORD value, nodep::DWORD bits) {
	Z3_sort type;

	switch (bits) {
		case 8:
			type = byteSort;
			break;
		case 16:
			type = wordSort;
			break;
		case 24: // only for unaligned mem acceses
			type = tbSort;
			break;
		case 32:
			type = dwordSort;
			break;
		default :
			DEBUG_BREAK;
	}

	Z3_ast ret = Z3_mk_int(
		context,
		value,
		type
	);

	PRINTF_SYM("const %p <= %08lx\n", ret, value);
	PRINT_AST(context, ret);

	return (void *)ret;
}

// lsb and size are both expressed in bits
void *Z3SymbolicExecutor::ExtractBits(void *expr, nodep::DWORD lsb, nodep::DWORD size) {
	Z3_ast ret = Z3_mk_extract(
		context,
		lsb + size - 1,
		lsb,
		(Z3_ast)expr
	);

	if (this->simplifyAllOutputZ3Expressions)
	{
		ret = Z3_simplify(context, ret);
	}

	PRINTF_SYM("extract %p <= %p, 0x%02lx, 0x%02lx\n", ret, expr, lsb+size-1, lsb);
	PRINT_AST(context, ret);
	return (void *)ret;
}

void *Z3SymbolicExecutor::ConcatBits(void *expr1, void *expr2) {
	Z3_ast ret = Z3_mk_concat(
		context,
		(Z3_ast)expr1,
		(Z3_ast)expr2
	);
	if (simplifyAllOutputZ3Expressions)
	{
		ret = Z3_simplify(context, ret);
	}

	PRINTF_SYM("concat %p <= %p, %p\n", ret, expr1, expr2);
	PRINT_AST(context, ret);
	return (void *)ret;
}

void *Z3SymbolicExecutor::ExecuteResolveAddress(void *base, void *index,
		nodep::BYTE scale) {
	Z3_ast indexRet = 0, scaleAst = 0, Ret = 0;

	switch(scale) {
	case 0:
		indexRet = zeroScale;
		break;
	case 1:
		indexRet = (Z3_ast)index;
		break;
	case 2:
		scaleAst = twoScale;
		break;
	case 4:
		scaleAst = fourScale;
		break;
	default:
		DEBUG_BREAK;
	}

	// if index and scale are valid
	if (index != nullptr && (void *)scaleAst != nullptr) {
		indexRet = Z3_mk_bvmul(context, (Z3_ast)index, scaleAst);
	}

	if (base != nullptr) {
		if ((void *)indexRet != nullptr) {
			Ret = Z3_mk_bvadd(context, (Z3_ast)base, indexRet);
		} else {
			Ret = (Z3_ast)base;
		}
	} else {
		if (index != nullptr) {
			Ret = indexRet;
		} else {
			Ret = (Z3_ast)0;
		}
	}
	fprintf(stderr, "base: %p, index: %p, scale: %d, indexRet: %p, Ret: %p\n",
			base, index, scale, (void *)indexRet, (void *)Ret);
	return (void *)Ret;
}


const unsigned int Z3SymbolicExecutor::Z3SymbolicCpuFlag::lazyMarker = 0xDEADBEEF;

/*====================================================================================================*/

Z3SymbolicExecutor::Z3SymbolicExecutor(sym::SymbolicEnvironment *e, AbstractFormat *aFormat,
	const bool _simplifyAllOutputZ3Expressions) :
		sym::SymbolicExecutor(e), 
		aFormat(aFormat),
		simplifyAllOutputZ3Expressions(_simplifyAllOutputZ3Expressions)
{
	symIndex = 1;

	lazyFlags[0] = new Z3FlagCF();
	lazyFlags[1] = new Z3FlagPF();
	lazyFlags[2] = new Z3FlagAF();
	lazyFlags[3] = new Z3FlagZF();
	lazyFlags[4] = new Z3FlagSF();
	lazyFlags[5] = new Z3FlagOF();
	lazyFlags[6] = new Z3FlagDF();

	for (int i = 0; i < flagCount; ++i) {
		lazyFlags[i]->SetParent(this);
	}

	lastCondition = nullptr;

	config = Z3_mk_config();
	context = Z3_mk_context(config);
	Z3_set_ast_print_mode(context, Z3_PRINT_SMTLIB2_COMPLIANT);
	//Z3_open_log("Z3.log");

	dwordSort = Z3_mk_bv_sort(context, 32);
	tbSort = Z3_mk_bv_sort(context, 24);
	wordSort = Z3_mk_bv_sort(context, 16);
	byteSort = Z3_mk_bv_sort(context, 8);
	bitSort = Z3_mk_bv_sort(context, 1);

	sevenBitSort = Z3_mk_bv_sort(context, 7); 

	zero32 = Z3_mk_int(context, 0, dwordSort);
	one32 = Z3_mk_int(context, 1, dwordSort);
	zero16 = Z3_mk_int(context, 0, wordSort);
	zero7 = Z3_mk_int(context, 0, sevenBitSort);
	zero8 = Z3_mk_int(context, 0, byteSort);
	one8 = Z3_mk_int(context, 1, byteSort);

	zeroFlag = Z3_mk_int(context, 0, bitSort);
	oneFlag = Z3_mk_int(context, 1, bitSort);

	zeroScale = zero32;
	twoScale = Z3_mk_int(context, 2, dwordSort);
	fourScale = Z3_mk_int(context, 4, dwordSort);

	solver = Z3_mk_solver(context);
	Z3_solver_push(context, solver);
	Z3_solver_inc_ref(context, solver);

	ls = new stk::LargeStack(saveStack, sizeof(saveStack), &saveTop, "flagStack.bin");
}

Z3SymbolicExecutor::~Z3SymbolicExecutor() {
	delete ls;

	Z3_solver_dec_ref(context, solver);

	for (int i = 0; i < flagCount; ++i) {
		delete lazyFlags[i];
	}
}

void Z3SymbolicExecutor::StepForward() {
	Z3_solver_push(context, solver);
	for (int i = 0; i < flagCount; ++i) {
		lazyFlags[i]->SaveState(*ls);
	}
}

void Z3SymbolicExecutor::StepBackward() {

	for (int i = 6; i >= 0; --i) {
		lazyFlags[i]->LoadState(*ls);
	}

	Z3_solver_pop(context, solver, 1);
}

void Z3SymbolicExecutor::SymbolicExecuteUnk(RiverInstruction *instruction, SymbolicOperands *ops) {
	fprintf(stderr, "Z3 execute unknown instruction %02x %02x \n",
			instruction->modifiers & RIVER_MODIFIER_EXT ? 0x0F : 0x00,
			instruction->opCode);
	
	if (instruction->opCode == 0x99) {
		printf("%d", instruction->opCode);
		return;
	}

	DEBUG_BREAK;
}

/* new impl */

// returns boolean

template <unsigned int flag> Z3_ast Z3SymbolicExecutor::Flag(SymbolicOperands *ops) {
	return (Z3_ast)ops->svf[flag];
}

// returns BV[1]
template <Z3SymbolicExecutor::BVFunc func> Z3_ast Z3SymbolicExecutor::Negate(SymbolicOperands *ops) {
	return Z3_mk_bvnot(
		context,
		(this->*func)(ops)
	);
}

// returns BV[1]
template <Z3SymbolicExecutor::BVFunc func1, Z3SymbolicExecutor::BVFunc func2> Z3_ast Z3SymbolicExecutor::Equals(SymbolicOperands *ops) {
	return Z3_mk_bvxnor(
		context,
		(this->*func1)(ops),
		(this->*func2)(ops)
	);
}

// returns BV[1]
template <Z3SymbolicExecutor::BVFunc func1, Z3SymbolicExecutor::BVFunc func2> Z3_ast Z3SymbolicExecutor::Or(SymbolicOperands *ops) {
	return Z3_mk_bvor(
		context,
		(this->*func1)(ops),
		(this->*func2)(ops)
	);
}

/* AstToBenchmarkString - turns ast node into string
 *
 * const_symbol - param that represents a symbol name.
 * The constraints corresponding to const_symbol are generated
 * in other system component TODO
 */

const char *Z3SymbolicExecutor::AstToBenchmarkString(Z3_ast ast, RiverInstruction *instruction, const char *const_name) 
{
	Z3_ast formula = Z3_mk_true(context);
	Z3_symbol const_symbol = Z3_mk_string_symbol(context, const_name);
	Z3_ast assertion = Z3_mk_eq(context,
			ast,
			Z3_mk_const(context, const_symbol, Z3_get_sort(context, ast)));

	if (simplifyAllOutputZ3Expressions)
	{
		assertion = Z3_simplify(context, assertion);
	}

	//printf("assertion: \n");
	Z3_string z3TestExpr = Z3_ast_to_string(context, assertion);
	//printf("%s\n", z3TestExpr);
	//PRINT_AST(context, assertion);
	
	return z3TestExpr;
}

// TODO: Improve this with a faster parsing of the tree instead of parsing the string :)
// asked on stackoverflow...
void getUsedSymbolsFromASTString(const char* astString, std::unordered_set<unsigned int>& outSetOfInputsUsed)
{
	static const int MAX_BUFFER_SIZE = 1024;
	static char symbolNumberStrBuffer[MAX_BUFFER_SIZE];

	const char* iter = astString;
	while (*iter)
	{
		const char* symbolBegin = strstr(iter, "@");
		if (symbolBegin == nullptr)
			break;

		symbolBegin += 1;

		const char* symbolEnd = symbolBegin;
		const int maxSymbolLen = 32;
		int symbolLen = 0;
		while (symbolLen < maxSymbolLen)
		{
			char currChar = *symbolEnd;
			if (!isdigit(currChar))
				break;
			//currChar == ' ' || currChar == '\t' || currChar == '\n' || currChar == '\0')
			//	break;

			symbolLen++;
			symbolEnd++;
		}

		// Check if the symbol number is ok and check the global usage array
		const int size = symbolEnd - symbolBegin;
		if (size > MAX_BUFFER_SIZE)
		{
			printf("ERROR: the buffer size is too small. I think someting is not right..\n");
			DEBUG_BREAK;
			continue;
		}

		memcpy(symbolNumberStrBuffer, symbolBegin, size);
		symbolNumberStrBuffer[size] = '\0';

		const int index = atoi(symbolNumberStrBuffer);
		if (outSetOfInputsUsed.find(index) == outSetOfInputsUsed.end())
		{
			outSetOfInputsUsed.insert(index);
		}

		iter = symbolEnd + 2;
	}
}

template <Z3SymbolicExecutor::BVFunc func> void Z3SymbolicExecutor::SymbolicJumpCC(RiverInstruction *instruction, SymbolicOperands *ops) {
	
	Z3_ast cond = (this->*func)(ops);

	m_lastConcolicTest.flags.testFlags = instruction->testFlags;
	for (int i = 0; i < flagCount; ++i) {
		if (ops->trf[i]) {
			m_lastConcolicTest.flags.symbolicFlags[i] = (unsigned int)ops->svf[i];
		} else {
			m_lastConcolicTest.flags.symbolicFlags[i] = 0;
		}
	}
	m_lastConcolicTest.flags.symbolicCond = (unsigned int)cond;

	char testSymbolName[32];
	snprintf(testSymbolName, 32, "$%08lx", (DWORD)(currentBasicBlockExecuted.address));
	m_lastConcolicTest.ast.address = AstToBenchmarkString((Z3_ast) m_lastConcolicTest.flags.symbolicCond, instruction, testSymbolName);
	m_lastConcolicTest.ast.size = strlen(m_lastConcolicTest.ast.address);
	m_lastConcolicTest.blockOptionTaken = currentBasicBlockExecuted.branchNext[0].address;
	m_lastConcolicTest.blockOptionNotTaken = currentBasicBlockExecuted.branchNext[1].address;
	m_lastConcolicTest.parentBlock = (DWORD) currentBasicBlockExecuted.address;
	getUsedSymbolsFromASTString(m_lastConcolicTest.ast.address, m_lastConcolicTest.indicesOfInputBytesUsed);
	m_lastConcolicTest.setPending(true);
}

template <Z3SymbolicExecutor::BVFunc func> void Z3SymbolicExecutor::SymbolicSetCC(RiverInstruction *instruction, SymbolicOperands *ops) {
	Z3_ast res = Z3_mk_concat(
		context,
		zero7,
		(this->*func)(ops)
	);
	env->SetOperand(0, res);
	PrintSetOperands(0);
}

void Z3SymbolicExecutor::PrintSetOperands(unsigned idx) {
	struct OperandInfo opInfo;
	opInfo.opIdx = idx;
	env->GetOperand(opInfo);

	if (opInfo.symbolic != nullptr) {
		PRINT_AST(context, (Z3_ast)opInfo.symbolic);
	}
}

template <Z3SymbolicExecutor::BVFunc func> void Z3SymbolicExecutor::SymbolicCmovCC(RiverInstruction *instruction, SymbolicOperands *ops) {
	Z3_ast cond = Z3_mk_eq(context, (this->*func)(ops), oneFlag);
	Z3_ast res = Z3_mk_ite(context,
			cond,
			(Z3_ast)ops->sv[1],
			(Z3_ast)ops->sv[0]
			);
	PRINTF_SYM("cmovcc %p <= src: %p cond: %p\n", res, cond, ops->sv[1]);
	env->SetOperand(0, res);
	PrintSetOperands(0);
}

/**/

void Z3SymbolicExecutor::SymbolicExecuteNop(RiverInstruction *instruction, SymbolicOperands *ops) {}

void Z3SymbolicExecutor::SymbolicExecuteCmpxchg(RiverInstruction *instruction, SymbolicOperands *ops) {
	Z3_ast eax = (Z3_ast)ops->sv[0];
	Z3_ast o1 = (Z3_ast)ops->sv[1];
	Z3_ast o2 = (Z3_ast)ops->sv[2];

	Z3_ast cond = Z3_mk_eq(context, eax, o1);
	Z3_ast r1 = Z3_mk_ite(context, cond, eax, o1);
	env->SetOperand(0, r1);
	PrintSetOperands(0);

	Z3_ast r2 = Z3_mk_ite(context, cond, o2, o1);
	env->SetOperand(1, r2);
	PrintSetOperands(1);

	PRINTF_SYM("cmpxchg eax[%p] o1[%p] <= eax[%p] o1[%p] o2[%p]\n", r1, r2,
			ops->sv[0], ops->sv[1], ops->sv[2]);
}

// should have one parameter only
Z3_ast Z3SymbolicExecutor::ExecuteInc(unsigned nOps, SymbolicOperands *ops) {
	if (nOps < 1) DEBUG_BREAK;
	Z3_ast o1 = (Z3_ast)ops->sv[0];

	PRINTF_SYM("inc %p\n", (void *)o1);
	Z3_ast res = Z3_mk_bvadd(context, o1, one32);
	env->SetOperand(0, res);
	PrintSetOperands(0);
	return res;
}

// should have one parameter only
Z3_ast Z3SymbolicExecutor::ExecuteDec(unsigned nOps, SymbolicOperands *ops) {
	if (nOps < 1) DEBUG_BREAK;
	Z3_ast o1 = (Z3_ast)ops->sv[0];

	PRINTF_SYM("dec %p\n", (void *)o1);
	Z3_ast res = Z3_mk_bvsub(context, o1, one32);
	env->SetOperand(0, res);
	PrintSetOperands(0);
	return res;
}

Z3_ast Z3SymbolicExecutor::ExecuteAdd(unsigned nOps, SymbolicOperands *ops) {
	if (nOps < 2) DEBUG_BREAK;
	Z3_ast o1 = (Z3_ast)ops->sv[0];
	Z3_ast o2 = (Z3_ast)ops->sv[1];

	Z3_ast r = Z3_mk_bvadd(context, o1, o2);
	env->SetOperand(0, r);

	PRINTF_SYM("add %p <= %p, %p\n", r, o1, o2);
	PrintSetOperands(0);
	return r;
}

Z3_ast Z3SymbolicExecutor::ExecuteOr(unsigned nOps, SymbolicOperands *ops) {
	if (nOps < 2) DEBUG_BREAK;
	Z3_ast o1 = (Z3_ast)ops->sv[0];
	Z3_ast o2 = (Z3_ast)ops->sv[1];

	Z3_ast r = Z3_mk_bvor(context, o1, o2);
	env->SetOperand(0, r);

	PRINTF_SYM("or %p <= %p[%d], %p[%d]\n",
			r, o1, Z3_get_bv_sort_size(context, Z3_get_sort(context, o1)),
			o2, Z3_get_bv_sort_size(context, Z3_get_sort(context, o2)));
	PrintSetOperands(0);
	return r;
}

Z3_ast Z3SymbolicExecutor::ExecuteAdc(unsigned nOps, SymbolicOperands *ops) {
	//return Z3_mk_bvadd(context, o1, o2);
	DEBUG_BREAK;
	return nullptr;
}

Z3_ast Z3SymbolicExecutor::ExecuteSbb(unsigned nOps, SymbolicOperands *ops) {
	Z3_ast dest = (Z3_ast)ops->sv[0];
	Z3_ast source = (Z3_ast)ops->sv[1];
	Z3_ast cf = (Z3_ast)ops->svf[RIVER_SPEC_IDX_CF];

	unsigned sourceSize = Z3_get_bv_sort_size(context,
			Z3_get_sort(context, source));
	unsigned cfSize = Z3_get_bv_sort_size(context,
			Z3_get_sort(context, cf));

	Z3_ast cfExtend = Z3_mk_concat(context,
			Z3_mk_int(context, 0, Z3_mk_bv_sort(context, sourceSize - 1)),
			cf);

	PRINTF_SYM("concat %p <= 0[%d] cf:%p[%d]\n",
			cfExtend, sourceSize - 1, cf, cfSize);

	Z3_ast op = Z3_mk_bvadd(context, source, cfExtend);
	Z3_ast res = Z3_mk_bvsub(context, dest, op);
	env->SetOperand(0, res);
	PrintSetOperands(0);
	PRINTF_SYM("sbb %p <= %p %p cf %p\n", res, dest, source, cf);
	return res;
}

Z3_ast Z3SymbolicExecutor::ExecuteAnd(unsigned nOps, SymbolicOperands *ops) {
	if (nOps < 2) DEBUG_BREAK;
	Z3_ast o1 = (Z3_ast)ops->sv[0];
	Z3_ast o2 = (Z3_ast)ops->sv[1];

	Z3_ast r = Z3_mk_bvand(context, o1, o2);
	env->SetOperand(0, r);

	PRINTF_SYM("and %p <= %p, %p\n", r, o1, o2);
	PrintSetOperands(0);
	return r;
}

Z3_ast Z3SymbolicExecutor::ExecuteSub(unsigned nOps, SymbolicOperands *ops) {
	if (nOps < 2) DEBUG_BREAK;
	Z3_ast o1 = (Z3_ast)ops->sv[0];
	Z3_ast o2 = (Z3_ast)ops->sv[1];

	Z3_ast r = Z3_mk_bvsub(context, o1, o2);
	env->SetOperand(0, r);

	PRINTF_SYM("sub %p <= %p, %p\n", r, o1, o2);
	return r;
	PrintSetOperands(0);
}

Z3_ast Z3SymbolicExecutor::ExecuteXor(unsigned nOps, SymbolicOperands *ops) {
	if (nOps < 2) DEBUG_BREAK;
	Z3_ast o1 = (Z3_ast)ops->sv[0];
	Z3_ast o2 = (Z3_ast)ops->sv[1];

	if (o1 == o2) {
		return nullptr;
	}

	Z3_ast r = Z3_mk_bvxor(context, o1, o2);
	env->SetOperand(0, r);

	PRINTF_SYM("xor %p <= %p, %p\n", r, o1, o2);
	PrintSetOperands(0);
	return r;
}

Z3_ast Z3SymbolicExecutor::ExecuteCmp(unsigned nOps, SymbolicOperands *ops) {
	if (nOps < 2) DEBUG_BREAK;
	Z3_ast o1 = (Z3_ast)ops->sv[0];
	Z3_ast o2 = (Z3_ast)ops->sv[1];

	Z3_ast r = Z3_mk_bvsub(context, o1, o2);
	Z3_error_code err = Z3_get_error_code(context);
	if (err != Z3_OK) DEBUG_BREAK;

	PRINTF_SYM("cmp %p <= %p, %p\n", r, o1, o2);
	PRINT_AST(context, r);

	return r;
}

Z3_ast Z3SymbolicExecutor::ExecuteTest(unsigned nOps, SymbolicOperands *ops) {
	if (nOps < 2) DEBUG_BREAK;
	Z3_ast o1 = (Z3_ast)ops->sv[0];
	Z3_ast o2 = (Z3_ast)ops->sv[1];

	if (!CheckSameSort(2, (Z3_ast *)ops->sv)) DEBUG_BREAK;

	Z3_ast r = Z3_mk_bvand(context, o1, o2);
	PRINTF_SYM("test %p <= %p, %p\n", r, o1, o2);
	PRINT_AST(context, r);
	return r;
}

Z3_ast Z3SymbolicExecutor::ExecuteRol(unsigned nOps, SymbolicOperands *ops) {
	if (nOps < 2) DEBUG_BREAK;

	unsigned i;
	Z3_get_numeral_uint(context, (Z3_ast)ops->sv[1], &i);
	Z3_ast t = (Z3_ast)ops->sv[0];

	Z3_ast r = Z3_mk_rotate_left(context, i, t);
	PRINTF_SYM("rol %p <= %p 0x%08X\n", r, t, i);
	PRINT_AST(context, r);
	return r;
}

Z3_ast Z3SymbolicExecutor::ExecuteRor(unsigned nOps, SymbolicOperands *ops) {
	if (nOps < 2) DEBUG_BREAK;

	unsigned i;
	Z3_get_numeral_uint(context, (Z3_ast)ops->sv[1], &i);
	Z3_ast t = (Z3_ast)ops->sv[0];

	Z3_ast r = Z3_mk_rotate_right(context, i, t);
	PRINTF_SYM("ror %p <= %p 0x%08X\n", r, t, i);
	PRINT_AST(context, r);
	return r;
}

Z3_ast Z3SymbolicExecutor::ExecuteRcl(unsigned nOps, SymbolicOperands *ops) {
	DEBUG_BREAK;
	return nullptr;
}

Z3_ast Z3SymbolicExecutor::ExecuteRcr(unsigned nOps, SymbolicOperands *ops) {
	DEBUG_BREAK;
	return nullptr;
}

Z3_ast Z3SymbolicExecutor::ExecuteShl(unsigned nOps, SymbolicOperands *ops) {
	if (nOps < 2) DEBUG_BREAK;
	Z3_ast t1 = (Z3_ast)ops->sv[0];
	Z3_ast t2 = (Z3_ast)ops->sv[1];

	Z3_ast r = Z3_mk_bvshl(context, t1, t2);
	PRINTF_SYM("shl %p <= %p %p\n", r, t1, t2);
	PRINT_AST(context, r);
	return r;
}

Z3_ast Z3SymbolicExecutor::ExecuteShr(unsigned nOps, SymbolicOperands *ops) {
	if (nOps < 2) DEBUG_BREAK;
	Z3_ast t1 = (Z3_ast)ops->sv[0];
	Z3_ast t2 = (Z3_ast)ops->sv[1];

	Z3_ast r = Z3_mk_bvlshr(context, t1, t2);
	PRINTF_SYM("shr %p <= %p %p\n", r, t1, t2);
	PRINT_AST(context, r);
	return r;
}

Z3_ast Z3SymbolicExecutor::ExecuteSal(unsigned nOps, SymbolicOperands *ops) {
	DEBUG_BREAK;
	return nullptr;
}

Z3_ast Z3SymbolicExecutor::ExecuteSar(unsigned nOps, SymbolicOperands *ops) {
	if (nOps < 2) DEBUG_BREAK;
	Z3_ast t1 = (Z3_ast)ops->sv[0];
	Z3_ast t2 = (Z3_ast)ops->sv[1];

	Z3_ast r = Z3_mk_bvashr(context, t1, t2);
	PRINTF_SYM("sar %p <= %p %p\n", r, t1, t2);
	PRINT_AST(context, r);
	return r;
}

Z3_ast Z3SymbolicExecutor::ExecuteNot(unsigned nOps, SymbolicOperands *ops) {
	if (nOps < 1) DEBUG_BREAK;

	Z3_ast r = Z3_mk_not(context, (Z3_ast)ops->sv[0]);
	PRINTF_SYM("not %p <= %p\n", r, (Z3_ast)ops->sv[0]);
	PRINT_AST(context, r);
	return r;
}

Z3_ast Z3SymbolicExecutor::ExecuteNeg(unsigned nOps, SymbolicOperands *ops) {
	if (nOps < 1) DEBUG_BREAK;

	Z3_ast r = Z3_mk_bvneg(context, (Z3_ast)ops->sv[0]);
	PRINTF_SYM("neg %p <= %p\n", r, (Z3_ast)ops->sv[0]);
	env->SetOperand(0, r);
	PrintSetOperands(0);
	return r;
}

Z3_ast Z3SymbolicExecutor::ExecuteMul(unsigned nOps, SymbolicOperands *ops) {
	if (nOps < 3) DEBUG_BREAK;
	Z3_ast r = Z3_mk_bvmul(context, (Z3_ast)ops->sv[1], (Z3_ast)ops->sv[2]);
	PRINTF_SYM("mul %p <= %p %p\n", r, (Z3_ast)ops->sv[1], (Z3_ast)ops->sv[2]);
	env->SetOperand(0, r);
	PrintSetOperands(0);
	return r;
}

Z3_ast Z3SymbolicExecutor::ExecuteImul(unsigned nOps, SymbolicOperands *ops) {
	if (nOps < 3) DEBUG_BREAK;
	Z3_ast r = Z3_mk_bvmul(context, (Z3_ast)ops->sv[1], (Z3_ast)ops->sv[2]);
	PRINTF_SYM("imul %p <= %p %p\n", r, (Z3_ast)ops->sv[1], (Z3_ast)ops->sv[2]);
	env->SetOperand(0, r);
	PrintSetOperands(0);
	return r;
}

// AL AH AX reg
Z3_ast Z3SymbolicExecutor::ExecuteDiv(unsigned nOps, SymbolicOperands *ops) {
	if (nOps < 4) DEBUG_BREAK;
	Z3_ast quotient = Z3_mk_bvudiv(context, (Z3_ast)ops->sv[2], (Z3_ast)ops->sv[3]);
	Z3_ast remainder = Z3_mk_bvurem(context, (Z3_ast)ops->sv[2], (Z3_ast)ops->sv[3]);

	PRINTF_SYM("div %p %p <= %p %p\n", quotient, remainder, (Z3_ast)ops->sv[1], (Z3_ast)ops->sv[2]);

	env->SetOperand(0, quotient);
	PrintSetOperands(0);
	env->SetOperand(1, remainder);
	PrintSetOperands(1);
}

Z3_ast Z3SymbolicExecutor::ExecuteIdiv(unsigned nOps, SymbolicOperands *ops) {
	if (nOps < 4) DEBUG_BREAK;
	Z3_ast quotient = Z3_mk_bvsdiv(context, (Z3_ast)ops->sv[2], (Z3_ast)ops->sv[3]);
	Z3_ast remainder = Z3_mk_bvsrem(context, (Z3_ast)ops->sv[2], (Z3_ast)ops->sv[3]);

	PRINTF_SYM("idiv %p %p <= %p %p\n", quotient, remainder, (Z3_ast)ops->sv[1], (Z3_ast)ops->sv[2]);

	env->SetOperand(0, quotient);
	PrintSetOperands(0);
	env->SetOperand(1, remainder);
	PrintSetOperands(1);
}

void Z3SymbolicExecutor::SymbolicExecuteMov(RiverInstruction *instruction, SymbolicOperands *ops) {
	// mov dest, addr
	if (ops->tr[1]) {
		env->SetOperand(0, ops->sv[1]);
		PRINTF_SYM("mov <= %p[%d]\n", ops->sv[1],
				Z3_get_bv_sort_size(context, Z3_get_sort(context, (Z3_ast)ops->sv[1])));
		PrintSetOperands(0);
	} else {
		env->UnsetOperand(0);
	}
}

void Z3SymbolicExecutor::SymbolicExecuteMovSx(RiverInstruction *instruction, SymbolicOperands *ops) {
	if (ops->tr[1]) {
		Z3_ast dst = Z3_mk_sign_ext(context, 24, Z3_mk_extract(context, 7, 0, (Z3_ast)ops->sv[1]));
		env->SetOperand(0, (void *)dst);

		PRINTF_SYM("movsx %p <= %p\n", dst, ops->sv[1]);
		PrintSetOperands(0);
	}
	else {
		env->UnsetOperand(0);
	}
}

void Z3SymbolicExecutor::SymbolicExecuteMovS(RiverInstruction *instruction, SymbolicOperands *ops) {
	SymbolicExecuteMov(instruction, ops);
}

void Z3SymbolicExecutor::SymbolicExecuteMovZx(RiverInstruction *instruction, SymbolicOperands *ops) {
	// find size of given bitvector
	unsigned size = Z3_get_bv_sort_size(context, Z3_get_sort(context, (Z3_ast)ops->sv[1]));
	if (ops->tr[1]) {
		Z3_ast dst = Z3_mk_zero_ext(context, 32 - size, (Z3_ast)ops->sv[1]);
		env->SetOperand(0, (void *)dst);

		PRINTF_SYM("movzx %p <= %p[%d]\n", dst, ops->sv[1], size);
		PrintSetOperands(0);
	}
	else {
		env->UnsetOperand(0);
	}
}

void Z3SymbolicExecutor::SymbolicExecuteImul(RiverInstruction *instruction, SymbolicOperands *ops) {
	// all ops and result must have same sort size
	for (int i = 1; i <= 2; ++i) {
		if (ops->sv[i] == nullptr) {
			env->UnsetOperand(0);
		}
		if ( i == 2) return;
	}

	if (!CheckSameSort(2, (Z3_ast *)(ops->sv + 1))) DEBUG_BREAK;
	ops->sv[0] = Z3_mk_bvmul(context, (Z3_ast)ops->sv[1], (Z3_ast)ops->sv[2]);

	if (!CheckSameSort(2, (Z3_ast *)ops->sv)) DEBUG_BREAK;

	PRINTF_SYM("imul %p <= %p %p\n", ops->sv[0], ops->sv[1], ops->sv[2]);
	env->SetOperand(0, ops->sv[0]);
	PrintSetOperands(0);
}


// definitions of lookup index are found in header Z3_FLAG_OP_***
nodep::BYTE LookupUnsetFlags[] = {
	/*0x00*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/*0x10*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/*0x20*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/*0x30*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/*0x40*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/*0x50*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/*0x60*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/*0x70*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/*0x80*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/*0x90*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/*0xA0*/ 0, RIVER_SPEC_FLAG_CF | RIVER_SPEC_FLAG_OF | RIVER_SPEC_FLAG_AF,
	/*0xA2*/ 0, 0,
	/*0xA4*/ RIVER_SPEC_FLAG_CF | RIVER_SPEC_FLAG_OF | RIVER_SPEC_FLAG_AF, 0,
	/*0xA6*/ RIVER_SPEC_FLAG_CF | RIVER_SPEC_FLAG_OF | RIVER_SPEC_FLAG_AF, 0,
	/*0xA8*/ 0, 0,
	/*0xAA*/ 0, 0,
	/*0xAC*/ 0, 0,
	/*0xAE*/ 0, 0,
	/*0xB0*/ 0, 0,
	/*0xB2*/ 0, RIVER_SPEC_FLAG_SF | RIVER_SPEC_FLAG_ZF | RIVER_SPEC_FLAG_AF | RIVER_SPEC_FLAG_PF,
	/*0xB4*/ RIVER_SPEC_FLAG_CF | RIVER_SPEC_FLAG_OF | RIVER_SPEC_FLAG_SF | RIVER_SPEC_FLAG_AF | RIVER_SPEC_FLAG_PF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/*0xC0*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/*0xD0*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/*0xE0*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/*0xF0*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

template <Z3SymbolicExecutor::CommonOperation func, unsigned int funcCode> void Z3SymbolicExecutor::SymbolicExecuteCommonOperation(RiverInstruction *instruction, SymbolicOperands *ops) {

	SymbolicOperandsLazyFlags solf;
	InitLazyFlagsOperands(&solf, ops);

	Z3_ast ret = (this->*func)(4, ops);
	solf.svAfter[0] = ret;
	for (int i = 0; i < flagCount; ++i) {
		if (flagList[i] & instruction->modFlags) {
			if (LookupUnsetFlags[funcCode] & flagList[i]) {
				env->UnsetFlgValue(flagList[i]);
			} else {
				lazyFlags[i]->SetSource(solf, funcCode);
				env->SetFlgValue(flagList[i], lazyFlags[i]);
			}
		}
	}
}

template <Z3SymbolicExecutor::SymbolicExecute fSubOps[8]> void Z3SymbolicExecutor::SymbolicExecuteSubOp(RiverInstruction *instruction, SymbolicOperands *ops) {
	(this->*fSubOps[instruction->subOpCode])(instruction, ops);
}

void Z3SymbolicExecutor::GetSymbolicValues(RiverInstruction *instruction, SymbolicOperands *ops, nodep::DWORD mask) {
	nodep::DWORD opsFlagsMask = mask & ops->av;

	bool foundSort = false;
	Z3_sort operandsSort = dwordSort;
	for (int i = 0; i < opCount; ++i) {
		if ((OPERAND_BITMASK(i) & opsFlagsMask) && ops->tr[i]) {
			 Z3_sort localSort = Z3_get_sort(context, (Z3_ast)ops->sv[i]);
			 if (foundSort && !Z3_is_eq_sort(context, localSort, operandsSort)) {
				 DEBUG_BREAK;
			 }
			 if (!foundSort) {
				 operandsSort = localSort;
				 foundSort = true;
			 }
		}
	}

	for (int i = 0; i < opCount; ++i) {
		if ((OPERAND_BITMASK(i) & opsFlagsMask) && !ops->tr[i]) {
			ops->sv[i] = Z3_mk_int(context, ops->cvb[i], operandsSort);
			PRINTF_SYM("mkint %lu size: %u. Z3 adrr %p\n", ops->cvb[i],
					Z3_get_bv_sort_size(context, operandsSort), ops->sv[i]);
		}
	}

	for (int i = 0; i < flagCount; ++i) {
		if (flagList[i] & opsFlagsMask) {
			if (ops->trf[i]) {
				ops->svf[i] = ((Z3SymbolicCpuFlag *)ops->svf[i])->GetValue();
			} else {
				ops->svf[i] = Z3_mk_int(context, ops->cvfb[i], bitSort);
			}
		}
	}
}

void printoperand(struct OperandInfo oinfo) {
	PRINTF_INFO("[%d] operand: fields: %s %s %s\n",
		oinfo.opIdx,
		oinfo.fields & OP_HAS_SYMBOLIC ? "SYMBOLIC" : "",
		oinfo.fields & OP_HAS_CONCRETE_BEFORE ? "CONCRETE_BEFORE" : "",
		oinfo.fields & OP_HAS_CONCRETE_AFTER ? "CONCRETE_AFTER" : ""
	);

	if (oinfo.fields & OP_HAS_SYMBOLIC) {
		PRINTF_INFO("\tsymbolic : 0x%08lX\n", (DWORD)oinfo.symbolic);
	};

	if (oinfo.fields & OP_HAS_CONCRETE_BEFORE) {
		PRINTF_INFO("\tconcreteBefore : 0x%08lX\n", oinfo.concreteBefore);
	};

	if (oinfo.fields & OP_HAS_CONCRETE_AFTER) {
		PRINTF_INFO("\tconcreteAfter : 0x%08lX\n", oinfo.concreteAfter);
	};
}

void InitializeOperand(struct OperandInfo &oinfo) {
	oinfo.opIdx = -1;
	oinfo.fields = 0;
	oinfo.concreteBefore = 0;
	oinfo.concreteAfter = 0;
	oinfo.symbolic = nullptr;
}

void Z3SymbolicExecutor::ComposeScaleAndIndex(nodep::BYTE &scale,
		struct OperandInfo &indexOp) {

	indexOp.fields = OP_HAS_CONCRETE_BEFORE;
	indexOp.concreteBefore = 0;

	if (scale == 0) {
		indexOp.fields &= ~OP_HAS_SYMBOLIC;
		indexOp.symbolic = nullptr; //(void *)zero32;
		indexOp.concreteBefore = 0;
	}

	if (indexOp.fields & OP_HAS_SYMBOLIC) {
		// index has to be resolved if needConcat or needExtract
		Z3_sort opSort = Z3_get_sort(context, (Z3_ast)indexOp.symbolic);
		Z3_ast res = Z3_mk_bvmul(context, (Z3_ast)indexOp.symbolic,
				Z3_mk_int(context, scale, opSort));
		PRINTF_SYM("<sym> add %p <= %d + %p\n",
				res, scale, indexOp.symbolic);
		indexOp.symbolic = (void *)res;
	} else {
		indexOp.concreteBefore = scale * indexOp.concreteBefore;
	}
}

void Z3SymbolicExecutor::AddOperands(struct OperandInfo &left,
		struct OperandInfo &right,
		unsigned displacement,
		struct OperandInfo &result)
{
	if (left.opIdx == right.opIdx) {
		result.opIdx = left.opIdx;
	} else {
		DEBUG_BREAK;
	}
	//result.concrete = 0;
	
	result.fields = left.fields & right.fields;
	result.fields |= (left.fields & OP_HAS_SYMBOLIC) | (right.fields & OP_HAS_SYMBOLIC);

	if (result.fields & OP_HAS_CONCRETE_BEFORE) {
		result.concreteBefore = left.concreteBefore + right.concreteBefore + displacement;
	}

	if (result.fields & OP_HAS_CONCRETE_AFTER) {
		result.concreteAfter = left.concreteAfter + right.concreteAfter + displacement;
	}

	if (result.fields & OP_HAS_SYMBOLIC) {
		if (0 == (left.fields & OP_HAS_SYMBOLIC)) {
			if (0 == (left.concreteBefore + displacement)) {
				result = right;
				return;
			}

			left.symbolic = Z3_mk_int(context, left.concreteBefore + displacement, dwordSort);

			if (left.concreteBefore) {
				left.fields |= OP_HAS_SYMBOLIC;
			}

			PRINTF_SYM("<sym> mkint left %p <= %08lX\n", left.symbolic, left.concreteBefore + displacement);
		}

		if (0 == (right.fields & OP_HAS_SYMBOLIC)) {
			if (0 == (right.concreteBefore + displacement)) {
				result = left;
				return;
			}

			right.symbolic = Z3_mk_int(context, right.concreteBefore + displacement, dwordSort);

			if (right.concreteBefore) {
				right.fields |= OP_HAS_SYMBOLIC;
			}

			PRINTF_SYM("<sym> mkint right %p <= %08lX + %d\n", right.symbolic, right.concreteBefore, displacement);
		}

		// add two symbolic objects
		result.symbolic = Z3_mk_bvadd(context, (Z3_ast)left.symbolic,
			(Z3_ast)right.symbolic);
		PRINTF_SYM("<sym> add %p <= %p + %p\n", result.symbolic, left.symbolic,
			right.symbolic);
	}
}

void Z3SymbolicExecutor::OnExecutionControl(const rev::BasicBlockInfo& bblock)
{
	m_lastConcolicTest.pathBBlocks.insert((DWORD)bblock.address);
}

void Z3SymbolicExecutor::Execute(RiverInstruction *instruction) {
	SymbolicOperands ops;

	// Check if there is any pending jump instruction waiting for result to be filled
	if (m_lastConcolicTest.isPending())
	{
		m_lastConcolicTest.taken = ((DWORD)(currentBasicBlockExecuted.address) == m_lastConcolicTest.blockOptionTaken);
		aFormat->WriteZ3SymbolicJumpCC(m_lastConcolicTest);
		m_lastConcolicTest.Reset();
	}

	ops.av = 0;
	nodep::BOOL uo[4], uof[flagCount];
	nodep::BOOL isSymb = false;

	for (int i = 0; i < opCount; ++i) {
		struct OperandInfo opInfo;
		opInfo.opIdx = (nodep::BYTE)i;
		opInfo.fields = 0;

		if (true == (uo[i] = env->GetOperand(opInfo))) {
			ops.av |= OPERAND_BITMASK(i);
			isSymb |= (opInfo.fields & OP_HAS_SYMBOLIC) != 0;
			if (isSymb)
				printoperand(opInfo);
		}

		ops.tr[i] = (0 != (opInfo.fields & OP_HAS_SYMBOLIC));
		ops.cvb[i] = opInfo.concreteBefore;
		ops.cva[i] = opInfo.concreteAfter;
		ops.sv[i] = opInfo.symbolic;

		struct OperandInfo baseOpInfo, indexOpInfo, composedIndexOpInfo;
		bool hasIndex = false, hasBase = false;

		InitializeOperand(baseOpInfo);
		baseOpInfo.opIdx = i;
		hasBase = env->GetAddressBase(baseOpInfo);

		InitializeOperand(indexOpInfo);
		nodep::BYTE scale = 0;
		indexOpInfo.opIdx = i;
		hasIndex = env->GetAddressScaleAndIndex(indexOpInfo, scale);

		struct OperandInfo opAddressInfo;
		composedIndexOpInfo = indexOpInfo;
		if (hasIndex) {
			ComposeScaleAndIndex(scale, composedIndexOpInfo);
		}

		struct AddressDisplacement addressDisplacement;
		bool hasDisplacement = env->GetAddressDisplacement(i, addressDisplacement);

		AddOperands(baseOpInfo, composedIndexOpInfo,
				hasDisplacement ? addressDisplacement.disp : 0,
				opAddressInfo);

#if 0  // Symbolic address tracking are disabled for now - TODO
		if (opAddressInfo.fields & OP_HAS_SYMBOLIC) {
			SymbolicAddress sa = {
				.symbolicBase = (unsigned int)baseOpInfo.symbolic,
				.scale = (unsigned int)scale,
				.symbolicIndex = (unsigned int)indexOpInfo.symbolic,
				.composedSymbolicAddress = (unsigned int)opAddressInfo.symbolic,
				.dispType = 0,
				.displacement = 0,
				.inputOutput = 0
			};

			if ((RIVER_SPEC_MODIFIES_OP(i) | RIVER_SPEC_IGNORES_OP(i)) & instruction->specifiers) {
				sa.inputOutput |= OUTPUT_ADDR;
			} else if (RIVER_SPEC_MODIFIES_OP(i) & instruction->specifiers) {
				sa.inputOutput |= INPUT_ADDR | OUTPUT_ADDR;
			} else if (RIVER_SPEC_IGNORES_OP(i) & instruction->specifiers) {
				DEBUG_BREAK;
			} else {
				sa.inputOutput |= INPUT_ADDR;
			}

			if (hasDisplacement) {
				sa.displacement = addressDisplacement.disp;
				if (addressDisplacement.type & RIVER_ADDR_DISP8) {
					sa.dispType = DISP8;
				} else if (addressDisplacement.type & RIVER_ADDR_DISP) {
					sa.dispType = DISP;
				} else {
					DEBUG_BREAK;
				}
			}

			TranslateAddressToBasicBlockPointer(&sa.bbp,
					instruction->instructionAddress,
					mCount, mInfo);

			SymbolicAst sast = {
				.address = AstToBenchmarkString((Z3_ast)sa.composedSymbolicAddress, instruction, "address_symbol"),
			};
			sast.size = strlen(sast.address);

			aFormat->WriteZ3SymbolicAddress(0, sa, sast);
			PRINTF_SYM("address %p <= %d * %p + %p\n", opAddressInfo.symbolic,
					scale, indexOpInfo.symbolic, baseOpInfo.symbolic);
		}
		
#endif
	}

	for (int i = 0; i < flagCount; ++i) {
		struct FlagInfo flagInfo;
		flagInfo.opIdx = flagList[i];
		flagInfo.fields = 0;

		if (true == (uof[i] = env->GetFlgValue(flagInfo))) {
			ops.av |= flagInfo.opIdx;
			isSymb |= (0 != (flagInfo.fields & OP_HAS_SYMBOLIC));
		}
		ops.trf[i] = (0 != (flagInfo.fields & OP_HAS_SYMBOLIC));
		ops.cvfb[i] = flagInfo.concreteBefore;
		ops.cvfa[i] = flagInfo.concreteAfter;
		ops.svf[i] = flagInfo.symbolic;
	}

	if (isSymb) {
		nodep::DWORD dwTable = (instruction->modifiers & RIVER_MODIFIER_EXT) ? 1 : 0;
		Z3SymbolicExecutor::SymbolicExecute unkfunc =  &Z3SymbolicExecutor::SymbolicExecuteUnk;
		if (executeFuncs[dwTable][instruction->opCode] != unkfunc)
		{

			// This functionality must be moved into individual functions if the need arises
			GetSymbolicValues(instruction, &ops, ops.av);

			PRINTF_INFO("<info> Execute instruction: 0x%08lX\n", instruction->instructionAddress);
			(this->*executeFuncs[dwTable][instruction->opCode])(instruction, &ops);
		}
	} else {
		PRINTF_INFO("<info> Instruction is not symbolic: 0x%08lX\n", instruction->instructionAddress);
		// unset all modified operands
		for (int i = 0; i < opCount; ++i) {
			if (RIVER_SPEC_MODIFIES_OP(i) & instruction->specifiers) {
				if (ops.sv[i] != nullptr ||
						(RIVER_SPEC_IGNORES_OP(i) & instruction->specifiers)) {
					env->UnsetOperand(i);
				}
			}
		}

		// unset all modified flags
		for (int i = 0; i < flagCount; ++i) {
			if (flagList[i] & instruction->modFlags) {
				if (ops.svf[i] != nullptr) {
					env->UnsetFlgValue(flagList[i]);
				}
			}
		}
	}
}

#define Z3FLAG(f) &Z3SymbolicExecutor::Flag<(f)>
#define Z3NOT(f) &Z3SymbolicExecutor::Negate<(f)>
#define Z3EQUALS(f1, f2) &Z3SymbolicExecutor::Equals< (f1), (f2) >
#define Z3OR(f1, f2) &Z3SymbolicExecutor::Or< (f1), (f2) >




Z3SymbolicExecutor::SymbolicExecute Z3SymbolicExecutor::executeFuncs[2][0x100] = {
	{
		/*0x00*/ &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteAdd, Z3_FLAG_OP_ADD>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteAdd, Z3_FLAG_OP_ADD>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteAdd, Z3_FLAG_OP_ADD>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteAdd, Z3_FLAG_OP_ADD>,
		/*0x04*/ &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteAdd, Z3_FLAG_OP_ADD>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteAdd, Z3_FLAG_OP_ADD>, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x08*/ &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteOr,  Z3_FLAG_OP_OR>,  &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteOr,  Z3_FLAG_OP_OR>,  &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteOr,  Z3_FLAG_OP_OR>,  &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteOr,  Z3_FLAG_OP_OR>,
		/*0x0C*/ &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteOr,  Z3_FLAG_OP_OR>,  &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteOr,  Z3_FLAG_OP_OR>,  &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x10*/ &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteAdc, Z3_FLAG_OP_ADC>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteAdc, Z3_FLAG_OP_ADC>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteAdc, Z3_FLAG_OP_ADC>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteAdc, Z3_FLAG_OP_ADC>,
		/*0x14*/ &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteAdc, Z3_FLAG_OP_ADC>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteAdc, Z3_FLAG_OP_ADC>, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x18*/ &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteSbb, Z3_FLAG_OP_SBB>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteSbb, Z3_FLAG_OP_SBB>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteSbb, Z3_FLAG_OP_SBB>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteSbb, Z3_FLAG_OP_SBB>,
		/*0x1C*/ &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteSbb, Z3_FLAG_OP_SBB>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteSbb, Z3_FLAG_OP_SBB>, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x20*/ &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteAnd, Z3_FLAG_OP_AND>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteAnd, Z3_FLAG_OP_AND>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteAnd, Z3_FLAG_OP_AND>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteAnd, Z3_FLAG_OP_AND>,
		/*0x24*/ &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteAnd, Z3_FLAG_OP_AND>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteAnd, Z3_FLAG_OP_AND>, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x28*/ &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteSub, Z3_FLAG_OP_SUB>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteSub, Z3_FLAG_OP_SUB>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteSub, Z3_FLAG_OP_SUB>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteSub, Z3_FLAG_OP_SUB>,
		/*0x2C*/ &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteSub, Z3_FLAG_OP_SUB>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteSub, Z3_FLAG_OP_SUB>, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x30*/ &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteXor, Z3_FLAG_OP_XOR>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteXor, Z3_FLAG_OP_XOR>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteXor, Z3_FLAG_OP_XOR>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteXor, Z3_FLAG_OP_XOR>,
		/*0x34*/ &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteXor, Z3_FLAG_OP_XOR>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteXor, Z3_FLAG_OP_XOR>, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x38*/ &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteCmp, Z3_FLAG_OP_CMP>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteCmp, Z3_FLAG_OP_CMP>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteCmp, Z3_FLAG_OP_CMP>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteCmp, Z3_FLAG_OP_CMP>,
		/*0x3C*/ &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteCmp, Z3_FLAG_OP_CMP>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteCmp, Z3_FLAG_OP_CMP>, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x40*/ &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteInc, Z3_FLAG_OP_INC>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteInc, Z3_FLAG_OP_INC>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteInc, Z3_FLAG_OP_INC>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteInc, Z3_FLAG_OP_INC>,
		/*0x44*/ &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteInc, Z3_FLAG_OP_INC>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteInc, Z3_FLAG_OP_INC>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteInc, Z3_FLAG_OP_INC>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteInc, Z3_FLAG_OP_INC>,
		/*0x48*/ &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteDec, Z3_FLAG_OP_INC>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteDec, Z3_FLAG_OP_INC>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteDec, Z3_FLAG_OP_INC>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteDec, Z3_FLAG_OP_INC>,
		/*0x4C*/ &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteDec, Z3_FLAG_OP_INC>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteDec, Z3_FLAG_OP_INC>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteDec, Z3_FLAG_OP_INC>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteDec, Z3_FLAG_OP_INC>,

		/*0x50*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x54*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x58*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x5C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x60*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x64*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x68*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteImul, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x6C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x70*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3FLAG(RIVER_SPEC_IDX_OF)>,
		/*0x71*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3NOT(Z3FLAG(RIVER_SPEC_IDX_OF))>,
		/*0x72*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3FLAG(RIVER_SPEC_IDX_CF)>,
		/*0x73*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3NOT(Z3FLAG(RIVER_SPEC_IDX_CF))>,
		/*0x74*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3FLAG(RIVER_SPEC_IDX_ZF)>,
		/*0x75*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3NOT(Z3FLAG(RIVER_SPEC_IDX_ZF))>,
		/*0x76*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3OR(Z3FLAG(RIVER_SPEC_IDX_ZF), Z3FLAG(RIVER_SPEC_IDX_CF))>,
		/*0x77*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3NOT(Z3OR(Z3FLAG(RIVER_SPEC_IDX_ZF), Z3FLAG(RIVER_SPEC_IDX_CF)))>,
		/*0x78*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3FLAG(RIVER_SPEC_IDX_SF)>,
		/*0x79*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3NOT(Z3FLAG(RIVER_SPEC_IDX_SF))>,
		/*0x7A*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3FLAG(RIVER_SPEC_IDX_PF)>,
		/*0x7B*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3NOT(Z3FLAG(RIVER_SPEC_IDX_PF))>,
		/*0x7C*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3NOT(Z3EQUALS(Z3FLAG(RIVER_SPEC_IDX_SF), Z3FLAG(RIVER_SPEC_IDX_OF)))>,
		/*0x7D*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3EQUALS(Z3FLAG(RIVER_SPEC_IDX_SF), Z3FLAG(RIVER_SPEC_IDX_OF))>,
		/*0x7E*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3OR(Z3FLAG(RIVER_SPEC_IDX_ZF), Z3NOT(Z3EQUALS(Z3FLAG(RIVER_SPEC_IDX_SF), Z3FLAG(RIVER_SPEC_IDX_OF))))>,
		/*0x7F*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3NOT(Z3OR(Z3FLAG(RIVER_SPEC_IDX_ZF), Z3NOT(Z3EQUALS(Z3FLAG(RIVER_SPEC_IDX_SF), Z3FLAG(RIVER_SPEC_IDX_OF)))))>,

		/*0x80*/ &Z3SymbolicExecutor::SymbolicExecuteSubOp<Z3SymbolicExecutor::executeAssignmentOperations>, &Z3SymbolicExecutor::SymbolicExecuteSubOp<Z3SymbolicExecutor::executeAssignmentOperations>, &Z3SymbolicExecutor::SymbolicExecuteSubOp<Z3SymbolicExecutor::executeAssignmentOperations>, &Z3SymbolicExecutor::SymbolicExecuteSubOp<Z3SymbolicExecutor::executeAssignmentOperations>,
		/*0x84*/ &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteTest, Z3_FLAG_OP_AND>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteTest, Z3_FLAG_OP_AND>, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x88*/ &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov,
		/*0x8C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x90*/ &Z3SymbolicExecutor::SymbolicExecuteNop, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x94*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x98*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x9C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0xA0*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xA4*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteMovS, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xA8*/ &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteTest, Z3_FLAG_OP_AND>, &Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteTest, Z3_FLAG_OP_AND>, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xAC*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0xB0*/ &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov,
		/*0xB4*/ &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov,
		/*0xB8*/ &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov,
		/*0xBC*/ &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov,

		/*0xC0*/ &Z3SymbolicExecutor::SymbolicExecuteSubOp<Z3SymbolicExecutor::executeRotationOperations>, &Z3SymbolicExecutor::SymbolicExecuteSubOp<Z3SymbolicExecutor::executeRotationOperations>, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xC4*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteMov,
		/*0xC8*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xCC*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0xD0*/ &Z3SymbolicExecutor::SymbolicExecuteSubOp<Z3SymbolicExecutor::executeRotationOperations>, &Z3SymbolicExecutor::SymbolicExecuteSubOp<Z3SymbolicExecutor::executeRotationOperations>, &Z3SymbolicExecutor::SymbolicExecuteSubOp<Z3SymbolicExecutor::executeRotationOperations>, &Z3SymbolicExecutor::SymbolicExecuteSubOp<Z3SymbolicExecutor::executeRotationOperations>,
		/*0xD4*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xD8*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xDC*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0xE0*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xE4*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xE8*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xEC*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0xF0*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xF4*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteSubOp<Z3SymbolicExecutor::executeAssignmentLogicalOperations>, &Z3SymbolicExecutor::SymbolicExecuteSubOp<Z3SymbolicExecutor::executeAssignmentLogicalOperations>,
		/*0xF8*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xFC*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk
	},{
		/*0x00*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x04*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x08*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x0C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x10*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x14*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x18*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x1C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x20*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x24*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x28*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x2C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x30*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x34*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x38*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x3C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x40*/ &Z3SymbolicExecutor::SymbolicCmovCC<Z3FLAG(RIVER_SPEC_IDX_OF)>,
		/*0x41*/ &Z3SymbolicExecutor::SymbolicCmovCC<Z3NOT(Z3FLAG(RIVER_SPEC_IDX_OF))>,
		/*0x42*/ &Z3SymbolicExecutor::SymbolicCmovCC<Z3FLAG(RIVER_SPEC_IDX_CF)>,
		/*0x43*/ &Z3SymbolicExecutor::SymbolicCmovCC<Z3NOT(Z3FLAG(RIVER_SPEC_IDX_CF))>,
		/*0x44*/ &Z3SymbolicExecutor::SymbolicCmovCC<Z3FLAG(RIVER_SPEC_IDX_ZF)>,
		/*0x45*/ &Z3SymbolicExecutor::SymbolicCmovCC<Z3NOT(Z3FLAG(RIVER_SPEC_IDX_ZF))>,
		/*0x46*/ &Z3SymbolicExecutor::SymbolicCmovCC<Z3OR(Z3FLAG(RIVER_SPEC_IDX_ZF), Z3FLAG(RIVER_SPEC_IDX_CF))>,
		/*0x47*/ &Z3SymbolicExecutor::SymbolicCmovCC<Z3NOT(Z3OR(Z3FLAG(RIVER_SPEC_IDX_ZF), Z3FLAG(RIVER_SPEC_IDX_CF)))>,
		/*0x48*/ &Z3SymbolicExecutor::SymbolicCmovCC<Z3FLAG(RIVER_SPEC_IDX_SF)>,
		/*0x49*/ &Z3SymbolicExecutor::SymbolicCmovCC<Z3NOT(Z3FLAG(RIVER_SPEC_IDX_SF))>,
		/*0x4A*/ &Z3SymbolicExecutor::SymbolicCmovCC<Z3FLAG(RIVER_SPEC_IDX_PF)>,
		/*0x4B*/ &Z3SymbolicExecutor::SymbolicCmovCC<Z3NOT(Z3FLAG(RIVER_SPEC_IDX_PF))>,
		/*0x4C*/ &Z3SymbolicExecutor::SymbolicCmovCC<Z3NOT(Z3EQUALS(Z3FLAG(RIVER_SPEC_IDX_SF), Z3FLAG(RIVER_SPEC_IDX_OF)))>,
		/*0x4D*/ &Z3SymbolicExecutor::SymbolicCmovCC<Z3EQUALS(Z3FLAG(RIVER_SPEC_IDX_SF), Z3FLAG(RIVER_SPEC_IDX_OF))>,
		/*0x4E*/ &Z3SymbolicExecutor::SymbolicCmovCC<Z3OR(Z3FLAG(RIVER_SPEC_IDX_ZF), Z3NOT(Z3EQUALS(Z3FLAG(RIVER_SPEC_IDX_SF), Z3FLAG(RIVER_SPEC_IDX_OF))))>,
		/*0x4F*/ &Z3SymbolicExecutor::SymbolicCmovCC<Z3NOT(Z3OR(Z3FLAG(RIVER_SPEC_IDX_ZF), Z3NOT(Z3EQUALS(Z3FLAG(RIVER_SPEC_IDX_SF), Z3FLAG(RIVER_SPEC_IDX_OF)))))>,

		/*0x50*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x54*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x58*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x5C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x60*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x64*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x68*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x6C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x70*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x74*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x78*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x7C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x80*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3FLAG(RIVER_SPEC_IDX_OF)>,
		/*0x81*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3NOT(Z3FLAG(RIVER_SPEC_IDX_OF))>,
		/*0x82*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3FLAG(RIVER_SPEC_IDX_CF)>,
		/*0x83*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3NOT(Z3FLAG(RIVER_SPEC_IDX_CF))>,
		/*0x84*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3FLAG(RIVER_SPEC_IDX_ZF)>,
		/*0x85*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3NOT(Z3FLAG(RIVER_SPEC_IDX_ZF))>,
		/*0x86*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3OR(Z3FLAG(RIVER_SPEC_IDX_ZF), Z3FLAG(RIVER_SPEC_IDX_CF))>,
		/*0x87*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3NOT(Z3OR(Z3FLAG(RIVER_SPEC_IDX_ZF), Z3FLAG(RIVER_SPEC_IDX_CF)))>,
		/*0x88*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3FLAG(RIVER_SPEC_IDX_SF)>,
		/*0x89*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3NOT(Z3FLAG(RIVER_SPEC_IDX_SF))>,
		/*0x8A*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3FLAG(RIVER_SPEC_IDX_PF)>,
		/*0x8B*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3NOT(Z3FLAG(RIVER_SPEC_IDX_PF))>,
		/*0x8C*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3NOT(Z3EQUALS(Z3FLAG(RIVER_SPEC_IDX_SF), Z3FLAG(RIVER_SPEC_IDX_OF)))>,
		/*0x8D*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3EQUALS(Z3FLAG(RIVER_SPEC_IDX_SF), Z3FLAG(RIVER_SPEC_IDX_OF))>,
		/*0x8E*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3OR(Z3FLAG(RIVER_SPEC_IDX_ZF), Z3NOT(Z3EQUALS(Z3FLAG(RIVER_SPEC_IDX_SF), Z3FLAG(RIVER_SPEC_IDX_OF))))>,
		/*0x8F*/ &Z3SymbolicExecutor::SymbolicJumpCC<Z3NOT(Z3OR(Z3FLAG(RIVER_SPEC_IDX_ZF), Z3NOT(Z3EQUALS(Z3FLAG(RIVER_SPEC_IDX_SF), Z3FLAG(RIVER_SPEC_IDX_OF)))))>,
		/*0x90*/ &Z3SymbolicExecutor::SymbolicSetCC<Z3FLAG(RIVER_SPEC_IDX_OF)>,
		/*0x91*/ &Z3SymbolicExecutor::SymbolicSetCC<Z3NOT(Z3FLAG(RIVER_SPEC_IDX_OF))>,
		/*0x92*/ &Z3SymbolicExecutor::SymbolicSetCC<Z3FLAG(RIVER_SPEC_IDX_CF)>,
		/*0x93*/ &Z3SymbolicExecutor::SymbolicSetCC<Z3NOT(Z3FLAG(RIVER_SPEC_IDX_CF))>,
		/*0x94*/ &Z3SymbolicExecutor::SymbolicSetCC<Z3FLAG(RIVER_SPEC_IDX_ZF)>,
		/*0x95*/ &Z3SymbolicExecutor::SymbolicSetCC<Z3NOT(Z3FLAG(RIVER_SPEC_IDX_ZF))>,
		/*0x96*/ &Z3SymbolicExecutor::SymbolicSetCC<Z3OR(Z3FLAG(RIVER_SPEC_IDX_ZF), Z3FLAG(RIVER_SPEC_IDX_CF))>,
		/*0x97*/ &Z3SymbolicExecutor::SymbolicSetCC<Z3NOT(Z3OR(Z3FLAG(RIVER_SPEC_IDX_ZF), Z3FLAG(RIVER_SPEC_IDX_CF)))>,
		/*0x98*/ &Z3SymbolicExecutor::SymbolicSetCC<Z3FLAG(RIVER_SPEC_IDX_SF)>,
		/*0x99*/ &Z3SymbolicExecutor::SymbolicSetCC<Z3NOT(Z3FLAG(RIVER_SPEC_IDX_SF))>,
		/*0x9A*/ &Z3SymbolicExecutor::SymbolicSetCC<Z3FLAG(RIVER_SPEC_IDX_PF)>,
		/*0x9B*/ &Z3SymbolicExecutor::SymbolicSetCC<Z3NOT(Z3FLAG(RIVER_SPEC_IDX_PF))>,
		/*0x9C*/ &Z3SymbolicExecutor::SymbolicSetCC<Z3NOT(Z3EQUALS(Z3FLAG(RIVER_SPEC_IDX_SF), Z3FLAG(RIVER_SPEC_IDX_OF)))>,
		/*0x9D*/ &Z3SymbolicExecutor::SymbolicSetCC<Z3EQUALS(Z3FLAG(RIVER_SPEC_IDX_SF), Z3FLAG(RIVER_SPEC_IDX_OF))>,
		/*0x9E*/ &Z3SymbolicExecutor::SymbolicSetCC<Z3OR(Z3FLAG(RIVER_SPEC_IDX_ZF), Z3NOT(Z3EQUALS(Z3FLAG(RIVER_SPEC_IDX_SF), Z3FLAG(RIVER_SPEC_IDX_OF))))>,
		/*0x9F*/ &Z3SymbolicExecutor::SymbolicSetCC<Z3NOT(Z3OR(Z3FLAG(RIVER_SPEC_IDX_ZF), Z3NOT(Z3EQUALS(Z3FLAG(RIVER_SPEC_IDX_SF), Z3FLAG(RIVER_SPEC_IDX_OF)))))>,
		/*0xA0*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xA4*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xA8*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xAC*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0xB0*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteCmpxchg, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xB4*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteMovZx, &Z3SymbolicExecutor::SymbolicExecuteMovZx,
		/*0xB8*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xBC*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteMovSx, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0xC0*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xC4*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xC8*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xCC*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0xD0*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xD4*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xD8*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xDC*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0xE0*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xE4*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xE8*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xEC*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0xF0*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xF4*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xF8*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xFC*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk
	}
};

Z3SymbolicExecutor::SymbolicExecute Z3SymbolicExecutor::executeAssignmentOperations[8] = {
	&Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteAdd, Z3_FLAG_OP_ADD>,
	&Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteOr,  Z3_FLAG_OP_OR>,
	&Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteAdc, Z3_FLAG_OP_ADC>,
	&Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteSbb, Z3_FLAG_OP_SBB>,
	&Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteAnd, Z3_FLAG_OP_AND>,
	&Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteSub, Z3_FLAG_OP_SUB>,
	&Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteXor, Z3_FLAG_OP_XOR>,
	&Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteCmp, Z3_FLAG_OP_CMP>
};

Z3SymbolicExecutor::SymbolicExecute Z3SymbolicExecutor::executeRotationOperations[8] = {
	&Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteRol, Z3_FLAG_OP_ROL>,
	&Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteRor, Z3_FLAG_OP_ROR>,
	&Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteRcl, Z3_FLAG_OP_RCL>,
	&Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteRcr, Z3_FLAG_OP_RCR>,
	&Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteShl, Z3_FLAG_OP_SHL>,
	&Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteShr, Z3_FLAG_OP_SHR>,
	&Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteSal, Z3_FLAG_OP_SAL>,
	&Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteSar, Z3_FLAG_OP_SAR>
};

Z3SymbolicExecutor::SymbolicExecute Z3SymbolicExecutor::executeAssignmentLogicalOperations[8] = {
	&Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteTest, Z3_FLAG_OP_AND>,
	&Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteTest, Z3_FLAG_OP_AND>,
	&Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteNot, Z3_FLAG_OP_NOT>,
	&Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteNeg, Z3_FLAG_OP_NEG>,
	&Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteMul, Z3_FLAG_OP_MUL>,
	&Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteImul, Z3_FLAG_OP_MUL>,
	&Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteDiv, Z3_FLAG_OP_DIV>,
	&Z3SymbolicExecutor::SymbolicExecuteCommonOperation<&Z3SymbolicExecutor::ExecuteIdiv, Z3_FLAG_OP_DIV>
};
