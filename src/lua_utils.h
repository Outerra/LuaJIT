#pragma once
#include <luaJIT/lua.hpp>
#include <LuaJIT/luaext.h>
#include <comm/str.h>

class lua_utils
{
private:
    lua_utils();
    ~lua_utils();
public:
    static void print_lua_stack_size(lua_State * L);
    static void print_lua_stack(lua_State * L);
    static void print_lua_stack(lua_State * L, coid::charstr& out);
    static void print_lua_value(lua_State * L, int index, coid::charstr& out, bool table_member = false);
    static void print_lua_table(lua_State * L, int index, coid::charstr& out);
};

