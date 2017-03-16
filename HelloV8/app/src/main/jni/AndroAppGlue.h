//
// Created by dsotsen on 2016/10/18.
//

#ifndef HELLOV8_ANDROAPPGLUE_H
#define HELLOV8_ANDROAPPGLUE_H

#include <poll.h>
#include <pthread.h>
#include <sched.h>

#include <android/configuration.h>
#include <android/looper.h>
#include <android/native_activity.h>

namespace android {

    class App;

    enum class LooperID {
        /**
         * Looper data ID of commands coming from the app's main thread, which
         * is returned as an identifier from ALooper_pollOnce().  The data for this
         * identifier is a pointer to an android_poll_source structure.
         * These can be retrieved and processed with android_app_read_cmd()
         * and android_app_exec_cmd().
         */
        MAIN = 1,

        /**
         * Looper data ID of events coming from the AInputQueue of the
         * application's window, which is returned as an identifier from
         * ALooper_pollOnce().  The data for this identifier is a pointer to an
         * android_poll_source structure.  These can be read via the inputQueue
         * object of android_app.
         */
        INPUT = 2,

        /**
         * Start of user-defined ALooper identifiers.
         */
        USER = 3,
    };


    enum class AppCmd {
        /**
         * Command from main thread: the AInputQueue has changed.  Upon processing
         * this command, android_app->inputQueue will be updated to the new queue
         * (or NULL).
         */
                INPUT_CHANGED,

        /**
         * Command from main thread: a new ANativeWindow is ready for use.  Upon
         * receiving this command, android_app->window will contain the new window
         * surface.
         */
                 INIT_WINDOW,

        /**
         * Command from main thread: the existing ANativeWindow needs to be
         * terminated.  Upon receiving this command, android_app->window still
         * contains the existing window; after calling android_app_exec_cmd
         * it will be set to NULL.
         */
                 TERM_WINDOW,

        /**
         * Command from main thread: the current ANativeWindow has been resized.
         * Please redraw with its new size.
         */
                 WINDOW_RESIZED,

        /**
         * Command from main thread: the system needs that the current ANativeWindow
         * be redrawn.  You should redraw the window before handing this to
         * android_app_exec_cmd() in order to avoid transient drawing glitches.
         */
                 WINDOW_REDRAW_NEEDED,

        /**
         * Command from main thread: the content area of the window has changed,
         * such as from the soft input window being shown or hidden.  You can
         * find the new content rect in android_app::contentRect.
         */
                 CONTENT_RECT_CHANGED,

        /**
         * Command from main thread: the app's activity window has gained
         * input focus.
         */
                 GAINED_FOCUS,

        /**
         * Command from main thread: the app's activity window has lost
         * input focus.
         */
                 LOST_FOCUS,

        /**
         * Command from main thread: the current device configuration has changed.
         */
                 CONFIG_CHANGED,

        /**
         * Command from main thread: the system is running low on memory.
         * Try to reduce your memory use.
         */
                 LOW_MEMORY,

        /**
         * Command from main thread: the app's activity has been started.
         */
                 START,

        /**
         * Command from main thread: the app's activity has been resumed.
         */
                 RESUME,

        /**
         * Command from main thread: the app should generate a new saved state
         * for itself, to restore from later if needed.  If you have saved state,
         * allocate it with malloc and place it in android_app.savedState with
         * the size in android_app.savedStateSize.  The will be freed for you
         * later.
         */
                 SAVE_STATE,

        /**
         * Command from main thread: the app's activity has been paused.
         */
                 PAUSE,

        /**
         * Command from main thread: the app's activity has been stopped.
         */
                 STOP,

        /**
         * Command from main thread: the app's activity is being destroyed,
         * and waiting for the app thread to clean up and exit before proceeding.
         */
                 DESTROY,
    };
    
    struct PollSource {
        // The android_app this ident is associated with.
        LooperID id;
        App* app;
        // Function to call to perform the standard processing of data from
        // this source.
        virtual void process(App *) = 0;
    };
    
    class App {
    public:
        App(ANativeActivity* _activity, void* savedState, size_t savedStateSize);
        virtual ~App() {}

