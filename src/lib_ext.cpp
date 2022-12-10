#include <comm/str.h>

extern "C"{
    
#define LUA_CORE

#include "lj_state.h"
#include "lj_gc.h"
#include "lj_obj.h"
#include "lj_str.h"
#include "lj_lex.h"
#include "lj_vm.h"
#include "lj_frame.h"
#include "lj_coidtoken.h"

#define api_checknelems(L, n)		api_check(L, (n) <= (L->top - L->base))
#define api_checkvalidindex(L, i)	api_check(L, (i) != niltv(L))

extern TValue *index2adr(lua_State *L, int idx);
extern TValue *cpparser(lua_State *L, lua_CFunction dummy, void *ud);
extern const char *reader_string(lua_State *L, void *ud, size_t *size);
extern cTValue *lj_meta_tget(lua_State *L, cTValue *o, cTValue *k);
extern TValue *lj_meta_tset(lua_State *L, cTValue *o, cTValue *k);


inline int lua_loadx_ext(lua_State *L, lua_Reader reader, void *data,
    const coidtoken& chunkname, const char *mode)
{
    LexState ls;
    int status;
    ls.rfunc = reader;
    ls.rdata = data;
    ls.chunkarg = coid_tok_is_empty(chunkname) ? coid_token_from_cstr("?"):chunkname;
    ls.mode = mode;
    lj_str_initbuf(&ls.sb);
    status = lj_vm_cpcall(L, NULL, &ls, cpparser);
    lj_lex_cleanup(L, &ls);
    lj_gc_check(L);
    return status;
}


inline int lua_load_ext(lua_State *L, lua_Reader reader, void *data,
    const coidtoken& chunkname)
{
    return lua_loadx_ext(L, reader, data, chunkname, NULL);
}

typedef struct StringReaderCtx {
    const char *str;
    size_t size;
} StringReaderCtx;


inline int luaL_loadbufferx_ext(lua_State *L, const char *buf, size_t size,
    const coidtoken& name, const char *mode)
{
    StringReaderCtx ctx;
    ctx.str = buf;
    ctx.size = size;
    return lua_loadx_ext(L, reader_string, &ctx, name, mode);
}

inline int luaL_loadbuffer_ext(lua_State *L, const char *buf, size_t size,
    const coidtoken& name)
{
    return luaL_loadbufferx_ext(L, buf, size, name, NULL);
}

}

#define lua_totoken(L, idx) lua_toltoken(L, idx, NULL)

LUA_API void lua_pushtoken(lua_State*L, const coid::token& t) {
    if (t.is_empty()){
        setnilV(L->top);
    }
    else{
        GCstr *s;
        lj_gc_check(L);
        s = lj_str_new(L, t.ptr(), t.ptre() - t.ptr());
        setstrV(L, L->top, s);
    }
    incr_top(L);
}

LUA_API coid::token lua_toltoken(lua_State *L, int idx, size_t *len)
{
    TValue *o = index2adr(L, idx);
    GCstr *s;
    if (LJ_LIKELY(tvisstr(o))) {
        s = strV(o);
    }
    else if (tvisnumber(o)) {
        lj_gc_check(L);
        o = index2adr(L, idx);  /* GC may move the stack. */
        s = lj_str_fromnumber(L, o);
        setstrV(L, o, s);
    }
    else {
        if (len != NULL) *len = 0;
        return coid::token();
    }
    
    size_t slen = s->len;

    if (len != NULL) *len = slen;
    const char * ptr = strdata(s);

    return coid::token(ptr, ptr + slen);
}

coidtoken token_to_ctoken(const coid::token& tok) {
    coidtoken result;
    result._ptr = tok._ptr;
    result._pte = tok._pte;
    return result;
};

LUA_API int  luaL_loadbuffer(lua_State *L, const char *buf, size_t size,
    const coid::token& name) {
    return luaL_loadbuffer_ext(L, buf, size, token_to_ctoken(name));
}

LUA_API void lua_getfield(lua_State *L, int idx, const coid::token& k)
{
    cTValue *v, *t = index2adr(L, idx);
    TValue key;
    api_checkvalidindex(L, t);
    setstrV(L, &key, lj_str_new(L, k._ptr, k._pte - k._ptr));
    v = lj_meta_tget(L, t, &key);
    if (v == NULL) {
        L->top += 2;
        lj_vm_call(L, L->top - 2, 1 + 1);
        L->top -= 2;
        v = L->top + 1;
    }
    copyTV(L, L->top, v);
    incr_top(L);
}

LUA_API void lua_setfield(lua_State *L, int idx, const coid::token& k)
{
    TValue *o;
    TValue key;
    cTValue *t = index2adr(L, idx);
    api_checknelems(L, 1);
    api_checkvalidindex(L, t);
    setstrV(L, &key, lj_str_new(L, k._ptr, k._pte - k._ptr));
    o = lj_meta_tset(L, t, &key);
    if (o) {
        L->top--;
        /* NOBARRIER: lj_meta_tset ensures the table is not black. */
        copyTV(L, o, L->top);
    }
    else {
        L->top += 3;
        copyTV(L, L->top - 1, L->top - 6);
        lj_vm_call(L, L->top - 3, 0 + 1);
        L->top -= 2;
    }
}

LUA_API bool lua_hasfield(lua_State *L, int idx, const coid::token& k){
	if (!lua_istable(L, idx)){
		return false;
	}

	lua_getfield(L, idx, k);
	if (lua_isnil(L, -1)){
		lua_pop(L, 1);
		return false;
	}
	else{
		lua_pop(L, 1);
		return true;
	}
}

LUA_API void luaL_where_ext(lua_State *L, int level) {
	lua_Debug ar;
	if (lua_getstack(L, level, &ar)) {  /* check function at level */
		lua_getinfo(L, "Sl", &ar);  /* get info about it */
		if (ar.currentline > 0) {  /* is there info? */
			lua_pushstring(L, ar.source);
			lua_pushnumber(L, ar.currentline);
			return;
		}
	}
	lua_pushnil (L);  /* else, no information available... */
}

LUA_API void lua_pushcopy(lua_State *L, int idx) {
    if (lua_isfunction(L, idx)) {
        lua_pushcfunction(L, lua_tocfunction(L, idx));
    }
    else {
        lua_pushnil(L);
    }
}