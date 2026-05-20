#include <fcntl.h>
#include "codegen.h"

#define MAX_LINE 128

static bool read_first_line(const char *path, char *out, size_t out_size)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return false;
    if (!fgets(out, (int)out_size, f))
    {
        fclose(f);
        return false;
    }
    fclose(f);
    out[strcspn(out, "\r\n")] = '\0';
    return true;
}

static const char *detect_gpu_vendor(void)
{
    if (access("/proc/driver/nvidia/version", F_OK) == 0)
        return "nvidia";
    char buf[MAX_LINE];
    for (int i = 0; i < 16; i++)
    {
        char path[128];
        snprintf(path, sizeof(path), "/sys/class/drm/card%d/device/vendor", i);
        if (!read_first_line(path, buf, sizeof(buf)))
            continue;
        if (strncmp(buf, "0x10de", 6) == 0)
            return "nvidia";
        if (strncmp(buf, "0x1002", 6) == 0)
            return "amd";
        if (strncmp(buf, "0x8086", 6) == 0)
            return "intel";
    }
    return NULL;
}

static bool check_file_extensions(char *filename)
{
	for (size_t i = 0; filename[i]; i++)
	{
		if (filename[i] == '.' && filename[i + 1] == 'c' && filename[i + 2] == 'u' && filename[i + 3] == 'c' && filename[i + 4] == 'p' && filename[i + 5] == 'p' && !filename[i + 6])
			return (true);
	}
	err->err_str = strdup("invalid filename");
	return (false);
}

static bool parse_line(int fd, t_token **token)
{
        char buff[1025];
        ssize_t r = read(fd, buff, 1024);
        if (r < 0)
        {
                err->err_str = strdup("read error");
                return (false);
        }
        else
        {
                buff[r] = '\0';
                err->valid = generate_token(buff, token);
        }
		return (err->valid);
}

static bool parse_file(char *filename, t_token **token)
{
	int fd = open(filename, O_RDONLY);
	if (fd < 0)
	{
		err->err_str = strdup("Failed to open file");
		return (false);
	}
	else
		err->valid = parse_line(fd, token);
    close(fd);
	return (err->valid);
}

static bool has_exec_target(t_token *token, t_exec_target target)
{
    if (!token)
        return false;
    if (token->type == compute && token->exec_target == target)
        return true;
    if (token->body && has_exec_target(token->body, target))
        return true;
    if (token->next && has_exec_target(token->next, target))
        return true;
    return false;
}

static const char *detect_target_triple(bool prefer_gpu)
{
    const char *env = getenv("CUCPP_TARGET");
    if (env)
    {
        if (strcmp(env, "cuda") == 0)
            return "nvptx64-nvidia-cuda";
        if (strcmp(env, "arm") == 0)
            return "aarch64-unknown-linux-gnu";
        if (strcmp(env, "intel") == 0)
            return "x86_64-pc-linux-gnu";
    }
#if defined(__aarch64__)
    const char *host = "aarch64-unknown-linux-gnu";
#else
    const char *host = "x86_64-pc-linux-gnu";
#endif
    if (prefer_gpu)
        return "nvptx64-nvidia-cuda";
    return host;
}

static const char *detect_gpu_triple(void)
{
    const char *env = getenv("CUCPP_TARGET");
    if (env)
    {
        if (strcmp(env, "cuda") == 0)
            return "nvptx64-nvidia-cuda";
        if (strcmp(env, "amd") == 0)
            return "amdgcn-amd-amdhsa";
        if (strcmp(env, "intel") == 0)
            return "spir64-unknown-unknown";
    }
    const char *vendor = detect_gpu_vendor();
    if (!vendor)
        return NULL;
    if (strcmp(vendor, "nvidia") == 0)
        return "nvptx64-nvidia-cuda";
    if (strcmp(vendor, "amd") == 0)
        return "amdgcn-amd-amdhsa";
    if (strcmp(vendor, "intel") == 0)
        return "spir64-unknown-unknown";
    return NULL;
}

static bool llc_has_target(const char *llc_cmd, const char *needle)
{
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "%s --version 2>/dev/null", llc_cmd);
    FILE *fp = popen(cmd, "r");
    if (!fp)
        return false;
    char line[256];
    bool found = false;
    while (fgets(line, sizeof(line), fp))
    {
        if (strstr(line, needle))
        {
            found = true;
            break;
        }
    }
    pclose(fp);
    return found;
}

static const char *pick_linker_cmd(bool *is_ld)
{
    const char *env = getenv("CUCPP_LLD");
    if (env && env[0] != '\0')
    {
        *is_ld = (strstr(env, "ld.lld") == NULL);
        return env;
    }
    if (access("/usr/bin/ld.lld-12", X_OK) == 0 || access("/bin/ld.lld-12", X_OK) == 0)
    {
        *is_ld = false;
        return "ld.lld-12";
    }
    if (access("/usr/bin/ld.lld", X_OK) == 0 || access("/bin/ld.lld", X_OK) == 0)
    {
        *is_ld = false;
        return "ld.lld";
    }
    if (access("/usr/bin/ld", X_OK) == 0 || access("/bin/ld", X_OK) == 0)
    {
        *is_ld = true;
        return "ld";
    }
    *is_ld = false;
    return NULL;
}

