
#pragma once



#include <assert.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <variant>
#include <functional>
#include <optional>
#include <exception>


#include "source/location.h"
#include "source/source-buffer.h"
#include "source/token.h"
#include "source/tokenize.h"
#include "compilation/run-time/value.h"
#include "compilation/run-time/dictionary.h"
#include "compilation/run-time/compiler-runtime.h"
#include "compilation/byte-code/instruction.h"
#include "compilation/byte-code/script.h"
#include "compilation/byte-code/jit.h"
#include "compilation/byte-code/context.h"
#include "compilation/compiler.h"
#include "error.h"
