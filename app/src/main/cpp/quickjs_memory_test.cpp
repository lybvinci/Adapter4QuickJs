//
// Created by 李岩波 on 2019-09-02.
//

#include "quickjs_memory_test.h"
#include <string>
#include "quickjs_test.h"

extern "C" {
#include "../quickjs/quickjs-libc.h"
}

static JSValue log(JSContext *ctx, JSValueConst this_val,
                       int argc, JSValueConst *argv) {
  for (int i = 0; i < argc; i++) {
    const char* str = JS_ToCString(ctx, argv[i]);
    printf("testlog: %s", str);
    JS_FreeCString(ctx, str);
  }
  return JS_UNDEFINED;
}

void QuickMemoryTest::prepare() {
  rt_ = JS_NewRuntime();
  if (!rt_) {
    fprintf(stderr, "qjs: cannot allocate JS runtime\n");
    __android_log_print(ANDROID_LOG_ERROR, "lyb", "qjs: cannot allocate JS runtime");
    exit(2);
  }
  ctx_ = JS_NewContext(rt_);
  if (!ctx_) {
    fprintf(stderr, "qjs: cannot allocate JS context\n");
    __android_log_print(ANDROID_LOG_ERROR, "lyb", "qjs: cannot allocate JS context");
    exit(2);
  }
  JS_SetMaxStackSize(ctx_, (size_t)ULLONG_MAX);

  js_std_add_helpers(ctx_, 0, 0);

  /* system modules */
  js_init_module_std(ctx_, "std");
  js_init_module_os(ctx_, "os");

  // add obj to global function.
  JSValue global_obj, testconsole;
  global_obj = JS_GetGlobalObject(ctx_);
  testconsole = JS_NewObject(ctx_);
  JS_SetPropertyStr(ctx_, testconsole, "log",
                    JS_NewCFunction(ctx_, log, "testlog1", 1));
  JS_SetPropertyStr(ctx_, global_obj, "console", testconsole);
  JS_FreeValue(ctx_, global_obj);

  /* make 'std' and 'os' visible to non module code */
  const char *str = "import * as std from 'std';\n"
                    "std.global.std = std;\n import * as os from 'os';\n std.global.os = os;\n";
  eval_buf(ctx_, str, strlen(str), "<input>", JS_EVAL_TYPE_MODULE);
}

void QuickMemoryTest::run(const char *testScript) {
  eval_buf(ctx_, testScript, strlen(testScript), "<input>", JS_EVAL_TYPE_GLOBAL);
  js_std_loop(ctx_);
}

void QuickMemoryTest::destroy() {
  js_std_free_handlers(rt_);
  JS_FreeContext(ctx_);
  JS_FreeRuntime(rt_);
}
