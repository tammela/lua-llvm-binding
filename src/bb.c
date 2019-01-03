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

#include "bb.h"
#include "core.h"
#include "instruction.h"

#define getbasicblock(L) \
    (*(LLVMBasicBlockRef*)luaL_checkudata(L, 1, LLB_BASICBLOCK));

// ==================================================
//
// instantiates a new basic block object
//
// ==================================================
int bb_new(lua_State* L, LLVMBasicBlockRef bb) {
    newuserdata(L, bb, LLB_BASICBLOCK);
    return 1;
}

// ==================================================
//
// gets a light userdata reference to a basic block
//
// ==================================================
int bb_pointer(lua_State* L) {
    LLVMBasicBlockRef bb = getbasicblock(L);
    lua_pushlightuserdata(L, bb);
    return 1;
}

// ==================================================
//
// gets all successors of a basic block
//
// ==================================================
int bb_successors(lua_State* L) {
    LLVMBasicBlockRef bb = getbasicblock(L);

    LLVMValueRef terminator = LLVMGetBasicBlockTerminator(bb);
    unsigned n_succs = LLVMGetNumSuccessors(terminator);
    lua_newtable(L);
    for (int i = 0; i < n_succs; i++) {
        lua_pushlightuserdata(L, LLVMGetSuccessor(terminator, i));
        lua_seti(L, -2, i + 1);
    }

    return 1;
}

// ==================================================
//
// gets all instructions of a basic block
//
// ==================================================
int bb_instructions(lua_State* L) {
    LLVMBasicBlockRef bb = getbasicblock(L);
    lua_newtable(L);
    int i = 0;
    for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst != NULL;
         inst = LLVMGetNextInstruction(inst)) {
        instruction_new(L, inst);
        lua_seti(L, -2, i + 1);
        i++;
    }
    return 1;
}

// ==================================================
//
// __tostring metamethod
//
// ==================================================
int bb_tostring(lua_State* L) {
    LLVMBasicBlockRef bb = getbasicblock(L);
    lua_pushstring(L, LLVMGetBasicBlockName(bb));
    return 1;
}

// creates an array with all the store instructions within a basic block
int bb_store_instructions(lua_State* L) {
    LLVMBasicBlockRef bb = getbasicblock(L);
    lua_newtable(L);
    LLVMValueRef instruction = LLVMGetFirstInstruction(bb);
    do {
        if (LLVMIsAStoreInst(instruction)) {
            lua_newtable(L);
            instruction_new(L, instruction);
            lua_setfield(L, -2, "reference");
            instruction_new(L, LLVMGetOperand(instruction, 0));
            lua_setfield(L, -2, "value");
            instruction_new(L, LLVMGetOperand(instruction, 1));
            lua_setfield(L, -2, "alloca");
            lua_seti(L, -2, luaL_len(L, -2) + 1);
        }
    } while ((instruction = LLVMGetNextInstruction(instruction)));
    return 1;
}
