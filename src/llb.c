/*
 * Lua binding for LLVM C API.
 * Copyright (C) 2018 Matheus Ambrozio, Pedro Tammela, Renan Almeida.
 *
 * This file is part of lua-llvm-binding.
 *
 * lua-llvm-binding is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * lua-llvm-binding is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with lua-llvm-binding. If not, see <http://www.gnu.org/licenses/>.
 */

#include <lauxlib.h>
#include <lua.h>

#include <llvm-c/BitReader.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>

#include "llb.h"

// static void newclass(lua_State *L, const char *tname, const luaL_Reg *funcs) {
//     luaL_newmetatable(L, tname);
//     lua_pushvalue(L, -1);
//     lua_setfield(L, -2, "__index");
//     luaL_setfuncs(L, funcs, 0);
//     lua_pop(L, 1);
// }

// ==================================================
//
//  Module
//
// ==================================================

static int llb_load_ir(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    char *err;

    // FIXME: dispose of ctx after creating the module?
    // creating the context
    LLVMContextRef ctx = LLVMContextCreate();

    // creating the memory buffer
    LLVMMemoryBufferRef memory_buffer;
    if (LLVMCreateMemoryBufferWithContentsOfFile(path, &memory_buffer, &err)) {
        lua_pushnil(L);
        lua_pushfstring(L, "[LLVM] %s", err);
        return 2;
    }

    // creating the module
    LLVMModuleRef module;
    if (LLVMParseIRInContext(ctx, memory_buffer, &module, &err)) {
        // FIXME: this is causing an error
        // LLVMDisposeMemoryBuffer(memory_buffer);
        lua_pushnil(L);
        lua_pushfstring(L, "[LLVM] %s", err);
        return 2;
    }
    // FIXME: this is causing an error
    // LLVMDisposeMemoryBuffer(memory_buffer);

    // creating the user data for the module
    LLVMModuleRef *lua_module = lua_newuserdata(L, sizeof(LLVMModuleRef));
    *lua_module = module;
    luaL_setmetatable(L, LLB_MODULE);

    return 1;
}

static int llb_load_bitcode(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    char *err;

    // reading the file from path
    LLVMMemoryBufferRef memory_buffer;
    if (LLVMCreateMemoryBufferWithContentsOfFile(path, &memory_buffer, &err)) {
        lua_pushnil(L);
        lua_pushfstring(L, "[LLVM] %s", err);
        return 2;
    }

    // creating the module
    LLVMModuleRef module;
    if (LLVMParseBitcode(memory_buffer, &module, &err)) {
        LLVMDisposeMemoryBuffer(memory_buffer);
        lua_pushnil(L);
        lua_pushfstring(L, "[LLVM] %s", err);
        return 2;
    }
    LLVMDisposeMemoryBuffer(memory_buffer);

    // creating the user data for the module
    LLVMModuleRef *lua_module = lua_newuserdata(L, sizeof(LLVMModuleRef));
    *lua_module = module;
    luaL_setmetatable(L, LLB_MODULE);

    return 1;
}

static int llb_write_bitcode(lua_State* L) {
    // FIXME: not checking if the argument is of the correct type
    LLVMModuleRef module = *(LLVMModuleRef*)lua_touserdata(L, 1);
    const char *path = luaL_checkstring(L, 2);

    // writing the module to the output file
    if (LLVMWriteBitcodeToFile(module, path)) {
        lua_pushnil(L);
        lua_pushfstring(L, "[LLVM] could write bitcode to the output file");
        return 2;
    }

    LLVMDisposeModule(module);
    return 0;
}

static int llb_load_functions(lua_State* L) {
    // get module from Lua stack
    LLVMModuleRef module = *(LLVMModuleRef*)lua_touserdata(L, 1);

    // create the functions table
    lua_newtable(L);
    for (LLVMValueRef function = LLVMGetFirstFunction(module);
            function != NULL;
            function = LLVMGetNextFunction(function)) {
        unsigned bbs_count = LLVMCountBasicBlocks(function);

        // skip functions without basic blocks
        if (bbs_count == 0) {
            continue;
        }

        // get function name to use as key
        const char* f_name = LLVMGetValueName(function);

        lua_newtable(L);
        for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function);
                bb != NULL;
                bb = LLVMGetNextBasicBlock(bb)) {
            const char* bb_name = LLVMGetValueName(LLVMBasicBlockAsValue(bb));
            LLVMBasicBlockRef *lua_bb = lua_newuserdata(L, sizeof(LLVMBasicBlockRef));
            *lua_bb = bb;
            lua_setfield(L, -2, bb_name);
        }
        lua_setfield(L, -2, f_name);
    }

    return 1;
}


// ==================================================
//
//  luaopen
//
// ==================================================

int luaopen_llb(lua_State *L) {
    // module
    // TODO

    // basic_block
    // TODO

    // const luaL_Reg lib_llvm[] = {
    //     {"Core", coreobj},
    //     {NULL, NULL}
    // };
    // newclass(L, LUALLVM_CORE, lib_core);

    // core
    const luaL_Reg lib_llb[] = {
        {"load_ir", llb_load_ir},
        {"load_bitcode", llb_load_bitcode},
        {"write_bitcode", llb_write_bitcode},
        {"load_functions", llb_load_functions},
        {NULL, NULL}
    };

    luaL_newlib(L, lib_llb);
    return 1;
}