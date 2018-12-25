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

#include <llvm-c/Core.h>

#include "builder.h"
#include "core.h"
#include "function.h"

#define getmodule(L) (*(LLVMModuleRef*)luaL_checkudata(L, 1, LLB_MODULE))

static int internal_modgc(lua_State* L) {
    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        LLVMDisposeModule(*(LLVMModuleRef*)lua_touserdata(L, -2));
        lua_pop(L, 1);
    }
    return 0;
}

static void build_modstable(lua_State* L) {
    struct luaL_Reg mt[] = {{"__gc", internal_modgc}, {NULL, NULL}};
    lua_newtable(L);
    lua_newtable(L);
    luaL_setfuncs(L, mt, 0);
    lua_setmetatable(L, -2);
}

int module_new(lua_State* L, LLVMModuleRef module) {
    newuserdata(L, module, LLB_MODULE);
    if (lua_getfield(L, LUA_REGISTRYINDEX, "internal_modules") == LUA_TNIL) {
        lua_pop(L, 1);
        build_modstable(L);
        lua_pushvalue(L, -2);
        lua_pushboolean(L, 1);
        lua_settable(L, -3);
        lua_setfield(L, LUA_REGISTRYINDEX, "internal_modules");
    } else {
        lua_pushvalue(L, -2);
        lua_pushboolean(L, 1);
        lua_settable(L, -3);
        lua_pop(L, 1);
    }
    return 1;
}

int module_dispose(lua_State* L) {
    LLVMModuleRef module = getmodule(L);
    LLVMDisposeModule(module);
    return 0;
}

static int module_iterator(lua_State* L) {
    LLVMModuleRef module = getmodule(L);
    if (lua_isnil(L, 2)) {
        LLVMValueRef f = LLVMGetFirstFunction(module);
        const char* fname = LLVMGetValueName(f);
        lua_pushstring(L, fname);
        function_new(L, f);
    } else {
        const char* key = luaL_checkstring(L, 2);
        LLVMValueRef f = LLVMGetNamedFunction(module, key);
        LLVMValueRef fnext = LLVMGetNextFunction(f);
        if (fnext == NULL) {
            lua_pushnil(L);
            return 1;
        }
        const char* fnextname = LLVMGetValueName(fnext);
        lua_pushstring(L, fnextname);
        function_new(L, fnext);
    }
    return 2;
}

int module_pairs(lua_State* L) {
    lua_pushcfunction(L, module_iterator);
    lua_pushvalue(L, 1);
    lua_pushnil(L);
    return 3;
}

int module_index(lua_State* L) {
    LLVMModuleRef module = getmodule(L);
    const char* key = luaL_checkstring(L, 2);
    LLVMValueRef f = LLVMGetNamedFunction(module, key);
    if (f == NULL) {
        lua_pushnil(L);
    } else {
        function_new(L, f);
    }
    return 1;
}

int module_get_builder(lua_State* L) {
    LLVMModuleRef module = getmodule(L);
    LLVMContextRef context = LLVMGetModuleContext(module);
    LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);
    builder_new(L, builder);
    return 1;
}

int module_tostring(lua_State* L) {
    LLVMModuleRef module = getmodule(L);
    size_t size;
    lua_pushstring(L, LLVMGetModuleIdentifier(module, &size));
    return 1;
}
