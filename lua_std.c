// std

#include "lua_std.h"

//

LUAMOD_API int luaopen_std( lua_State *L ) {
    lua_newmt(L, LUA_MT_LOCK, __lock_index, lua_std_lock_gc);

    luaL_newlib(L, __index);

    lua_newtable(L);
        #include "lua_std_flags.c"
    lua_setfield(L, -2, "f");

    return 1;
}

static int lua_std_lock( lua_State *L ) {
    const char *path = luaL_checkstring(L, 1);

    int fd = open(path, O_CREAT | O_TRUNC, 00644);
    if ( fd < 0 ) {
        lua_errno(L);
    }

    int r = flock(fd, LOCK_EX | LOCK_NB);
    if ( r < 0 ) {
        if ( errno==EWOULDBLOCK ) {
            close(fd);
            lua_fail(L, "already locked (by another process?)", 0);
        } else {
            close(fd);
            unlink(path);
            lua_errno(L);
        }
    }

    lua_ud_lock *lock = (lua_ud_lock *)lua_newuserdata(L, sizeof(lua_ud_lock));
    if ( !lock ) {
        flock(fd, LOCK_UN);
        close(fd);
        unlink(path);
        lua_fail(L, "lua_ud_lock alloc failed", 0);
    }

    lock->fd = fd;

    size_t path_size = sizeof(char) * (strlen(path) + 1);
    lock->path = malloc(path_size);
    if ( lock->path==NULL ) {
        flock(fd, LOCK_UN);
        close(fd);
        unlink(path);
        lua_fail(L, "lock->path alloc failed", 0);
    }

    memcpy(lock->path, path, path_size);

    luaL_setmetatable(L, LUA_MT_LOCK);

    return 1;
}

static int lua_std_lock_gc( lua_State *L ) {
    lua_ud_lock *lock = luaL_checkudata(L, 1, LUA_MT_LOCK);

    if ( lock->fd >= 0 ) {
        flock(lock->fd, LOCK_UN);
        close(lock->fd);
        unlink(lock->path);
        free(lock->path);

        lock->fd = -1;
    }

    return 0;
}

static int lua_std_finite( lua_State *L ) {
    double n = lua_tonumber(L, 1);
    lua_pushboolean(L, finite(n));
    return 1;
}

static int lua_std_get_pid( lua_State *L ) {
    lua_pushinteger(L, getpid());
    return 1;
}

static int lua_std_get_uid( lua_State *L ) {
    lua_pushinteger(L, getuid());
    return 1;
}

static int lua_std_get_gid( lua_State *L ) {
    lua_pushinteger(L, getgid());
    return 1;
}

// 0,5-20,95-100=0    1,21,101=1    2-4,22-24,102-104=2
static int lua_std_intcase( lua_State *L ) {
    int num = abs(luaL_checkinteger(L, 1));
    int mod10  = num % 10;  // последний знак
    int mod100 = num % 100; // 2 последних знака

    if ( mod10==0 || (mod10>4&&mod10<10) || (mod100>4&&mod100<20) ) {
        lua_pushinteger(L, 0); // томатов
    } else if ( mod10==1 ) {
        lua_pushinteger(L, 1); // томат
    } else {
        lua_pushinteger(L, 2); // томата
    }

    return 1;
}

// old style:
    //int seconds = luaL_checkinteger(L, 1);
    //zmq_sleep(seconds);
    //return 0;
static int lua_std_sleep( lua_State *L ) {
    double input = luaL_optnumber(L, 1, 0);
    struct timespec t;
    t.tv_sec  = (long)input;
    t.tv_nsec = (long)((input - (long)input) * 1e9);
    if ( nanosleep(&t, NULL) == -1 ) lua_errno(L);
    return 1;
}

// CLOCK_REALTIME
// CLOCK_REALTIME_COARSE
// CLOCK_MONOTONIC
// CLOCK_MONOTONIC_COARSE
// CLOCK_MONOTONIC_RAW
// CLOCK_BOOTTIME
// CLOCK_PROCESS_CPUTIME_ID
// CLOCK_THREAD_CPUTIME_ID
// old style:
    // #include <sys/time.h>
    // struct timeval tv;
    // gettimeofday(&tv, NULL);
    // lua_pushnumber(L, tv.tv_sec + tv.tv_usec * 1e-6);
static int lua_std_microtime( lua_State *L ) {
    struct timespec t;
    if ( clock_gettime(CLOCK_REALTIME, &t) == -1 ) lua_errno(L);
    lua_pushnumber(L, t.tv_sec + t.tv_nsec * 1e-9);
    return 1;
}

static int lua_std_chmod( lua_State *L ) {
    const char *path = luaL_checkstring(L, 1);
    long int mode = oct2dec(luaL_checknumber(L, 2));

    int r = chmod(path, (mode_t)mode);

    if ( r == -1 ) {
        lua_errno(L);
    } else {
        lua_pushboolean(L, 1);
        return 1;
    }
}

