#include <jni.h>
#include <string>
#include <android/log.h>
#define printf(...) __android_log_print(ANDROID_LOG_DEBUG, "LYNX", __VA_ARGS__);
#define fprintf(iotype, ...) __android_log_print(ANDROID_LOG_ERROR, "LYNX", __VA_ARGS__);
extern "C" {
#include "./quickjs/quickjs.h"
#include "./quickjs/quickjs-libc.h"
#include "./quickjs/cutils.h"
}

#include "quickjs_test.h"

static JSValue testlog(JSContext *ctx, JSValueConst this_val,
                       int argc, JSValueConst *argv) {
    __android_log_print(ANDROID_LOG_ERROR, "lyb123456", "testlog");
    return JS_UNDEFINED;
}

//
int testScript() {

//  for (int i = 0; i < INT_MAX; ++i) {
//    int* aa = (int*) malloc(sizeof(int) * 3);
//    if (((intptr_t )aa) > 0) {
////      __android_log_print(ANDROID_LOG_ERROR, "lyb123", "1");
//    }else {
//      __android_log_print(ANDROID_LOG_ERROR, "lyb123", "0");
//    }
//  }
    JSRuntime *rt;
    JSContext *ctx;
//    char *expr = "var test=111;testlogobj.testlog(1);";
  //const char* expr = "var t = '*WARN*,*ERROR*'.split(/[\\s,]+/); console.log(t[0]);";
//  const char* expr = "Promise.resolve().then(() => testlogobj.testlog(1))";
  const char* expr = "this.setTimeout = os.setTimeout; testlogobj.testlog(1); setTimeout(function () { testlogobj.testlog(1);}, 1000);Promise.resolve().then(() => testlogobj.testlog(1));";

    rt = JS_NewRuntime();
    if (!rt) {
        fprintf(stderr, "qjs: cannot allocate JS runtime\n");
        __android_log_print(ANDROID_LOG_ERROR, "lyb", "qjs: cannot allocate JS runtime");
        exit(2);
    }
    ctx = JS_NewContext(rt);
    if (!ctx) {
        fprintf(stderr, "qjs: cannot allocate JS context\n");
        __android_log_print(ANDROID_LOG_ERROR, "lyb", "qjs: cannot allocate JS context");
        exit(2);
    }

    /* loader for ES6 modules */
//    JS_SetModuleLoaderFunc(rt, NULL, js_module_loader, NULL);

    js_std_add_helpers(ctx, 0, 0);


    // add obj to global function.
    JSValue global_obj, testconsole;
    global_obj = JS_GetGlobalObject(ctx);
    testconsole = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, testconsole, "testlog",
                      JS_NewCFunction(ctx, testlog, "testlog1", 1));
    JS_SetPropertyStr(ctx, global_obj, "testlogobj", testconsole);
    JS_FreeValue(ctx, global_obj);


//    const JSClassDef js_class_def = {
//            "classDef",
//    };
//    JSClassID jid;
//    JS_NewClassID(&jid);
//  __android_log_print(ANDROID_LOG_ERROR, "ttt", "jid = %d", jid);
//    JS_NewClass(JS_GetRuntime(ctx), jid, &js_class_def);
//    JSValue obj = JS_NewObjectClass(ctx, jid);
//    JS_FreeValue(ctx, obj);


    /* system modules */
    js_init_module_std(ctx, "std");
    js_init_module_os(ctx, "os");

    /* make 'std' and 'os' visible to non module code */
    const char *str = "import * as std from 'std';\n"
                      "std.global.std = std;\n import * as os from 'os';\n std.global.os = os;\n";
    eval_buf(ctx, str, strlen(str), "<input>", JS_EVAL_TYPE_MODULE);

    if (eval_buf(ctx, expr, strlen(expr), "<cmdline>", 0))
        goto fail;

    js_std_loop(ctx);

    js_std_free_handlers(rt);
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    return 0;
    fail:
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);

    return 1;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_lybvinci_adapter4quickjs_MainActivity_nativeInitJSEngine(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    initJSEngine();
    return env->NewStringUTF(hello.c_str());
}

extern "C" JNIEXPORT jint JNICALL
Java_com_lybvinci_adapter4quickjs_MainActivity_nativeRunTest(
        JNIEnv *env,
        jobject /* this */,
        jstring jsData,
        jstring fileName) {
//  const char* data = (char *)env->GetByteArrayElements(jsData, JNI_FALSE);
  const char* data = (char *)env->GetStringUTFChars(jsData, JNI_FALSE);
  const char* url = (char *)env->GetStringUTFChars(fileName, JNI_FALSE);
//  size_t len = env->GetArrayLength(jsData);
//  std::string stdstr(data);
//  printf("execute data = %s", data);
  return run_test(url, data, strlen(data));
}

extern "C" JNIEXPORT jint JNICALL
Java_com_lybvinci_adapter4quickjs_MainActivity_nativeRunCleanTest(
        JNIEnv *env,
        jobject /* this */,
        jstring jsData,
        jstring fileName) {
  const char* data = (char *)env->GetStringUTFChars(jsData, JNI_FALSE);
  const char* url = (char *)env->GetStringUTFChars(fileName, JNI_FALSE);
  return run_test_clean(url, data, strlen(data));
}

extern "C" JNIEXPORT void JNICALL
Java_com_lybvinci_adapter4quickjs_MainActivity_nativeDestroy(
        JNIEnv *env,
        jobject /* this */) {
  destroy();
}

extern "C" JNIEXPORT int JNICALL
Java_com_lybvinci_adapter4quickjs_MainActivity_nativeTestScript(
        JNIEnv *env,
        jobject /* this */) {
  return testScript();
}
