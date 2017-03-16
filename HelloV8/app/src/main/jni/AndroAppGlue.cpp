//
// Created by dsotsen on 2016/10/18.
//

#include <jni.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>

#include "AndroAppGlue.h"

#include <android/log.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "threaded_app", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "threaded_app", __VA_ARGS__))

/* For debug builds, always enable the debug traces in this library */
#ifndef NDEBUG
#  define LOGV(...)  ((void)__android_log_print(ANDROID_LOG_VERBOSE, "threaded_app", __VA_ARGS__))
#else
#  define LOGV(...)  ((void)0)
#endif

namespace android {

    AppCmd App::readCmd() {
        AppCmd cmd;
        if (read(msgread, &cmd, sizeof(cmd)) == sizeof(cmd)) {
            switch (cmd) {
                case AppCmd::SAVE_STATE:
                    freeSaveState();
                    break;
            }
            return cmd;
        } else {
            LOGE("No data on command pipe!");
        }
        return (AppCmd)-1;
    }


    void App::preExecCmd(AppCmd cmd) {
        switch (cmd) {
            case AppCmd::INPUT_CHANGED:
                LOGV("AppCmd::INPUT_CHANGED\n");
                pthread_mutex_lock(& mutex);
                if ( inputQueue != NULL) {
                    AInputQueue_detachLooper( inputQueue);
                }
                 inputQueue =  pendingInputQueue;
                if ( inputQueue != NULL) {
                    LOGV("Attaching input queue to looper");
                    AInputQueue_attachLooper( inputQueue,
                                              looper, (int)LooperID::INPUT, NULL,
                                             & inputPollSource);
                }
                pthread_cond_broadcast(& cond);
                pthread_mutex_unlock(& mutex);
                break;

            case AppCmd::INIT_WINDOW:
                LOGV("AppCmd::INIT_WINDOW\n");
                pthread_mutex_lock(& mutex);
                 window =  pendingWindow;
                pthread_cond_broadcast(& cond);
                pthread_mutex_unlock(& mutex);
                break;

            case AppCmd::TERM_WINDOW:
                LOGV("AppCmd::TERM_WINDOW\n");
                pthread_cond_broadcast(& cond);
                break;

            case AppCmd::RESUME:
            case AppCmd::START:
            case AppCmd::PAUSE:
            case AppCmd::STOP:
                LOGV("activityState=%d\n", cmd);
                pthread_mutex_lock(& mutex);
                 activityState = cmd;
                pthread_cond_broadcast(& cond);
                pthread_mutex_unlock(& mutex);
                break;

            case AppCmd::CONFIG_CHANGED:
                LOGV("AppCmd::CONFIG_CHANGED\n");
                AConfiguration_fromAssetManager( config,
                                                 activity->assetManager);
                printConfig();
                break;

            case AppCmd::DESTROY:
                LOGV("AppCmd::DESTROY\n");
                 destroyRequested = 1;
                break;
        }
    }


    void App::postExecCmd(AppCmd cmd) {
        switch (cmd) {
            case  AppCmd::TERM_WINDOW:
                LOGV(" AppCmd::TERM_WINDOW\n");
                pthread_mutex_lock(&this->mutex);
                this->window = NULL;
                pthread_cond_broadcast(&this->cond);
                pthread_mutex_unlock(&this->mutex);
                break;

            case  AppCmd::SAVE_STATE:
                LOGV(" AppCmd::SAVE_STATE\n");
                pthread_mutex_lock(&this->mutex);
                this->stateSaved = 1;
                pthread_cond_broadcast(&this->cond);
                pthread_mutex_unlock(&this->mutex);
                break;

            case  AppCmd::RESUME:
                this->freeSaveState();
                break;
        }
    }

    void App::freeSaveState() {
        pthread_mutex_lock(&mutex);
        if (savedState != NULL) {
            ::free(savedState);
            savedState = NULL;
            savedStateSize = 0;
        }
        pthread_mutex_unlock(&mutex);
    }

    void App::printConfig() {
        char lang[2], country[2];
        AConfiguration_getLanguage(config, lang);
        AConfiguration_getCountry(config, country);

        LOGV("Config: mcc=%d mnc=%d lang=%c%c cnt=%c%c orien=%d touch=%d dens=%d "
                     "keys=%d nav=%d keysHid=%d navHid=%d sdk=%d size=%d long=%d "
                     "modetype=%d modenight=%d",
             AConfiguration_getMcc( config),
             AConfiguration_getMnc( config),
             lang[0], lang[1], country[0], country[1],
             AConfiguration_getOrientation( config),
             AConfiguration_getTouchscreen( config),
             AConfiguration_getDensity( config),
             AConfiguration_getKeyboard( config),
             AConfiguration_getNavigation( config),
             AConfiguration_getKeysHidden( config),
             AConfiguration_getNavHidden( config),
             AConfiguration_getSdkVersion( config),
             AConfiguration_getScreenSize( config),
             AConfiguration_getScreenLong( config),
             AConfiguration_getUiModeType( config),
             AConfiguration_getUiModeNight( config));
    }