static int lua_std_fchmod( lua_State *L ) {
    int fd = luaL_checknumber(L, 1);
    long int mode = oct2dec(luaL_checknumber(L, 2));

    int r = fchmod(fd, (mode_t)mode);

    if ( r == -1 ) {
        lua_errno(L);
    } else {
        lua_pushboolean(L, 1);
        return 1;
    }
}

static int lua_std_strict( lua_State *L ) {
    int t;

    if ( lua_istable(L, 1) ) {
        lua_settop(L, 1);
    } else {
        lua_settop(L, 0);
        lua_pushglobaltable(L);
    }

    if ( lua_getmetatable(L, 1) ) {
        t = lua_getfield(L, -1, "__index");
        lua_pop(L, 1);
        if ( t ) {
            lua_fail(L, "_G metatable already have __index method", 0);
        }

        t = lua_getfield(L, -1, "__newindex");
        lua_pop(L, 1);
        if ( t ) {
            lua_fail(L, "_G metatable already have __newindex method", 0);
        }

        luaL_setfuncs(L, __strict, 0);
    } else {
        luaL_newlib(L, __strict);
        lua_setmetatable(L, 1);
    }

    lua_pushboolean(L, 1);
    return 1;
}

static int lua_std_strict__index( lua_State *L ) {
    size_t len;
    const char *str = luaL_tolstring(L, 2, &len);

    if ( strncmp("trace", str, len)==0 ) {
        lua_getglobal(L, "require");
        lua_insert(L, 2);
        lua_settop(L, 3);

        if ( lua_pcall(L, 1, 1, 0) != LUA_OK ) {
            return luaL_error(L, "std.strict autoload failed: %s", lua_tostring(L, -1));
        } else { // _G, trace
            lua_pushvalue(L, 2); // _G, trace, trace
            lua_insert(L, 1); // trace, _G, trace
            lua_pushstring(L, str); // trace, _G, trace, "trace"
            lua_insert(L, -2); // trace, _G, "trace", trace
            lua_rawset(L, 2); // trace, _G
            lua_settop(L, 1);// trace
            return 1;
        }
    } else {
        return luaL_error(L, "strict get: unknown key: '%s'", str);
    }
}

static int lua_std_strict__newindex( lua_State *L ) {
    size_t len;
    const char *str = luaL_tolstring(L, 2, &len);

    return luaL_error(L, "strict set: unknown key: '%s'", str);
}

static int lua_std_getrlimit( lua_State *L ) {
    int resource = luaL_checkinteger(L, 1);

    struct rlimit old_limit;

    int r = getrlimit(resource, &old_limit);

    if ( r == -1 ) {
        lua_errno(L);
    } else {
        lua_pushinteger(L, old_limit.rlim_cur);
        lua_pushinteger(L, old_limit.rlim_max);
        return 2;
    }
}

static int lua_std_setrlimit( lua_State *L ) {
    int resource = luaL_checkinteger(L, 1);

    struct rlimit new_limit;

    new_limit.rlim_cur = luaL_checkinteger(L, 2);
    new_limit.rlim_max = luaL_checkinteger(L, 3);

    int r = setrlimit(resource, &new_limit);

    if ( r == -1 ) {
        lua_errno(L);
    } else {
        lua_pushboolean(L, 1);
        return 1;
    }
}

static int lua_std_trim( lua_State *L ) {
    const char *front;
    const char *end;
    size_t size;

    front = luaL_checklstring(L, 1, &size);
    end = &front[size - 1];

    for ( ; size && isspace(*front); size--,front++ );
    for ( ; size && isspace(*end); size--,end-- );

    lua_pushlstring(L, front, (size_t)(end - front) + 1);

    return 1;
}

static int lua_std_concat( lua_State *L ) {
    size_t len, glue_len;
    const char *str, *glue;
    int i, n;
    luaL_Buffer B;



    glue = luaL_checklstring(L, 1, &glue_len);
    n = lua_gettop(L);

    if ( n==1 ) {
        return 1;
    } else {
        luaL_buffinit(L, &B);

        for ( i=2; i<=n; ++i ) {
            str = luaL_tolstring(L, i, &len);
            luaL_addlstring(&B, str, len);
            if ( i < n ) {
                luaL_addlstring(&B, glue, glue_len);
            }
        }

        luaL_pushresult(&B);
        return 1;
    }
}

static int lua_std_sandbox( lua_State *L ) {
    const luaL_Reg *lib;

    if ( lua_istable(L, 1) ) {
        lua_settop(L, 1);
    } else {
        lua_settop(L, 0);
        lua_newtable(L);
    }

    for ( lib=__std_lua_libs; lib->func; lib++ ) {
        lua_pushstring(L, lib->name);
        luaL_requiref(L, lib->name, lib->func, 0);
        lua_rawset(L, -3);
    }

    return 1;
}

//

long int oct2dec( long int octal ) {
    long int decimal = 0;
    int i = 0;
    while ( octal ) {
        decimal += (octal % 10) * pow(8, i++);
        octal /= 10;
    }
    return decimal;
}
