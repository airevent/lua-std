// std

#include "lua_std.h"

//

LUAMOD_API int luaopen_std( lua_State *L ) {
    lua_newmt(L, LUA_MT_LOCK, __lock_index, lua_std_lock_gc);

    luaL_newlib(L, __index);
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
