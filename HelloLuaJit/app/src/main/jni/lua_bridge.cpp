#include <jni.h>
#include <android/log.h>
#include <string>
#include <sstream>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

#define TAG "LuaVM"

lua_State *L = NULL;


static void print(lua_State* state) {
    const char* msg = luaL_checkstring(state, 1);
    __android_log_print(ANDROID_LOG_INFO, TAG, msg);
}

void test(JNIEnv * env, jobject jRoot, jobject jObj) {
    jclass clasz = env->FindClass("com/tencent/helloluajit/BenchListener");
    jmethodID method = env->GetMethodID(clasz, "onResult", "(Ljava/lang/String;)V");
    std::stringstream out;
    env->CallVoidMethod(jObj, method, env->NewStringUTF(out.str().c_str()));
}

static void eval(JNIEnv * env, jobject jRoot, jstring jObj) {
    const char* str;
    str = env->GetStringUTFChars(jObj, false);
    if(str == NULL) {
        return;
    }
    __android_log_print(ANDROID_LOG_INFO, TAG, "evaluating (%s).", str);
    if(luaL_dostring(L, str)) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "failed to evaluate (%s).", str);;
    }
    env->ReleaseStringUTFChars(jObj, str);
}

static JNINativeMethod gMethods[] = {
        {"makeTest",    "(Lcom/tencent/helloluajit/BenchListener;)V",   (void *)test},
        {"eval",        "(Ljava/lang/String;)V",                        (void *)eval}
};

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    jint result = -1;
    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR, TAG," JNI init failed.. ");
        goto bail;
    }
    __android_log_print(ANDROID_LOG_DEBUG, TAG," JNI init succeeded.. ");
    if(JNI_OK != env->RegisterNatives(env->FindClass("com/tencent/helloluajit/MainActivity"), gMethods, 2))
    {
       __android_log_print(ANDROID_LOG_ERROR, TAG,"jni method register failed..");
    }
    result = JNI_VERSION_1_4;

    L = luaL_newstate();
    lua_register(L, "aprint", print);

    bail:
    return result;
}

void JNI_OnUnload(JavaVM* vm, void* reserved) {
    lua_close(L);
    L = NULL;
    __android_log_print(ANDROID_LOG_ERROR, TAG," JavaVM and LuaVM closed..");
}