typedef struct s_symbol {
    char *name;
    t_var_type type;
    struct s_symbol *next;
} t_symbol;

static void free_symbol_table(t_symbol *table) {
    while (table) {
        t_symbol *next = table->next;
        free(table->name);
        free(table);
        table = next;
    }
}

static t_symbol *find_symbol(t_symbol *table, char *name) {
    while (table) {
        if (strcmp(table->name, name) == 0) return table;
        table = table->next;
    }
    return NULL;
}

static void add_symbol(t_symbol **table, char *name, t_var_type type) {
    t_symbol *sym = malloc(sizeof(t_symbol));
    sym->name = strdup(name);
    sym->type = type;
    sym->next = *table;
    *table = sym;
}

static bool validate_types(t_token *token, t_symbol **table) {
    if (!token) return true;
    
    if (token->type == assignment) {
        t_symbol *existing = find_symbol(*table, token->name);
        if (existing) {
            if (existing->type != token->var_type && token->var_type != TYPE_VAR && existing->type != TYPE_VAR) {
                if (err->err_str) free(err->err_str);
                char buf[256];
                snprintf(buf, sizeof(buf), "Error: Cannot reassign variable '%s' to a different type", token->name);
                err->err_str = strdup(buf);
                err->valid = false;
                return false;
            }
            if (existing->type == TYPE_VAR && token->var_type != TYPE_VAR)
                existing->type = token->var_type;
        } else {
            add_symbol(table, token->name, token->var_type);
        }
    }
    
    if (token->body && !validate_types(token->body, table)) return false;
    if (token->next && !validate_types(token->next, table)) return false;
    return true;
}

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
        if (!validate_types(token, &table)) {
            error_printer(err);
        } else {
            bool has_cpu = has_exec_target(token, EXEC_CPU);
            bool has_gpu = has_exec_target(token, EXEC_GPU);
            if (has_cpu)
            {
                generate_llvm_ir_cpu(token, "output.ll");
				
                char cmd[512];
                snprintf(cmd, sizeof(cmd), "clang -g -Wno-override-module output.ll -o %s", out_bin);
                int ret = system(cmd);
                if (ret == 0) {
                    printf("[SUCCESS] Generated binary '%s'. Try running ./%s\n", out_bin, out_bin);
                    if (!emit_llvm) {
                        remove("output.ll");
                    }
                } else {
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
                    llc_cmd = "llc-12";
                bool is_ld = false;
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
                        if (!emit_llvm) {
                            remove("output_gpu.ll");
                        }
                        return;
                    }
                }
                if (strstr(triple, "nvptx") != NULL)
                {
                    snprintf(cmd, sizeof(cmd), "%s -march=nvptx64 -filetype=asm -mtriple=%s output_gpu.ll -o output_gpu.ptx", llc_cmd, triple);
                    int ret = system(cmd);
                    if (ret == 0) {
                        printf("[SUCCESS] Generated GPU PTX 'output_gpu.ptx' (triple: %s)\n", triple);
                        if (!emit_llvm) {
                            remove("output_gpu.ll");
                        }
                    } else {
                        printf("\n[ERROR] Failed to compile GPU LLVM IR with llc.\n");
                        printf("[HINT] Run with --emit-llvm and check the generated 'output_gpu.ll' file.\n");
                    }
                }
                else if (strstr(triple, "amdgcn") != NULL)
                {
                    snprintf(cmd, sizeof(cmd), "%s -march=amdgcn -filetype=obj -mtriple=%s output_gpu.ll -o output_gpu.tmp.o", llc_cmd, triple);
                    int ret = system(cmd);
                    if (ret == 0) {
                        snprintf(cmd, sizeof(cmd), "%s -r -o output_gpu.o output_gpu.tmp.o", lld_cmd);
                        ret = system(cmd);
                        if (ret == 0) {
                            printf("[SUCCESS] Generated GPU object 'output_gpu.o' (triple: %s)\n", triple);
                            remove("output_gpu.tmp.o");
                            if (!emit_llvm) {
                                remove("output_gpu.ll");
                            }
                        } else {
                            printf("\n[ERROR] Failed to link GPU object with lld.\n");
                            printf("[HINT] Run with --emit-llvm and check the generated 'output_gpu.ll' file.\n");
                        }
                    } else {
                        printf("\n[ERROR] Failed to compile GPU LLVM IR with llc.\n");
                        printf("[HINT] Run with --emit-llvm and check the generated 'output_gpu.ll' file.\n");
                    }
                }
                else
                {
                    snprintf(cmd, sizeof(cmd), "%s -filetype=obj -mtriple=%s output_gpu.ll -o output_gpu.tmp.o", llc_cmd, triple);
                    int ret = system(cmd);
                    if (ret == 0) {
                        rename("output_gpu.tmp.o", "output_gpu.o");
                        printf("[WARN] GPU linker not available. Kept GPU object as 'output_gpu.o' without linking.\n");
                        if (!emit_llvm) {
                            remove("output_gpu.ll");
                        }
                    } else {
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

