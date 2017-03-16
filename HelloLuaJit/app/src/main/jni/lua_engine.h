//
// Created by dsotsen on 16/6/30.
//

#ifndef HELLOLUAJIT_LUA_ENGINE_H
#define HELLOLUAJIT_LUA_ENGINE_H

extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

class LuaEngine {
public:
    LuaEngine();
    ~LuaEngine();

    void eval(const char * str);
private:
    lua_State *L;
};


#endif //HELLOLUAJIT_LUA_ENGINE_H
