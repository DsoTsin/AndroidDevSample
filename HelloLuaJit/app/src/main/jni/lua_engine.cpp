//
// Created by dsotsen on 16/6/30.
//

#include "lua_engine.h"
#include <android/log.h>

#define TAG  "LuaEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

LuaEngine::LuaEngine()
: L(nullptr) {

}

LuaEngine::~LuaEngine() {

}

void LuaEngine::eval(const char *str) {
    if(luaL_dostring(L, str)) {
        LOGI("failed to evaluate (%s).", str);
    }
}
