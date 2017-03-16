#include <jni.h>
#include <arm_neon.h>
#include <android/log.h>
#include <malloc.h>
#include <sys/time.h>
#include <time.h>
#include <string>
#include <sstream>

const int DATA_SIZE = 1920*1080;

void neon_cpy(void *dst, void *src, size_t len) {
    for(size_t offset = 0; offset<len; offset+=4 ) {

    }
}

void test(JNIEnv * env, jobject jRoot, jobject jObj) {
    int *testSet1 = (int*)malloc(sizeof(int)*DATA_SIZE);

    for(uint32_t i = 0; i<DATA_SIZE; i++) {
        testSet1[i] = i;
    }


    clock_t begin = clock();

    for (uint32_t i=0; i<DATA_SIZE/4/2; i++) {
        int32_t *src = testSet1+i*4;
        int32_t *dest = testSet1+DATA_SIZE - 4*(i+1);
        int32x4_t tmp = vld1q_dup_s32(src);
        int32x4_t destData = vld1q_dup_s32(dest);
        int32x4_t rDestData = vrev64q_s32(destData);
        vst1q_s32(src, rDestData);
        vst1q_s32(dest, tmp);
    }

    clock_t end = clock();

    for (uint32_t i = 0; i<DATA_SIZE/2; i++) {
        int t = testSet1[i];
        int d = testSet1[DATA_SIZE-1-i];
        testSet1[i] = d;
        testSet1[DATA_SIZE-1-i] = t;
    }

    clock_t end2 = clock();

    clock_t cost1 = end-begin;
    clock_t cost2 = end2-end;

    __android_log_print(ANDROID_LOG_DEBUG, "NEON", "last number is %d, acc=%.1fx", testSet1[DATA_SIZE-1], 1.f*cost2/cost1);

    free(testSet1);

    jclass clasz = env->FindClass("com/tencent/helloneon/BenchListener");
    jmethodID method = env->GetMethodID(clasz, "onResult", "(Ljava/lang/String;)V");

    std::stringstream out;
    out << "benchResult:" << 1.f*cost2/cost1;
    env->CallVoidMethod(jObj, method, env->NewStringUTF(out.str().c_str()));
}

static JNINativeMethod gMethods[] = {
        {"makeTest",       "(Lcom/tencent/helloneon/BenchListener;)V",            (void *)test}
};

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    jint result = -1;
    //判断一下JNI的版本
    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR, "NEON"," JNI init failed.. ");
        goto bail;
    }

    __android_log_print(ANDROID_LOG_DEBUG, "NEON"," JNI init succeeded.. ");

   if(JNI_OK != env->RegisterNatives(env->FindClass("com/tencent/helloneon/MainActivity"), gMethods, 1))
   {
       __android_log_print(ANDROID_LOG_ERROR, "NEON"," method register failed..");
   }

    result = JNI_VERSION_1_4;

    bail:
    return result;
}
