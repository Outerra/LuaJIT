#ifndef luaext_h
#define luaext_h

#include <comm/str.h>

#define lua_totoken(L,i)	lua_toltoken(L, (i), NULL)
LUA_API void  (lua_pushtoken)(lua_State *L, const coid::token& s);
LUA_API coid::token (lua_toltoken)(lua_State *L, int idx, size_t *len);
LUA_API int   (lua_load)(lua_State *L, lua_Reader reader, void *dt,
    const coid::token& name);
LUALIB_API int (luaL_loadbuffer)(lua_State *L, const char *buff, size_t sz,
    const coid::token& name);

#endif 