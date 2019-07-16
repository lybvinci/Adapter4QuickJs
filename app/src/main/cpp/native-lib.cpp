#include <jni.h>
#include <string>
#include <android/log.h>
extern "C" {
#include <quickjs-libc.h>

#include "./quickjs/quickjs.h"
#include "./quickjs/quickjs-libc.h"
#include "./quickjs/cutils.h"

static JSValue testlog(JSContext *ctx, JSValueConst this_val,
                       int argc, JSValueConst *argv) {
    __android_log_print(ANDROID_LOG_ERROR, "lyb", "testlog");
    return JS_UNDEFINED;
}

static int eval_buf(JSContext *ctx, const char *buf, int buf_len,
                    const char *filename, int eval_flags) {
    JSValue val;
    int ret;

    val = JS_Eval(ctx, buf, buf_len, filename, eval_flags);
    if (JS_IsException(val)) {
        js_std_dump_error(ctx);
        ret = -1;
    } else {
        ret = 0;
    }
    JS_FreeValue(ctx, val);
    return ret;
}
//


int loadjs() {
    JSRuntime *rt;
    JSContext *ctx;
    char *expr = "var test=1111; console.log(\"lyb\", test); print(test);testlogobj.testlog(1);";

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
    JS_SetModuleLoaderFunc(rt, NULL, js_module_loader, NULL);

    js_std_add_helpers(ctx, 0, 0);


    // add obj to global function.
    JSValue global_obj, testconsole;
    global_obj = JS_GetGlobalObject(ctx);
    testconsole = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, testconsole, "testlog",
                      JS_NewCFunction(ctx, testlog, "testlog1", 1));
    JS_SetPropertyStr(ctx, global_obj, "testlogobj", testconsole);
    JS_FreeValue(ctx, global_obj);



    /* system modules */
//    js_init_module_std(ctx, "std");
//    js_init_module_os(ctx, "os");

    /* make 'std' and 'os' visible to non module code */
//    const char *str = "import * as std from 'std';\n"
//                      "std.global.std = std;\n";
//    eval_buf(ctx, str, strlen(str), "<input>", JS_EVAL_TYPE_MODULE);

    if (eval_buf(ctx, expr, strlen(expr), "<cmdline>", 0))
        goto fail;
    js_std_loop(ctx);

    js_std_free_handlers(rt);
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    return 0;
    fail:
    js_std_free_handlers(rt);
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    return 1;
}


}

extern "C" JNIEXPORT jstring JNICALL
Java_com_lybvinci_adapter4quickjs_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    loadjs();
    return env->NewStringUTF(hello.c_str());
}


