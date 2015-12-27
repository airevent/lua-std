// std

#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lua_pd.h"

//

#define LUA_MT_LOCK "mt.lock"

//

typedef struct lua_ud_lock {
    int fd;
    char *path;
} lua_ud_lock;

//

LUAMOD_API int luaopen_std( lua_State *L );

static int lua_std_lock( lua_State *L );
static int lua_std_lock_gc( lua_State *L );

static int lua_std_finite( lua_State *L );
static int lua_std_get_pid( lua_State *L );
static int lua_std_get_uid( lua_State *L );
static int lua_std_get_gid( lua_State *L );
static int lua_std_intcase( lua_State *L );
static int lua_std_sleep( lua_State *L );
static int lua_std_microtime( lua_State *L );
static int lua_std_chmod( lua_State *L );
static int lua_std_fchmod( lua_State *L );

long int oct2dec( long int octal );

//

static const luaL_Reg __index[] = {
    {"lock", lua_std_lock},
    {"finite", lua_std_finite},
    {"get_pid", lua_std_get_pid},
    {"get_uid", lua_std_get_uid},
    {"get_gid", lua_std_get_gid},
    {"intcase", lua_std_intcase},
    {"sleep", lua_std_sleep},
    {"microtime", lua_std_microtime},
    {"chmod", lua_std_chmod},
    {"fchmod", lua_std_fchmod},
    {NULL, NULL}
};

static const luaL_Reg __lock_index[] = {
    {"unlock", lua_std_lock_gc},
    {NULL, NULL}
};
