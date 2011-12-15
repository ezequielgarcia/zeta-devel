/*
 Copyright (c) 2011 Gabriel Duarte <gabrield@impa.br>
 Copyright (c) 2011 Ezequiel Garcia <elezegarcia@yahoo.com.ar>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
/* Lua headers */
#include <lua.h>
#include <lauxlib.h>
#include <luaconf.h>
#include <lualib.h>
/* V4L2 */
#include "core.h"

static BYTE* _img = NULL; 
static int _imagesize = 0;

/* TODO LIST:
 *
 * - I dont like to return the file descriptor to the user; isn't this a security breach?
 * - I dont like to pass the whole image through Lua stack. Maybe I am just paranoid?
 *
 */

static int l_width(lua_State *L)
{
    lua_pushnumber(L, get_width());
    return 1;
}

static int l_height(lua_State *L)
{
    lua_pushnumber(L, get_height());
    return 1;
}
                    
static int l_open(lua_State *L)
{
    int fd = -1;
    const char *device;
    
    if (!lua_gettop(L))
        return luaL_error(L, "device name missing");

    device = luaL_checkstring(L, 1);

    fd = open_device(device);
	if (fd < 0) 
        return luaL_error(L, "device error");

	/* start capturing */
	init_device();
	start_capturing();

	/* we return the file descriptor; lil' security hole?*/
	lua_pushinteger(L, fd);

    _imagesize = get_imagesize();

    return 1;
}

static int l_close(lua_State *L)
{
    int fd = -1;
    int dev = 0;

    if (!lua_gettop(L))
        return luaL_error(L, "set a device");

    fd = luaL_checkint(L, 1);
    
	stop_capturing();
    uninit_device();
    dev = close_device(fd);

	/* We return the closed device */
    lua_pushinteger(L, dev);

    return 1;
}

static int l_getframe(lua_State *L)
{
    size_t i;

    _img = newframe();

    lua_createtable(L, _imagesize, 0);

    for(i = 0; i < _imagesize; ++i) {
        lua_pushnumber(L, _img[i]);
        lua_rawseti(L, -2, i+1);
    }
    
    return 1;
}

static int l_liststandards(lua_State* L)
{
	size_t i,count;
	char** arr_stds;

	/* get controls count */
	count =	list_standards(NULL);

	arr_stds = (char**)malloc(count * sizeof(char*));

	/* get controls, callee manages memory */
	list_standards(&arr_stds);

	/* return a table with keys as control names, value = 1*/
	lua_newtable(L);	

	for (i=0; i<count; i++) {
	
		lua_pushstring(L, arr_stds[i]);
		lua_pushboolean(L,1);
		lua_settable(L,-3);
	}

	/* free array */
	free(arr_stds);

	return 1;
}

static int l_listcontrols(lua_State* L)
{
	size_t i,count;
	char** arr_ctrls;

	/* get controls count */
	count =	list_controls(NULL);

	arr_ctrls = (char**)malloc(count * sizeof(char*));

	/* get controls, callee manages memory */
	list_controls(&arr_ctrls);

	/* return a table with keys as control names, value = 1*/
	lua_newtable(L);	

	for (i=0; i<count; i++) {
	
		lua_pushstring(L, arr_ctrls[i]);
		lua_pushboolean(L,1);
		lua_settable(L,-3);
	}

	/* free array */
	free(arr_ctrls);

	return 1;
}

static int l_standard(lua_State *L)
{
	char* name;

	if ( !lua_gettop(L) ) {

		/* getter */

		/* if get_standard returns 0, then callee
		 * allocates memory for the name and we
		 * must free it here...
		 */
		if (!get_standard(&name)) 
		    	lua_pushstring(L, name);
		else
			lua_pushnil(L);

		/* free standard name */
		free(name);

		return 1;
	}

	/* setter */
	name = (char*)luaL_checkstring(L, 1);
	set_standard(name);
    	
	return 0;
}

static int l_control(lua_State *L)
{
	const char* name;
	int value;

	if ( !lua_gettop(L) ) 
        	return luaL_error(L, "control name missing");
    
	if ( lua_gettop(L) == 1 ) {

		/* getter */
		name = luaL_checkstring(L, 1);
		if (!get_control(name, &value)) 
		    	lua_pushnumber(L, value);
		else
			lua_pushnil(L);

		return 1;
	}

	/* setter */
	name = luaL_checkstring(L, 1);
	value = luaL_checknumber(L, 2);
	set_control(name, value);
    	
	return 0;
}


int LUA_API luaopen_v4l(lua_State *L)
{
    const luaL_Reg driver[] = 
    {
        {"open", l_open},
        {"close", l_close},
        {"widht", l_width},
        {"height", l_height},
        {"getframe", l_getframe},
        {"list_controls", l_listcontrols},
        {"list_standards", l_liststandards},
        {"control", l_control},
        {"standard", l_standard},
        {NULL, NULL},
    };
    
    luaL_openlib (L, "v4l", driver, 0);
    lua_settable(L, -1);
    
    return 1;
}
