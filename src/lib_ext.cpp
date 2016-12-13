#include <comm/str.h>

extern "C"{

#include "lj_state.h"
#include "lj_gc.h"
#include "lj_obj.h"
#include "lj_str.h"
#include "lj_lex.h"
#include "lj_vm.h"
#include "lj_frame.h"
#include "lj_coidtoken.h"

extern TValue *index2adr(lua_State *L, int idx);
extern TValue *cpparser(lua_State *L, lua_CFunction dummy, void *ud);
extern const char *reader_string(lua_State *L, void *ud, size_t *size);

int lua_loadx_ext(lua_State *L, lua_Reader reader, void *data,
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


int lua_load_ext(lua_State *L, lua_Reader reader, void *data,
    const coidtoken& chunkname)
{
    return lua_loadx_ext(L, reader, data, chunkname, NULL);
}

typedef struct StringReaderCtx {
    const char *str;
    size_t size;
} StringReaderCtx;


int luaL_loadbufferx_ext(lua_State *L, const char *buf, size_t size,
    const coidtoken& name, const char *mode)
{
    StringReaderCtx ctx;
    ctx.str = buf;
    ctx.size = size;
    return lua_loadx_ext(L, reader_string, &ctx, name, mode);
}

int luaL_loadbuffer_ext(lua_State *L, const char *buf, size_t size,
    const coidtoken& name)
{
    return luaL_loadbufferx_ext(L, buf, size, name, NULL);
}

}

#define lua_totoken(L, idx) lua_toltoken(L, idx, NULL)

void lua_pushtoken(lua_State*L, const coid::token& t) {
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

coid::token lua_toltoken(lua_State *L, int idx, size_t *len)
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

int  luaL_loadbuffer(lua_State *L, const char *buf, size_t size,
    const coid::token& name) {
    return luaL_loadbuffer_ext(L, buf, size, token_to_ctoken(name));
}