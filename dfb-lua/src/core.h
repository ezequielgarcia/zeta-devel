
#ifndef _CORE_H_
#define _CORE_H_

#include "directfb.h"


#if defined(__GNUC__) && __GNUC__ >= 4
	#define DLL_EXPORT	__attribute__((visibility("default")))
	#define DLL_LOCAL	__attribute__((visibility("hidden")))
#else
	#define DLL_EXPORT
	#define DLL_LOCAL
#endif

int open_idirectfb (lua_State *L);
int open_idirectfb_surface (lua_State *L);

void push_IDirectFB(lua_State *L, IDirectFB *interface);
void push_IDirectFBSurface(lua_State *L, IDirectFBSurface *interface);

IDirectFB **check_IDirectFB (lua_State *L, int index);
IDirectFBSurface **check_IDirectFBSurface (lua_State *L, int index);

#endif
