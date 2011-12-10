#include "lua.h"
#include "lauxlib.h"
#include "core.h"
#include "directfb.h"

DLL_LOCAL void push_IDirectFB(lua_State *L, IDirectFB *interface)
{
	IDirectFB **p = lua_newuserdata(L, sizeof(IDirectFB*));
	*p = interface;
	luaL_getmetatable( L, "IDirectFB" );
	lua_setmetatable( L, -2 );
}

DLL_LOCAL IDirectFB **check_IDirectFB (lua_State *L, int index)
{
	IDirectFB **p;
	luaL_checktype(L, index, LUA_TUSERDATA);
	p = (IDirectFB **)luaL_checkudata(L, index, "IDirectFB");
	if (p == NULL) luaL_typerror(L, index, "IDirectFB");
	return p;
}

DLL_LOCAL void push_IDirectFBSurface(lua_State *L, IDirectFBSurface *interface)
{
	IDirectFBSurface **p = lua_newuserdata(L, sizeof(IDirectFBSurface*));
	*p = interface;
	luaL_getmetatable( L, "IDirectFBSurface" );
	lua_setmetatable( L, -2 );
}

DLL_LOCAL IDirectFBSurface **check_IDirectFBSurface (lua_State *L, int index)
{
	IDirectFBSurface **p;
	luaL_checktype(L, index, LUA_TUSERDATA);
	p = (IDirectFBSurface **)luaL_checkudata(L, index, "IDirectFBSurface");
	if (p == NULL) luaL_typerror(L, index, "IDirectFBSurface");
	return p;
}

static int l_DirectFBInit (lua_State *L)
{
	DirectFBInit(NULL, NULL);

	return 0;
}

static int l_DirectFBCreate (lua_State *L)
{
	IDirectFB *interface;

	DirectFBCreate(&interface);

	push_IDirectFB(L, interface);

	return 1;  /* new userdatum is already on the stack */
}

static const luaL_reg dfb_m[] = {
	{"DirectFBCreate", l_DirectFBCreate},
	{"DirectFBInit", l_DirectFBInit},
	{NULL, NULL}
};

int LUALIB_API luaopen_directfb (lua_State *L)
{
	open_idirectfb(L);

	open_idirectfb_surface(L);

	/* create methods table,
	   add it to the globals */
	luaL_openlib(L, "directfb", dfb_m, 0); 	

	return 1;
}

