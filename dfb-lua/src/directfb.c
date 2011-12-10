#include "lua.h"
#include "lauxlib.h"
#include "core.h"
#include "directfb.h"

static int l_IDirectFB_SetCooperativeLevel (lua_State *L)
{
	IDirectFB **p = check_IDirectFB(L, 1);

	(*p)->SetCooperativeLevel(*p, DFSCL_FULLSCREEN);

	return 0;
}

static int l_IDirectFB_CreateSurface (lua_State *L)
{
	IDirectFB **p = check_IDirectFB(L, 1);
	IDirectFBSurface *s;
	DFBSurfaceDescription desc;

	desc.flags = DSDESC_CAPS;
	desc.caps  = DSCAPS_PRIMARY | DSCAPS_FLIPPING;

	(*p)->CreateSurface(*p, &desc, &s);

	push_IDirectFBSurface(L, s);

	return 1;
}

static const luaL_reg idirectfb_methods[] = {
	{"SetCooperativeLevel", l_IDirectFB_SetCooperativeLevel},
	{"CreateSurface", l_IDirectFB_CreateSurface},
	{NULL, NULL}
};

DLL_LOCAL int open_idirectfb (lua_State *L)
{
	luaL_newmetatable(L, "IDirectFB");

	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);  /* pushes the metatable */
	lua_settable(L, -3);  /* metatable.__index = metatable */

	luaL_openlib(L, NULL, idirectfb_methods, 0);

	return 1;
}

