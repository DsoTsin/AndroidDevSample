#include <jni.h>
#include <android/log.h>
#include <sstream>
#include <android/asset_manager_jni.h>
#include "v8_engine.h"

#define TAG "HelloV8"

using namespace v8;


#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__);
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__);
#define LOGI(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__);

V8Engine* gV8 = nullptr;

void test(JNIEnv * env, jobject jRoot, jobject jObj) {
    std::stringstream ss;
    auto isolate = gV8->getIsoloate();
    HandleScope scope(isolate);
    MaybeLocal<String> source = String::NewFromUtf8(isolate, "print('Hello' + ', World!');", NewStringType::kNormal);

    Local<Value> ret;
    gV8->eval(source.ToLocalChecked(), &ret);

    jclass clasz = env->FindClass("com/tencent/hellov8/BenchListener");
    jmethodID method = env->GetMethodID(clasz, "onResult", "(Ljava/lang/String;)V");
    env->CallVoidMethod(jObj, method, env->NewStringUTF(ss.str().c_str()));
}

static JNINativeMethod benchMethod[] = {
        {"makeTest",       "(Lcom/tencent/hellov8/BenchListener;)V",            (void *)test}
};

jobject global_asset_manager = nullptr;

static void j_v8_engine_init(JNIEnv * env, jobject jRoot, jobject jObj) {
    jclass contextCls = env->FindClass("android/content/Context");
    jmethodID masset = env->GetMethodID(contextCls, "getAssets", "()Landroid/content/res/AssetManager;");
    jobject asset_manager = env->CallObjectMethod(jObj, masset); // activity.getAssets();
    global_asset_manager = env->NewGlobalRef(asset_manager);
    gV8 = new V8Engine(env, asset_manager);
    env->DeleteGlobalRef(global_asset_manager);
}

static void j_v8_engine_execute(JNIEnv * env, jobject jRoot, jstring jObj) {
    jboolean copied;
    const char* script = env->GetStringUTFChars(jObj, &copied);
    LOGD("script:%s", script);
}

static JNINativeMethod v8Method[] = {
        {"init",       "(Landroid/content/Context;)V",            (void *)j_v8_engine_init},
        {"execute",    "(Ljava/lang/String;)V",            (void *)j_v8_engine_execute}
};

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    jint result = -1;
    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        LOGE(" JNI init failed.. ");
        return result;
    }
    LOGI(" JNI init succeeded.. ");
    if(JNI_OK != env->RegisterNatives(env->FindClass("com/tencent/hellov8/MainActivity"),
                                      benchMethod, 1)) {
        LOGE(" method register failed..");
    }

    if(JNI_OK != env->RegisterNatives(env->FindClass("com/tencent/hellov8/V8Engine"),
                                      v8Method, 2)) {
        LOGE(" method register failed..");
    }

    result = JNI_VERSION_1_4;
    return result;
}
