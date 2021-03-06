#include <jni.h>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

static JavaVM *jvm;
static jclass mainActivityClass = NULL;
static jmethodID renderCallbackMethodId;
static const unsigned int buffer_size = 256*144*4;

extern "C"
{
#include "chester.h"

chester gChester;

// Button states
bool button_a;
bool button_b;
bool button_start;
bool button_select;
bool button_up;
bool button_down;
bool button_left;
bool button_right;

static int keysCb(keys* k)
{
    k->a = button_a;
    k->b = button_b;
    k->start = button_start;
    k->select = button_select;
    k->up = button_up;
    k->down = button_down;
    k->left = button_left;
    k->right = button_right;

    return button_a || button_b ||
            button_start || button_select ||
            button_up || button_down || button_left || button_right;
}

static uint32_t ticksCb()
{
    struct timespec res;
    clock_gettime(CLOCK_MONOTONIC, &res);
    return static_cast<uint32_t>(1000.0 * res.tv_sec + (double) res.tv_nsec / 1e6);
}

static void delayCb(uint32_t ms)
{
    usleep(ms * 1000);
}

static bool initGpuCb(gpu* g)
{
    g->pixel_data = malloc(buffer_size);
    return true;
}

static void uninitGpuCb(gpu* g)
{
    if (g->pixel_data) {
        free(g->pixel_data);
        g->pixel_data = NULL;
    }
}

static bool initPixelData(gpu* g)
{
    // Buffer allocated in initGpuCb can be reused
    assert (g->pixel_data);

    return true;
}

static void renderCb(gpu* g)
{
    JNIEnv *env;
    jvm->AttachCurrentThread(&env, NULL);

    jobject directByteBuffer = env->NewDirectByteBuffer(g->pixel_data, buffer_size);
    env->CallStaticVoidMethod(mainActivityClass, renderCallbackMethodId, directByteBuffer);
    env->DeleteLocalRef(directByteBuffer);
}

JNIEXPORT jboolean JNICALL
Java_com_chester_chesterapp_MainActivity_initChester(
        JNIEnv *env,
        jobject, /* this */
        jstring romPath,
        jstring savePath) {
    jint rs = env->GetJavaVM(&jvm);
    assert (rs == JNI_OK);

    register_delay_callback(&gChester, &delayCb);
    register_keys_callback(&gChester, &keysCb);
    register_get_ticks_callback(&gChester, &ticksCb);
    register_gpu_init_callback(&gChester, &initGpuCb);
    register_gpu_uninit_callback(&gChester, &uninitGpuCb);
    register_gpu_alloc_image_buffer_callback(&gChester, &initPixelData);
    register_gpu_render_callback(&gChester, &renderCb);
    register_serial_callback(&gChester, NULL);

    button_a = false;
    button_b = false;
    button_start = false;
    button_select = false;
    button_up = false;
    button_down = false;
    button_left = false;
    button_right = false;

    const char *nativeRomPath = env->GetStringUTFChars(romPath, 0);
    const char *nativeSavePath = env->GetStringUTFChars(savePath, 0);

    const bool ret = init(&gChester, nativeRomPath, nativeSavePath, NULL);

    env->ReleaseStringUTFChars(romPath, nativeRomPath);
    env->ReleaseStringUTFChars(savePath, nativeSavePath);

    jclass cls = env->FindClass("com/chester/chesterapp/MainActivity");
    renderCallbackMethodId = env->GetStaticMethodID(cls, "renderCallback", "(Ljava/nio/ByteBuffer;)V");
    mainActivityClass = (jclass)env->NewGlobalRef(cls);

    return static_cast<jboolean>(ret);
}

JNIEXPORT void JNICALL
Java_com_chester_chesterapp_MainActivity_uninitChester(
        JNIEnv *env,
        jobject /* this */) {
    uninit(&gChester);

    if (mainActivityClass) {
        env->DeleteGlobalRef(mainActivityClass);
        mainActivityClass = NULL;
    }
}

JNIEXPORT void JNICALL
Java_com_chester_chesterapp_MainActivity_saveChester(
        JNIEnv *env,
        jobject /* this */) {
    save_if_needed(&gChester);
}

JNIEXPORT jint JNICALL
Java_com_chester_chesterapp_MainActivity_runChester(
        JNIEnv *env,
        jobject /* this */) {
    return run(&gChester);
}

JNIEXPORT void JNICALL
Java_com_chester_chesterapp_MainActivity_setKeyA(
        JNIEnv *env,
        jobject /* this */,
        jboolean pressed
) {
    button_a = pressed;
}

JNIEXPORT void JNICALL
Java_com_chester_chesterapp_MainActivity_setKeyB(
        JNIEnv *env,
        jobject /* this */,
        jboolean pressed
) {
    button_b = pressed;
}

JNIEXPORT void JNICALL
Java_com_chester_chesterapp_MainActivity_setKeyStart(
        JNIEnv *env,
        jobject /* this */,
        jboolean pressed
) {

    button_start = pressed;
}

JNIEXPORT void JNICALL
Java_com_chester_chesterapp_MainActivity_setKeySelect(
        JNIEnv *env,
        jobject /* this */,
        jboolean pressed
) {
    button_select = pressed;
}

JNIEXPORT void JNICALL
Java_com_chester_chesterapp_MainActivity_setKeyUp(
        JNIEnv *env,
        jobject /* this */,
        jboolean pressed
) {
    button_up = pressed;
}

JNIEXPORT void JNICALL
Java_com_chester_chesterapp_MainActivity_setKeyDown(
        JNIEnv *env,
        jobject /* this */,
        jboolean pressed
) {
    button_down = pressed;
}

JNIEXPORT void JNICALL
Java_com_chester_chesterapp_MainActivity_setKeyLeft(
        JNIEnv *env,
        jobject /* this */,
        jboolean pressed
) {
    button_left = pressed;
}

JNIEXPORT void JNICALL
Java_com_chester_chesterapp_MainActivity_setKeyRight(
        JNIEnv *env,
        jobject /* this */,
        jboolean pressed
) {
    button_right = pressed;
}

JNIEXPORT void JNICALL
Java_com_chester_chesterapp_MainActivity_toggleColorCorrection(
        JNIEnv *env,
        jobject /* this */
) {
    set_color_correction(&gChester, !get_color_correction(&gChester));
}
}
