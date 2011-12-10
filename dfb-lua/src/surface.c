
#include "lua.h"
#include "lauxlib.h"
#include "core.h"
#include "directfb.h"

static int l_IDirectFBSurface_Blit (lua_State *L)
{
	IDirectFBSurface **p = check_IDirectFBSurface(L, 1);

	return 0;
}

static int l_IDirectFBSurface_Flip (lua_State *L)
{
	IDirectFBSurface **p = check_IDirectFBSurface(L, 1);

	(*p)->Flip(*p, NULL, 0);

	return 0;
}

static int l_IDirectFBSurface_Clear (lua_State *L)
{
	IDirectFBSurface **p = check_IDirectFBSurface(L, 1);
	int r = luaL_checkinteger(L, 2);
	int g = luaL_checkinteger(L, 3);
	int b = luaL_checkinteger(L, 4);
	int a = luaL_checkinteger(L, 5);

	(*p)->Clear(*p, r, g, b, a);

	return 0;
}

static const luaL_reg idirectfb_surface_methods[] = {
	{"Flip", l_IDirectFBSurface_Flip},
	{"Clear", l_IDirectFBSurface_Clear},
	{NULL, NULL}
};

DLL_LOCAL int open_idirectfb_surface (lua_State *L)
{
	luaL_newmetatable(L, "IDirectFBSurface");

	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);  /* pushes the metatable */
	lua_settable(L, -3);  /* metatable.__index = metatable */

	luaL_openlib(L, NULL, idirectfb_surface_methods, 0);

	return 1;
}