    void App::destroy() {
        LOGV("this_destroy!");
        freeSaveState();
        pthread_mutex_lock(&this->mutex);
        if (this->inputQueue != NULL) {
            AInputQueue_detachLooper(this->inputQueue);
        }
        AConfiguration_delete(this->config);
        this->destroyed = 1;
        pthread_cond_broadcast(&this->cond);
        pthread_mutex_unlock(&this->mutex);
    }

    struct InputProcessor : public PollSource {
        InputProcessor(LooperID _id, App * _app): id(_id), app(_app) {}

        void process(App * app) override {
            AInputEvent* event = NULL;
            while (AInputQueue_getEvent(app->inputQueue, &event) >= 0) {
                LOGV("New input event: type=%d\n", AInputEvent_getType(event));
                if (AInputQueue_preDispatchEvent(app->inputQueue, event)) {
                    continue;
                }
                int32_t handled = app->onInputEvent(event);
                AInputQueue_finishEvent(app->inputQueue, event, handled);
            }
        }
    };

    struct CmdProcessor : public PollSource {

        CmdProcessor(LooperID _id, App * _app): id(_id), app(_app) {}

        void process(App *app) override {
            AppCmd cmd = app->readCmd();
            app->preExecCmd(cmd);
            app->onCmd(cmd);
            app->postExecCmd(cmd);
        }
    };

    void App::writeCmd(AppCmd cmd) {
        if (write(this->msgwrite, &cmd, sizeof(cmd)) != sizeof(cmd)) {
            LOGE("Failure writing this cmd: %s\n", strerror(errno));
        }
    }

    void App::setInput(AInputQueue *inputQueue) {
        pthread_mutex_lock(&this->mutex);
        this->pendingInputQueue = inputQueue;
        writeCmd(AppCmd::INPUT_CHANGED);
        while (this->inputQueue != this->pendingInputQueue) {
            pthread_cond_wait(&this->cond, &this->mutex);
        }
        pthread_mutex_unlock(&this->mutex);
    }

    void App::setWindow(ANativeWindow *window) {
        pthread_mutex_lock(&this->mutex);
        if (this->pendingWindow != NULL) {
            writeCmd(AppCmd::TERM_WINDOW);
        }
        this->pendingWindow = window;
        if (window != NULL) {
            writeCmd(AppCmd::INIT_WINDOW);
        }
        while (this->window != this->pendingWindow) {
            pthread_cond_wait(&this->cond, &this->mutex);
        }
        pthread_mutex_unlock(&this->mutex);
    }

    void App::setActivityState(AppCmd appCmd) {
        pthread_mutex_lock(&this->mutex);
        writeCmd(appCmd);
        while (this->activityState != appCmd) {
            pthread_cond_wait(&this->cond, &this->mutex);
        }
        pthread_mutex_unlock(&this->mutex);
    }

    void App::free() {
        pthread_mutex_lock(&this->mutex);
        writeCmd(AppCmd::DESTROY);
        while (!this->destroyed) {
            pthread_cond_wait(&this->cond, &this->mutex);
        }
        pthread_mutex_unlock(&this->mutex);

        close(this->msgread);
        close(this->msgwrite);
        pthread_cond_destroy(&this->cond);
        pthread_mutex_destroy(&this->mutex);
        //free(); free app 
    }

    void App::onDestroy(ANativeActivity* activity) {
        LOGV("Destroy: %p\n", activity);
        auto app = ((App*)activity->instance);
        app->free();
    }

    void App::onStart(ANativeActivity* activity) {
        LOGV("Start: %p\n", activity);
        auto app = (App*)activity->instance;
        app->setActivityState(AppCmd::START);
    }

    void App::onResume(ANativeActivity* activity) {
        LOGV("Resume: %p\n", activity);
        auto app = (App*)activity->instance;
        app->setActivityState(AppCmd::RESUME);
    }

    void* App::onSaveInstanceState(ANativeActivity* activity, size_t* outLen) {
        App* android_app = (App*)activity->instance;
        void* savedState = NULL;

        LOGV("SaveInstanceState: %p\n", activity);
        pthread_mutex_lock(&android_app->mutex);
        android_app->stateSaved = 0;
        android_app->writeCmd(AppCmd::SAVE_STATE);
        while (!android_app->stateSaved) {
            pthread_cond_wait(&android_app->cond, &android_app->mutex);
        }

        if (android_app->savedState != NULL) {
            savedState = android_app->savedState;
            *outLen = android_app->savedStateSize;
            android_app->savedState = NULL;
            android_app->savedStateSize = 0;
        }

        pthread_mutex_unlock(&android_app->mutex);

        return savedState;
    }

    void App::onPause(ANativeActivity* activity) {
        LOGV("Pause: %p\n", activity);
        auto app = (App*)activity->instance;
        app->setActivityState(AppCmd::PAUSE);
    }

    void App::onStop(ANativeActivity* activity) {
        LOGV("Stop: %p\n", activity);
        auto app = (App*)activity->instance;
        app->setActivityState(AppCmd::STOP);
    }

