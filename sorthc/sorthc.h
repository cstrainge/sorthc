
#pragma once



#if   defined (__WIN32)  \
   || defined (_WIN32)   \
   || defined (WIN32)    \
   || defined (_WIN64)   \
   || defined (__WIN64)  \
   || defined (WIN64)    \
   || defined (__WINNT)

    #define IS_WINDOWS 1

#endif

#if    defined(unix)      \
    || defined(__unix__)  \
    || defined(__unix)    \
    || defined(__APPLE__)

    #define IS_UNIX 1

    #if defined(__APPLE__)

        #define IS_MACOS 1

    #endif

#endif



#include <assert.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <stack>
#include <tuple>
#include <filesystem>
#include <variant>
#include <functional>
#include <optional>
#include <exception>


#include "error.h"
#include "source/location.h"
#include "source/source-buffer.h"
#include "source/token.h"
#include "source/tokenize.h"
#include "compilation/word.h"
#include "compilation/byte-code/script.h"
#include "compilation/run-time/value.h"
#include "compilation/byte-code/instruction.h"
#include "compilation/byte-code/construction.h"
#include "compilation/byte-code/context.h"
#include "compilation/run-time/dictionary.h"
#include "compilation/run-time/contextual-list.h"
#include "compilation/run-time/compiler-runtime.h"
#include "compilation/run-time/built-in-words/built-in-words.h"
#include "compilation/byte-code/jit.h"
#include "compilation/compiler.h"
