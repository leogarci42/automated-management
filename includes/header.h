#pragma once

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "fd_tracker.h"
#include "error.h"
#include "token.h"

void generate_llvm_ir(t_token *ast, const char *outfile);
