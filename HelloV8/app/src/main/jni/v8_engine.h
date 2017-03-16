//
// Created by dsotsen on 16/6/30.
//

#ifndef HELLOV8_V8_ENGINE_H
#define HELLOV8_V8_ENGINE_H

#include <v8.h>
#include <libplatform/libplatform.h>
#include "v8_allocator.h"
#include <string>
#include <jni.h>
#include <map>
#include <android/asset_manager.h>

class V8Engine {
public:
    V8Engine(JNIEnv* env, jobject asset);
    ~V8Engine();

    v8::Isolate* getIsoloate() const { return isolate_; }

    void setGlobalVariables(std::string const & prop, std::string const & val);
    bool eval(v8::Local<v8::String> script, v8::Local<v8::Value> * ret);

private:
    v8::Isolate*                    isolate_;
    v8::Global<v8::Context>         context_;
    v8::Local<v8::ObjectTemplate>   global_template_;
    ArrayBufferAllocator            buffer_allocator_;
    AAssetManager *                 asset_manager_;
    bool                            initialized_;

private:
    void initializeWithExtData();

    void initialize();
    void destroy();

    v8::Context::Scope *            context_scope_;
    v8::Persistent<v8::Context> *   primary_context_;
};


#endif //HELLOV8_V8_ENGINE_H