        virtual void onCmd(AppCmd cmd) {}

        // Fill this in with the function to process input events.  At this point
        // the event has already been pre-dispatched, and it will be finished upon
        // return.  Return 1 if you have handled the event, 0 for any default
        // dispatching.
        virtual int32_t onInputEvent(AInputEvent* event) {}

        virtual void mainLoop() {};


/**
 * Call when ALooper_pollAll() returns LOOPER_ID_MAIN, reading the next
 * app command message.
 */
        AppCmd readCmd();

/**
 * Call with the command returned by android_app_read_cmd() to do the
 * initial pre-processing of the given command.  You can perform your own
 * actions for the command after calling this function.
 */
        void preExecCmd(AppCmd cmd);

/**
 * Call with the command returned by android_app_read_cmd() to do the
 * final post-processing of the given command.  You must have done your own
 * actions for the command before calling this function.
 */
        void postExecCmd(AppCmd cmd);


        void* userData;

        // The ANativeActivity object instance that this app is running in.
        ANativeActivity* activity;

        // The current configuration the app is running in.
        AConfiguration* config;

    public:
        static void onDestroy(ANativeActivity* activity);

        static void onStart(ANativeActivity* activity);

        static void onResume(ANativeActivity* activity);

        static void* onSaveInstanceState(ANativeActivity* activity, size_t* outLen);

        static void onPause(ANativeActivity* activity);

        static void onStop(ANativeActivity* activity);

        static void onConfigurationChanged(ANativeActivity* activity);

        static void onLowMemory(ANativeActivity* activity);

        static void onWindowFocusChanged(ANativeActivity* activity, int focused);

        static void onNativeWindowCreated(ANativeActivity* activity, ANativeWindow* window);

        static void onNativeWindowDestroyed(ANativeActivity* activity, ANativeWindow* window);

        static void onInputQueueCreated(ANativeActivity* activity, AInputQueue* queue);

        static void onInputQueueDestroyed(ANativeActivity* activity, AInputQueue* queue);

        static void* entry(void* param);

    protected:
        // This is the last instance's saved state, as provided at creation time.
        // It is NULL if there was no state.  You can use this as you need; the
        // memory will remain around until you call android_app_exec_cmd() for
        // APP_CMD_RESUME, at which point it will be freed and savedState set to NULL.
        // These variables should only be changed when processing a APP_CMD_SAVE_STATE,
        // at which point they will be initialized to NULL and you can malloc your
        // state and place the information here.  In that case the memory will be
        // freed for you later.
        void* savedState;
        size_t savedStateSize;

        // The ALooper associated with the app's thread.
        ALooper* looper;

        // When non-NULL, this is the input queue from which the app will
        // receive user input events.
        AInputQueue* inputQueue;

        // When non-NULL, this is the window surface that the app can draw in.
        ANativeWindow* window;

        // Current content rectangle of the window; this is the area where the
        // window's content should be placed to be seen by the user.
        ARect contentRect;

        // Current state of the app's activity.  May be either APP_CMD_START,
        // APP_CMD_RESUME, APP_CMD_PAUSE, or APP_CMD_STOP; see below.
        AppCmd activityState;

        // This is non-zero when the application's NativeActivity is being
        // destroyed and waiting for the app thread to complete.
        int destroyRequested;

    private:
        void freeSaveState();
        void printConfig();
        void destroy();
        void writeCmd(AppCmd cmd);

        void setInput(AInputQueue* inputQueue);

        void setWindow(ANativeWindow* window);

        void setActivityState(AppCmd appCmd);

        void free();

        pthread_mutex_t mutex;
        pthread_cond_t cond;

        int msgread;
        int msgwrite;

        pthread_t thread;

        PollSource* cmdPollSource;
        PollSource* inputPollSource;

        int running;
        int stateSaved;
        int destroyed;
        int redrawNeeded;
        AInputQueue* pendingInputQueue;
        ANativeWindow* pendingWindow;
        ARect pendingContentRect;

        friend struct InputProcessor;
        friend struct CmdProcessor;
    };
}

#endif //HELLOV8_ANDROAPPGLUE_H
