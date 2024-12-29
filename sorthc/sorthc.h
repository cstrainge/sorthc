
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


#include "sorth-runtime.h"


#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCDisassembler/MCDisassembler.h>
#include <llvm/MC/MCInst.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/MC/MCInstPrinter.h>
#include <llvm/MC/MCRegisterInfo.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/MC/MCSubtargetInfo.h>
#include <llvm/Object/SymbolicFile.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Utils.h>


#include "error.h"
#include "source/location.h"
#include "source/source-buffer.h"
#include "source/token.h"
#include "source/tokenize.h"
#include "compilation/word.h"
#include "compilation/byte-code/structure-type.h"
#include "compilation/byte-code/script.h"
#include "compilation/run-time/value.h"
#include "compilation/run-time/array.h"
#include "compilation/byte-code/instruction.h"
#include "compilation/byte-code/construction.h"
#include "compilation/byte-code/context.h"
#include "compilation/run-time/dictionary.h"
#include "compilation/run-time/contextual-list.h"
#include "compilation/run-time/compiler-runtime.h"
#include "compilation/run-time/built-in-words/built-in-words.h"
#include "compilation/byte-code/jit.h"
#include "compilation/ir_generator.h"
#include "compilation/compiler.h"
