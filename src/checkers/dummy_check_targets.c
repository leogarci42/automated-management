#include "dummy_check_internal.h"

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
	char buf[128];
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

const char *detect_target_triple(bool prefer_gpu)
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

const char *detect_gpu_triple(void)
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

bool llc_has_target(const char *llc_cmd, const char *needle)
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

const char *pick_linker_cmd(bool *is_ld)
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
