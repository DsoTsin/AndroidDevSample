//
// Created by dsotsen on 16/6/30.
//

#include "v8_engine.h"
#include <android/asset_manager_jni.h>
#include <android/log.h>

#define TAG "V8Engine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__);
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__);

using namespace v8;

V8Engine::V8Engine(JNIEnv* env, jobject asset)
: isolate_(nullptr)
, initialized_(false)
{
    asset_manager_ = AAssetManager_fromJava(env, asset);
    initializeWithExtData();
    initialize();
}

V8Engine::~V8Engine() {
    destroy();
}

static void printMessage(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (args.Length() < 1) return;
    String::Utf8Value value(args[0]);
    LOGI("V8Js:%s", *value);
}

void V8Engine::destroy() {
    isolate_->Dispose();
    V8::Dispose();
    V8::ShutdownPlatform();
}

bool V8Engine::eval(Local<String> script, v8::Local<v8::Value> * result) {
    LOGI("engine eval");
    // Enter a scope isolate
    Isolate::Scope isolate_scope(getIsoloate());
    HandleScope handle_scope(getIsoloate());
    Local<Context> context = Local<Context>::New(getIsoloate(), context_);
    Context::Scope context_scope(context);

    // We're just about to compile the script; set up an error handler to
    // catch any exceptions the script might throw.
    TryCatch try_catch(getIsoloate());
    // Compile the script and check for errors.
    Local<Script> compiled_script;
    if (!Script::Compile(context, script).ToLocal(&compiled_script)) {
        String::Utf8Value error(try_catch.Exception());
        LOGE(*error);
        return false;
    }

    // Run the script!
    auto val = compiled_script->Run(context);
    if (!val.ToLocal(result)) {
        // The TryCatch above is still in effect and will have caught the error.
        String::Utf8Value error(try_catch.Exception());
        LOGE(*error);
        return false;
    }
    return true;
}

void V8Engine::initializeWithExtData() {
    AAsset * natives = AAssetManager_open(asset_manager_, "natives_blob.bin", AASSET_MODE_BUFFER);
    LOGI("  nativeblob size=%d", AAsset_getLength(natives));

    char *ntdat = new char[AAsset_getLength(natives)];
    memcpy(ntdat, AAsset_getBuffer(natives), AAsset_getLength(natives));
    StartupData ndata = {ntdat, AAsset_getLength(natives)};
    V8::SetNativesDataBlob(&ndata);
    AAsset_close(natives);

    AAsset * snapshot = AAssetManager_open(asset_manager_, "snapshot_blob.bin", AASSET_MODE_BUFFER);
    LOGI("  snapshot size=%d", AAsset_getLength(snapshot));

    char *snapdat = new char[AAsset_getLength(snapshot)];
    memcpy(snapdat, AAsset_getBuffer(snapshot), AAsset_getLength(snapshot));
    StartupData sdata = {snapdat, AAsset_getLength(snapshot)};
    V8::SetSnapshotDataBlob(&sdata);
    AAsset_close(snapshot);

    auto platform = platform::CreateDefaultPlatform();
    V8::InitializePlatform(platform);
    V8::Initialize();
}

void V8Engine::initialize() {
    // V8::InitializeICU("/sdcard/icudata.dat");
    Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = &buffer_allocator_;
    auto isolate = Isolate::New(create_params);
    isolate_ = isolate;
    HandleScope handleScope(isolate);
    global_template_ = ObjectTemplate::New(isolate);
    global_template_->Set(String::NewFromUtf8(isolate, "print"), FunctionTemplate::New(isolate, printMessage));
    Local<Context> context = Context::New(isolate, nullptr, global_template_);
    context_.Reset(isolate, context);
    LOGI("engine initialized..");
}

void V8Engine::setGlobalVariables(std::string const & prop, std::string const & val) {
    HandleScope handle_scope(isolate_);
    Local<Context> context = Local<Context>::New(isolate_, context_);
    v8::Context::Scope context_scope(context);
    v8::Local<v8::Value> value = v8::JSON::Parse(v8::String::NewFromUtf8(isolate_, val.c_str()));
    context->Global()->Set(v8::String::NewFromUtf8(isolate_, prop.c_str()), value);
}