    void App::onConfigurationChanged(ANativeActivity* activity) {
        App* android_app = (App*)activity->instance;
        LOGV("ConfigurationChanged: %p\n", activity);
        auto app = (App*)activity->instance;
        app->setActivityState(AppCmd::CONFIG_CHANGED);
    }

    void App::onLowMemory(ANativeActivity* activity) {
        App* android_app = (App*)activity->instance;
        LOGV("LowMemory: %p\n", activity);
        auto app = (App*)activity->instance;
        app->setActivityState(AppCmd::LOW_MEMORY);
    }

    void App::onWindowFocusChanged(ANativeActivity* activity, int focused) {
        LOGV("WindowFocusChanged: %p -- %d\n", activity, focused);
        auto app = (App*)activity->instance;
        app->setActivityState(focused ? AppCmd::GAINED_FOCUS : AppCmd::LOST_FOCUS);
    }

    void App::onNativeWindowCreated(ANativeActivity* activity, ANativeWindow* window) {
        LOGV("NativeWindowCreated: %p -- %p\n", activity, window);
        auto app = (App*)activity->instance;
        app->setWindow(window);
    }

    void App::onNativeWindowDestroyed(ANativeActivity* activity, ANativeWindow* window) {
        LOGV("NativeWindowDestroyed: %p -- %p\n", activity, window);
        auto app = (App*)activity->instance;
        app->setWindow(nullptr);
    }

    void App::onInputQueueCreated(ANativeActivity* activity, AInputQueue* queue) {
        LOGV("InputQueueCreated: %p -- %p\n", activity, queue);
        auto app = (App*)activity->instance;
        app->setInput(queue);
    }

    void App::onInputQueueDestroyed(ANativeActivity* activity, AInputQueue* queue) {
        LOGV("InputQueueDestroyed: %p -- %p\n", activity, queue);
        auto app = (App*)activity->instance;
        app->setInput(nullptr);
    }

    App::App(ANativeActivity* _activity, void *savedState, size_t savedStateSize) {
        this->activity = _activity;

        pthread_mutex_init(&this->mutex, NULL);
        pthread_cond_init(&this->cond, NULL);

        if (savedState != NULL) {
            this->savedState = malloc(savedStateSize);
            this->savedStateSize = savedStateSize;
            memcpy(this->savedState, savedState, savedStateSize);
        }

        int msgpipe[2];
        if (pipe(msgpipe)) {
            LOGE("could not create pipe: %s", strerror(errno));
            return;
        }
        this->msgread = msgpipe[0];
        this->msgwrite = msgpipe[1];

        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&this->thread, &attr, entry, this);

        // Wait for thread to start.
        pthread_mutex_lock(&this->mutex);
        while (!this->running) {
            pthread_cond_wait(&this->cond, &this->mutex);
        }
        pthread_mutex_unlock(&this->mutex);
    }

    void *App::entry(void *param) {
        App* android_app = (App*)param;
        android_app->config = AConfiguration_new();
        AConfiguration_fromAssetManager(android_app->config, android_app->activity->assetManager);
        android_app->printConfig();

        android_app->cmdPollSource = new CmdProcessor(LooperID::MAIN, android_app);
        android_app->inputPollSource = new InputProcessor(LooperID::INPUT, android_app);

        ALooper* looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
        ALooper_addFd(looper, android_app->msgread, (int)LooperID::MAIN, ALOOPER_EVENT_INPUT, NULL,
                      &android_app->cmdPollSource);
        android_app->looper = looper;

        pthread_mutex_lock(&android_app->mutex);
        android_app->running = 1;
        pthread_cond_broadcast(&android_app->cond);
        pthread_mutex_unlock(&android_app->mutex);

        android_app->mainLoop();

        android_app->destroy();
        return NULL;
    }

}

using namespace android;

void ANativeActivity_onCreate(ANativeActivity* activity,
                              void* savedState, size_t savedStateSize) {
    LOGV("Creating: %p\n", activity);
    activity->callbacks->onDestroy = App::onDestroy;
    activity->callbacks->onStart = App::onStart;
    activity->callbacks->onResume = App::onResume;
    activity->callbacks->onSaveInstanceState = App::onSaveInstanceState;
    activity->callbacks->onPause = App::onPause;
    activity->callbacks->onStop = App::onStop;
    activity->callbacks->onConfigurationChanged = App::onConfigurationChanged;
    activity->callbacks->onLowMemory = App::onLowMemory;
    activity->callbacks->onWindowFocusChanged = App::onWindowFocusChanged;
    activity->callbacks->onNativeWindowCreated = App::onNativeWindowCreated;
    activity->callbacks->onNativeWindowDestroyed = App::onNativeWindowDestroyed;
    activity->callbacks->onInputQueueCreated = App::onInputQueueCreated;
    activity->callbacks->onInputQueueDestroyed = App::onInputQueueDestroyed;

    activity->instance = new App(activity, savedState, savedStateSize);
}