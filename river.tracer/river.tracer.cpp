#include "ezOptionParser.h"
#include "z3.h"

#include "simple.tracer.h"

// annotated tracer header dependencies
#include "SymbolicEnvironment/Environment.h"
#include "TrackingExecutor.h"
#include "annotated.tracer.h"

#include "Execution/Execution.h"
#include "CommonCrossPlatform/Common.h"

#ifdef _WIN32
#define LIB_EXT ".dll"
#else
#define LIB_EXT ".so"
#endif

void afisareee() {
	Z3_config config = Z3_mk_config();
    Z3_context context = Z3_mk_context(config);
	 Z3_solver solver = Z3_mk_solver(context);
	const char *result2 = "(declare-fun jump_symbol () (_ BitVec 1)) (declare-fun |s[0]| () (_ BitVec 8)) (assert (= (ite (= |s[0]| (_ bv65 8)) (_ bv1 1) (_ bv0 1)) jump_symbol))";

	 Z3_ast fs = Z3_parse_smtlib2_string(context, result2, 0, 0, 0, 0, 0, 0);
	 Z3_solver_assert(context, solver, fs);
	 int result_solver = Z3_solver_check ((Z3_context) context, (Z3_solver) solver);
	 //printf("Scopes : %d\n", Z3_solver_get_num_scopes((Z3_context) context, (Z3_solver) solver));
	 //printf("%s \n", Z3_ast_to_string(context, fs));
	Z3_model model = Z3_solver_get_model ((Z3_context) context,  (Z3_solver) solver);
	printf("Model : %s\n", Z3_model_to_string ((Z3_context) context,  model));
	
	unsigned n = Z3_model_get_num_consts(context, model);
		for(unsigned i = 0; i < n; i++) {
			//unsigned Z3_api rezultat = Z3_model_get_num_funcs_decl(context, model, i);
			Z3_func_decl fd = Z3_model_get_const_decl(context, model, i);

			if (Z3_model_has_interp(context, model, fd))
			{
				Z3_ast s = Z3_model_get_const_interp(context, model, fd);
				printf("%s \n", Z3_ast_to_string(context, s));
			}
				
  			//Z3_ast litle_assert =  Z3_model_get_func_interp(context, model, fd);
			printf("%s \n ",Z3_func_decl_to_string (context, fd));

		}
		

	/* // CODUL 1
	const char *result2 = "(declare-fun jump_symbol () (_ BitVec 1)) (declare-fun |s[0]| () (_ BitVec 8)) (assert (= (ite (= |s[0]| (_ bv65 8)) (_ bv1 1) (_ bv0 1)) jump_symbol))";
	Z3_config cfg = Z3_mk_config();
    Z3_context ctx = Z3_mk_context(cfg);
    Z3_ast fs = Z3_parse_smtlib2_string(ctx, result2, 0, 0, 0, 0, 0, 0);
    Z3_inc_ref(ctx, fs);
    Z3_solver solver2 = Z3_mk_solver(ctx);
    Z3_solver_inc_ref(ctx, solver2);
    Z3_solver_assert(ctx, solver2, fs);
    Z3_solver_check(ctx, solver2);
    Z3_model m = Z3_solver_get_model(ctx, solver2);
	printf("%s \n", Z3_model_to_string(ctx, m));
    Z3_model_inc_ref(ctx, m);
    Z3_solver_check(ctx, solver2);


    // work with model

    Z3_solver_dec_ref(ctx, solver2);
    Z3_model_dec_ref(ctx, m);
    Z3_dec_ref(ctx, fs);
    Z3_del_config(cfg);
	*/
}

at::AnnotatedTracer *AT = nullptr;

void __stdcall SymbolicHandler(void *ctx, void *offset, void *addr) {
	RiverInstruction *instr = (RiverInstruction *)addr;

	if (AT != nullptr) {
		AT->observer.regEnv->SetCurrentInstruction(instr, offset);
		AT->observer.executor->Execute(instr);
	}
}

int main(int argc, const char *argv[]) {
	//afisareee();
	ez::ezOptionParser opt;

	opt.overview = "River simple tracer.";
	opt.syntax = "tracer.simple [OPTIONS]";
	opt.example = "tracer.simple -o<outfile>\n";

	opt.add(
			"",
			0,
			0,
			0,
			"Use inprocess execution.",
			"--inprocess"
		   );

	opt.add(
			"",
			0,
			0,
			0,
			"Disable logs",
			"--disableLogs"
		   );

	opt.add(
			"",
			0,
			0,
			0,
			"write Log On File",
			"--writeLogOnFile"
		   );



	opt.add(
			"",
			0,
			0,
			0,
			"Use extern execution.",
			"--extern"
		   );

	opt.add(
			"", // Default.
			0, // Required?
			0, // Number of args expected.
			0, // Delimiter if expecting multiple args.
			"Display usage instructions.", // Help description.
			"-h",     // Flag token. 
			"--help", // Flag token.
			"--usage" // Flag token.
		   );

	opt.add(
			"trace.simple.out", // Default.
			0, // Required?
			1, // Number of args expected.
			0, // Delimiter if expecting multiple args.
			"Set the trace output file.", // Help description.
			"-o",			 // Flag token.
			"--outfile"     // Flag token. 
		   );

	opt.add(
			"",
			false,
			0,
			0,
			"Use binary logging instead of textual logging.",
			"--binlog"
		   );

	opt.add(
			"",
			false,
			0,
			0,
			"Use Z3 for symbolic execution expressions.",
			"--z3"
		   );

	opt.add(
			"",
			false,
			0,
			0,
			"When using binlog, buffer everything before writing the result",
			"--binbuffered"
		   );

	opt.add(
			"",
			false,
			0,
			0,
			"Use a corpus file instead of individual inputs.",
			"--batch"
		   );

	opt.add(
			"",
			false,
			0,
			0,
			"Use this to create a flow of input payload- trace output. Input is fed from a different process",
			"--flow"
		   );

	opt.add(
			"",
			false,
			0,
			0,
			"River generated annotated traces using taint analysis.",
			"--annotated"
		   );

	opt.add(
			"payload" LIB_EXT,
			0,
			1,
			0,
			"Set the payload file. Only applicable for in-process tracing.",
			"-p",
			"--payload"
		   );

	opt.add(
			"",
			0,
			1,
			0,
			"Set the memory patching file.",
			"-m",
			"--mem-patch"
		   );

	opt.parse(argc, argv);

	if (opt.isSet("--inprocess") && opt.isSet("--extern")) {
		std::cerr << "Conflicting options --inprocess and --extern" << std::endl;
		return 0;
	}

	if (opt.isSet("-h")) {
		std::string usage;
		opt.getUsage(usage);
		std::cerr << usage;
		return 0;
	}

	if (opt.isSet("--annotated")) {
		AT = new at::AnnotatedTracer();
		AT->SetSymbolicHandler(SymbolicHandler);
		int result =  AT->Run(opt);
		return result;
	} else {
		st::SimpleTracer *ST = new st::SimpleTracer();
		return ST->Run(opt);
	}
}
