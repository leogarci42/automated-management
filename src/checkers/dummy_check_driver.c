#include "dummy_check_internal.h"

void dummy_parse(char *filename, bool emit_llvm, char *out_bin)
{
	err->valid = check_file_extensions(filename);
	error_printer(err);

	t_token *token = NULL;
	err->valid = parse_file(filename, &token);
	if (!err->valid)
		error_printer(err);
	else if (token)
	{
		t_symbol *table = NULL;
		if (!validate_types(token, &table))
		{
			error_printer(err);
		}
		else
		{
			bool has_cpu = has_exec_target(token, EXEC_CPU);
			bool has_gpu = has_exec_target(token, EXEC_GPU);
			if (has_cpu)
			{
				generate_llvm_ir_cpu(token, "output.ll");

				char cmd[512];
				snprintf(cmd, sizeof(cmd), "clang -g -Wno-unused-command-line-argument -Wno-override-module output.ll -o %s", out_bin);
				int ret = system(cmd);
				if (ret == 0)
				{
					printf("[SUCCESS] Generated binary '%s'. Try running ./%s\n", out_bin, out_bin);
					if (!emit_llvm)
						remove("output.ll");
				}
				else
				{
					printf("\n[ERROR] Failed to compile LLVM IR to binary.\n");
					printf("[HINT] Run with --emit-llvm and check the generated 'output.ll' file.\n");
				}
			}
			if (has_gpu)
			{
				const char *triple = detect_gpu_triple();
				if (!triple)
				{
					printf("[WARN] No GPU detected. Set CUCPP_TARGET=cuda|amd|intel to force a backend.\n");
					triple = detect_target_triple(true);
				}
				generate_llvm_ir_gpu(token, "output_gpu.ll", triple);
				const char *llc_cmd = getenv("CUCPP_LLC");
				if (!llc_cmd || llc_cmd[0] == '\0')
					llc_cmd = "llc";
				bool is_ld = false;
				const char *lld_cmd = pick_linker_cmd(&is_ld);
				char cmd[512];
				if (strstr(triple, "spir64") != NULL)
				{
					if (!llc_has_target(llc_cmd, "spirv") && llc_has_target(llc_cmd, "amdgcn"))
					{
						printf("[WARN] SPIR-V backend not available in %s. Falling back to AMD backend.\n", llc_cmd);
						triple = "amdgcn-amd-amdhsa";
					}
					else if (!llc_has_target(llc_cmd, "spirv"))
					{
						printf("[WARN] SPIR-V backend not available in %s. Install a newer LLVM or set CUCPP_TARGET=cuda|amd.\n", llc_cmd);
						if (!emit_llvm)
							remove("output_gpu.ll");
						return;
					}
				}
				if (strstr(triple, "nvptx") != NULL)
				{
					snprintf(cmd, sizeof(cmd), "%s -march=nvptx64 -filetype=asm -mtriple=%s output_gpu.ll -o output_gpu.ptx", llc_cmd, triple);
					int ret = system(cmd);
					if (ret == 0)
					{
						printf("[SUCCESS] Generated GPU PTX 'output_gpu.ptx' (triple: %s)\n", triple);
						if (!emit_llvm)
							remove("output_gpu.ll");
					}
					else
					{
						printf("\n[ERROR] Failed to compile GPU LLVM IR with llc.\n");
						printf("[HINT] Run with --emit-llvm and check the generated 'output_gpu.ll' file.\n");
					}
				}
				else if (strstr(triple, "amdgcn") != NULL)
				{
					snprintf(cmd, sizeof(cmd), "%s -march=amdgcn -filetype=obj -mtriple=%s output_gpu.ll -o output_gpu.tmp.o", llc_cmd, triple);
					int ret = system(cmd);
					if (ret == 0)
					{
						if (!lld_cmd)
						{
							rename("output_gpu.tmp.o", "output_gpu.o");
							printf("[WARN] GPU linker not available. Kept GPU object as 'output_gpu.o' without linking.\n");
							if (!emit_llvm)
								remove("output_gpu.ll");
						}
						else
						{
							snprintf(cmd, sizeof(cmd), "%s -r -o output_gpu.o output_gpu.tmp.o", lld_cmd);
							ret = system(cmd);
							if (ret == 0)
							{
								printf("[SUCCESS] Generated GPU object 'output_gpu.o' (triple: %s)\n", triple);
								remove("output_gpu.tmp.o");
								if (!emit_llvm)
									remove("output_gpu.ll");
							}
							else
							{
								printf("\n[ERROR] Failed to link GPU object with lld.\n");
								printf("[HINT] Run with --emit-llvm and check the generated 'output_gpu.ll' file.\n");
							}
						}
					}
					else
					{
						printf("\n[ERROR] Failed to compile GPU LLVM IR with llc.\n");
						printf("[HINT] Run with --emit-llvm and check the generated 'output_gpu.ll' file.\n");
					}
				}
				else
				{
					snprintf(cmd, sizeof(cmd), "%s -filetype=obj -mtriple=%s output_gpu.ll -o output_gpu.tmp.o", llc_cmd, triple);
					int ret = system(cmd);
					if (ret == 0)
					{
						rename("output_gpu.tmp.o", "output_gpu.o");
						printf("[WARN] GPU linker not available. Kept GPU object as 'output_gpu.o' without linking.\n");
						if (!emit_llvm)
							remove("output_gpu.ll");
					}
					else
					{
						printf("\n[ERROR] Failed to compile GPU LLVM IR with llc.\n");
						printf("[HINT] Run with --emit-llvm and check the generated 'output_gpu.ll' file.\n");
					}
				}
			}
		}
		free_symbol_table(table);
	}
	free_token(token);
}
