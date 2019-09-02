#include <jni.h>
#include <string>
#include <android/log.h>
#include <chrono>

/* enable test262 thread support to test SharedArrayBuffer and Atomics */
#define CONFIG_AGENT
int test_count = 0;
int test_failed = 0;
int async_done = 0;
bool outfile = true;
extern "C" {
#include "./quickjs/quickjs.h"
#include "./quickjs/quickjs-libc.h"
#include "./quickjs/cutils.h"
#include "./quickjs/list.h"
}

#define CMD_NAME "run-test262"

JSRuntime* rt_;
JSContext* ctx_;

void warning(const char *, ...) __attribute__((__format__(__printf__, 1, 2)));
void fatal(int, const char *, ...) __attribute__((__format__(__printf__, 2, 3)));

void warning(const char *fmt, ...)
{
  va_list ap;

  fflush(stdout);
  fprintf(stderr, "%s: ", CMD_NAME);
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
}

void fatal(int errcode, const char *fmt, ...)
{
  va_list ap;

  fflush(stdout);
  fprintf(stderr, "%s: ", CMD_NAME);
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
  exit(errcode);
}

#ifdef CONFIG_AGENT

#include <pthread.h>
#include <unistd.h>

typedef struct {
  struct list_head link;
  pthread_t tid;
  char *script;
  JSValue broadcast_func;
  BOOL broadcast_pending;
  JSValue broadcast_sab; /* in the main context */
  uint8_t *broadcast_sab_buf;
  size_t broadcast_sab_size;
  int32_t broadcast_val;
} Test262Agent;

typedef struct {
  struct list_head link;
  char *str;
} AgentReport;

static JSValue add_helpers1(JSContext *ctx);
static void add_helpers(JSContext *ctx);

static pthread_mutex_t agent_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t agent_cond = PTHREAD_COND_INITIALIZER;
/* list of Test262Agent.link */
static struct list_head agent_list = LIST_HEAD_INIT(agent_list);

static pthread_mutex_t report_mutex = PTHREAD_MUTEX_INITIALIZER;
/* list of AgentReport.link */
static struct list_head report_list = LIST_HEAD_INIT(report_list);

static void *agent_start(void *arg)
{
  Test262Agent *agent = static_cast<Test262Agent *>(arg);
  JSRuntime *rt;
  JSContext *ctx;
  JSValue ret_val;
  int ret;

  rt = JS_NewRuntime();
  if (rt == NULL) {
    fatal(1, "JS_NewRuntime failure");
  }
  ctx = JS_NewContext(rt);
  if (ctx == NULL) {
    JS_FreeRuntime(rt);
    fatal(1, "JS_NewContext failure");
  }
  JS_SetContextOpaque(ctx, agent);
  JS_SetRuntimeInfo(rt, "agent");
  JS_SetCanBlock(rt, TRUE);

  add_helpers(ctx);
  ret_val = JS_Eval(ctx, agent->script, strlen(agent->script),
                    "<evalScript>", JS_EVAL_TYPE_GLOBAL);
  free(agent->script);
  agent->script = NULL;
  if (JS_IsException(ret_val))
    js_std_dump_error(ctx);
  JS_FreeValue(ctx, ret_val);

  for(;;) {
    JSContext *ctx1;
    ret = JS_ExecutePendingJob(JS_GetRuntime(ctx), &ctx1);
    if (ret < 0) {
      js_std_dump_error(ctx);
      break;
    } else if (ret == 0) {
      if (JS_IsUndefined(agent->broadcast_func)) {
        break;
      } else {
        JSValue args[2];

        pthread_mutex_lock(&agent_mutex);
        while (!agent->broadcast_pending) {
          pthread_cond_wait(&agent_cond, &agent_mutex);
        }

        agent->broadcast_pending = FALSE;
        pthread_cond_signal(&agent_cond);

        pthread_mutex_unlock(&agent_mutex);

        args[0] = JS_NewArrayBuffer(ctx, agent->broadcast_sab_buf,
                                    agent->broadcast_sab_size,
                                    NULL, NULL, TRUE);
        args[1] = JS_NewInt32(ctx, agent->broadcast_val);
        ret_val = JS_Call(ctx, agent->broadcast_func, JS_UNDEFINED,
                          2, (JSValueConst *)args);
        JS_FreeValue(ctx, args[0]);
        JS_FreeValue(ctx, args[1]);
        if (JS_IsException(ret_val))
          js_std_dump_error(ctx);
        JS_FreeValue(ctx, ret_val);
        JS_FreeValue(ctx, agent->broadcast_func);
        agent->broadcast_func = JS_UNDEFINED;
      }
    }
  }
  JS_FreeValue(ctx, agent->broadcast_func);

  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);
  return NULL;
}

static JSValue js_agent_start(JSContext *ctx, JSValue this_val,
                              int argc, JSValue *argv)
{
  const char *script;
  Test262Agent *agent;

  if (JS_GetContextOpaque(ctx) != NULL)
    return JS_ThrowTypeError(ctx, "cannot be called inside an agent");

  script = JS_ToCString(ctx, argv[0]);
  if (!script)
    return JS_EXCEPTION;
  agent = static_cast<Test262Agent *>(malloc(sizeof(*agent)));
  memset(agent, 0, sizeof(*agent));
  agent->broadcast_func = JS_UNDEFINED;
  agent->broadcast_sab = JS_UNDEFINED;
  agent->script = strdup(script);
  JS_FreeCString(ctx, script);
  list_add_tail(&agent->link, &agent_list);
  pthread_create(&agent->tid, NULL, agent_start, agent);
  return JS_UNDEFINED;
}

static void js_agent_free(JSContext *ctx)
{
  struct list_head *el, *el1;
  Test262Agent *agent;

  list_for_each_safe(el, el1, &agent_list) {
    agent = list_entry(el, Test262Agent, link);
    pthread_join(agent->tid, NULL);
    JS_FreeValue(ctx, agent->broadcast_sab);
    list_del(&agent->link);
    free(agent);
  }
}

static JSValue js_agent_leaving(JSContext *ctx, JSValue this_val,
                                int argc, JSValue *argv)
{
  Test262Agent *agent = static_cast<Test262Agent *>(JS_GetContextOpaque(ctx));
  if (!agent)
    return JS_ThrowTypeError(ctx, "must be called inside an agent");
  /* nothing to do */
  return JS_UNDEFINED;
}

static BOOL is_broadcast_pending(void)
{
  struct list_head *el;
  Test262Agent *agent;
  list_for_each(el, &agent_list) {
    agent = list_entry(el, Test262Agent, link);
    if (agent->broadcast_pending)
      return TRUE;
  }
  return FALSE;
}

static JSValue js_agent_broadcast(JSContext *ctx, JSValue this_val,
                                  int argc, JSValue *argv)
{
  JSValueConst sab = argv[0];
  struct list_head *el;
  Test262Agent *agent;
  uint8_t *buf;
  size_t buf_size;
  int32_t val;

  if (JS_GetContextOpaque(ctx) != NULL)
    return JS_ThrowTypeError(ctx, "cannot be called inside an agent");

  buf = JS_GetArrayBuffer(ctx, &buf_size, sab);
  if (!buf)
    return JS_EXCEPTION;
  if (JS_ToInt32(ctx, &val, argv[1]))
    return JS_EXCEPTION;

  /* broadcast the values and wait until all agents have started
     calling their callbacks */
  pthread_mutex_lock(&agent_mutex);
  list_for_each(el, &agent_list) {
    agent = list_entry(el, Test262Agent, link);
    agent->broadcast_pending = TRUE;
    /* the shared array buffer is used by the thread, so increment
       its refcount */
    agent->broadcast_sab = JS_DupValue(ctx, sab);
    agent->broadcast_sab_buf = buf;
    agent->broadcast_sab_size = buf_size;
    agent->broadcast_val = val;
  }
  pthread_cond_broadcast(&agent_cond);

  while (is_broadcast_pending()) {
    pthread_cond_wait(&agent_cond, &agent_mutex);
  }
  pthread_mutex_unlock(&agent_mutex);
  return JS_UNDEFINED;
}

static JSValue js_agent_receiveBroadcast(JSContext *ctx, JSValue this_val,
                                         int argc, JSValue *argv)
{
  Test262Agent *agent = static_cast<Test262Agent *>(JS_GetContextOpaque(ctx));
  if (!agent)
    return JS_ThrowTypeError(ctx, "must be called inside an agent");
  if (!JS_IsFunction(ctx, argv[0]))
    return JS_ThrowTypeError(ctx, "expecting function");
  JS_FreeValue(ctx, agent->broadcast_func);
  agent->broadcast_func = JS_DupValue(ctx, argv[0]);
  return JS_UNDEFINED;
}

static JSValue js_agent_sleep(JSContext *ctx, JSValue this_val,
                              int argc, JSValue *argv)
{
  uint32_t duration;
  if (JS_ToUint32(ctx, &duration, argv[0]))
    return JS_EXCEPTION;
  usleep(duration * 1000);
  return JS_UNDEFINED;
}

static int64_t get_clock_ms(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000 + (ts.tv_nsec / 1000000);
}

static JSValue js_agent_monotonicNow(JSContext *ctx, JSValue this_val,
                                     int argc, JSValue *argv)
{
  return JS_NewInt64(ctx, get_clock_ms());
}

static JSValue js_agent_getReport(JSContext *ctx, JSValue this_val,
                                  int argc, JSValue *argv)
{
  AgentReport *rep;
  JSValue ret;

  pthread_mutex_lock(&report_mutex);
  if (list_empty(&report_list)) {
    rep = NULL;
  } else {
    rep = list_entry(report_list.next, AgentReport, link);
    list_del(&rep->link);
  }
  pthread_mutex_unlock(&report_mutex);
  if (rep) {
    ret = JS_NewString(ctx, rep->str);
    free(rep->str);
    free(rep);
  } else {
    ret = JS_NULL;
  }
  return ret;
}

static JSValue js_agent_report(JSContext *ctx, JSValue this_val,
                               int argc, JSValue *argv)
{
  const char *str;
  AgentReport *rep;

  str = JS_ToCString(ctx, argv[0]);
  if (!str)
    return JS_EXCEPTION;
  rep = static_cast<AgentReport *>(malloc(sizeof(*rep)));
  rep->str = strdup(str);
  JS_FreeCString(ctx, str);

  pthread_mutex_lock(&report_mutex);
  list_add_tail(&rep->link, &report_list);
  pthread_mutex_unlock(&report_mutex);
  return JS_UNDEFINED;
}

static const JSCFunctionListEntry js_agent_funcs[] = {
        /* only in main */
        JS_CFUNC_DEF("start", 1, js_agent_start ),
        JS_CFUNC_DEF("getReport", 0, js_agent_getReport ),
        JS_CFUNC_DEF("broadcast", 2, js_agent_broadcast ),
        /* only in agent */
        JS_CFUNC_DEF("report", 1, js_agent_report ),
        JS_CFUNC_DEF("leaving", 0, js_agent_leaving ),
        JS_CFUNC_DEF("receiveBroadcast", 1, js_agent_receiveBroadcast ),
        /* in both */
        JS_CFUNC_DEF("sleep", 1, js_agent_sleep ),
        JS_CFUNC_DEF("monotonicNow", 0, js_agent_monotonicNow ),
};

static JSValue js_new_agent(JSContext *ctx)
{
  JSValue agent;
  agent = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, agent, js_agent_funcs,
                             countof(js_agent_funcs));
  return agent;
}
#endif

static JSValue js_print(JSContext *ctx, JSValueConst this_val,
                        int argc, JSValueConst *argv)
{
  int i;
  const char *str;

  if (outfile) {
    for (i = 0; i < argc; i++) {
      if (i != 0)
//        fputc(' ', outfile);
      str = JS_ToCString(ctx, argv[i]);
      if (!str)
        return JS_EXCEPTION;
      if (!strcmp(str, "Test262:AsyncTestComplete"))
        async_done++;
//      fputs(str, outfile);
      printf("js_print:%s", str);
      JS_FreeCString(ctx, str);
    }
//    fputc('\n', outfile);
  }
  return JS_UNDEFINED;
}

static JSValue js_detachArrayBuffer(JSContext *ctx, JSValue this_val,
                                    int argc, JSValue *argv)
{
  JS_DetachArrayBuffer(ctx, argv[0]);
  return JS_UNDEFINED;
}

static JSValue js_evalScript(JSContext *ctx, JSValue this_val,
                             int argc, JSValue *argv)
{
  const char *str;
  size_t len;
  JSValue ret;
  str = JS_ToCStringLen(ctx, &len, argv[0]);
  if (!str)
    return JS_EXCEPTION;
  ret = JS_Eval(ctx, str, len, "<evalScript>", JS_EVAL_TYPE_GLOBAL);
  JS_FreeCString(ctx, str);
  return ret;
}

static JSValue add_helpers1(JSContext *ctx)
{
  JSValue global_obj;
  JSValue obj262;

  global_obj = JS_GetGlobalObject(ctx);

  JS_SetPropertyStr(ctx, global_obj, "print",
                    JS_NewCFunction(ctx, js_print, "print", 1));

  /* add it in the engine once the proposal is accepted */
  JS_DefinePropertyValueStr(ctx, global_obj, "globalThis",
                            JS_DupValue(ctx, global_obj),
                            JS_PROP_CONFIGURABLE | JS_PROP_WRITABLE);

  /* $262 special object used by the tests */
  obj262 = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, obj262, "detachArrayBuffer",
                    JS_NewCFunction(ctx, js_detachArrayBuffer,
                                    "detachArrayBuffer", 1));
  JS_SetPropertyStr(ctx, obj262, "evalScript",
                    JS_NewCFunction(ctx, js_evalScript,
                                    "evalScript", 1));
  JS_SetPropertyStr(ctx, obj262, "codePointRange",
                    JS_NewCFunction(ctx, js_string_codePointRange,
                                    "codePointRange", 2));
#ifdef CONFIG_AGENT
  JS_SetPropertyStr(ctx, obj262, "agent", js_new_agent(ctx));
#endif

  JS_SetPropertyStr(ctx, obj262, "global",
                    JS_DupValue(ctx, global_obj));

#ifdef CONFIG_REALM
  JS_SetPropertyStr(ctx, obj262, "createRealm",
                      JS_NewCFunction(ctx, js_createRealm,
                                      "createRealm", 0));
#endif

  JS_SetPropertyStr(ctx, global_obj, "$262", JS_DupValue(ctx, obj262));

  JS_FreeValue(ctx, global_obj);
  return obj262;
}

static void add_helpers(JSContext *ctx)
{
  JS_FreeValue(ctx, add_helpers1(ctx));
}

int eval_buf(JSContext *ctx, const char *buf, int buf_len,
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
  __android_log_print(ANDROID_LOG_ERROR, "lyb","eval_buf=%d", ret);
  JS_FreeValue(ctx, val);
  return ret;
}

bool dump_memory = false;

static int run_test_buf(JSRuntime* rt, JSContext* ctx, const char *filename, char *harness,
                        const char *buf, size_t buf_len, const char* error_type,
                        int eval_flags, BOOL is_negative, BOOL is_async,
                        BOOL can_block)
{
  int ret;

  ret = eval_buf(ctx, buf, buf_len, filename, eval_flags);
  ret = (ret != 0);

  if (dump_memory) {
//    update_stats(rt, filename);
  }

  test_count++;
  if (ret) {
    test_failed++;
      /* do not output a failure number to minimize diff */
      fprintf(outfile, "  FAILED\n");
    printf("%s test failed, count=%d", filename, test_failed);
  } else {
    printf("%s test success, count=%d", filename, test_count);
  }
  return ret;
}

int run_test(const char *filename, const char* buf, size_t buf_len)
{
  char *harness;
  char *error_type;
  int ret, eval_flags, use_strict, use_nostrict;
  BOOL is_negative, is_nostrict, is_onlystrict, is_async, is_module, skip;
  BOOL can_block;

  is_nostrict = is_onlystrict = is_negative = is_async = is_module = skip = FALSE;
  can_block = TRUE;
  error_type = NULL;

//  is_module = true;
  if (is_module) {
    eval_flags = JS_EVAL_TYPE_MODULE;
  } else {
    eval_flags = JS_EVAL_TYPE_GLOBAL;
  }
  std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> start = std::chrono::high_resolution_clock::now();
  ret = run_test_buf(rt_, ctx_, filename, harness, buf, buf_len,
                     error_type, eval_flags, is_negative, is_async,
                     can_block);
//  if (use_nostrict) {
//
//  }
//  if (use_strict) {
//    ret |= run_test_buf(rt_, ctx_, filename, harness, buf, buf_len,
//                        error_type, eval_flags | JS_EVAL_FLAG_STRICT,
//                        is_negative, is_async, can_block);
//  }
//  js_std_loop(ctx_);
  auto finish = std::chrono::high_resolution_clock::now();
  double cost = std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count()
                / 1000000.0;
  fprintf(outfile, " time: %d ms\n", cost);
  free(error_type);

  return ret;
}



void initJSEngine() {
  rt_ = JS_NewRuntime();
  if (!rt_) {
    __android_log_print(ANDROID_LOG_ERROR, "LYNX", "qjs: cannot allocate JS runtime");
    exit(2);
  }
  ctx_ = JS_NewContext(rt_);
  if (!ctx_) {
    __android_log_print(ANDROID_LOG_ERROR, "LYNX", "qjs: cannot allocate JS context");
    exit(2);
  }

  JS_SetCanBlock(rt_, TRUE);
  JS_SetMaxStackSize(ctx_, (size_t)ULLONG_MAX);
//  js_std_add_helpers(ctx_, 0, 0);
  /* system modules */
//  js_init_module_std(ctx_, "std");
//  js_init_module_os(ctx_, "os");
  /* make 'std' and 'os' visible to non module code */
//  const char *str = "import * as std from 'std';\n"
//                    "std.global.std = std;\n import * as os from 'os';\n std.global.os = os;\n";
//  eval_buf(ctx_, str, strlen(str), "<input>", JS_EVAL_TYPE_MODULE);
//
//  const char* basic = "setTimeout = os.setTimeout; clearTimeout = os.clearTimeout;";
//  int ret = eval_buf(ctx_, basic, strlen(basic), "<basic>", JS_EVAL_TYPE_GLOBAL);

  add_helpers(ctx_);

  JS_EnableIsErrorProperty(ctx_, TRUE);
}

void destroy() {
#ifdef CONFIG_AGENT
  js_agent_free(ctx_);
#endif
  js_std_free_handlers(rt_);
  JS_FreeContext(ctx_);
  JS_FreeRuntime(rt_);
}

int run_test_clean(const char *filename, const char* buf, size_t buf_len)
{
  // init
  JSRuntime* rt = JS_NewRuntime();
  if (!rt) {
    __android_log_print(ANDROID_LOG_ERROR, "LYNX", "qjs: cannot allocate JS runtime");
    exit(2);
  }
  JSContext* ctx = JS_NewContext(rt);
  if (!ctx) {
    __android_log_print(ANDROID_LOG_ERROR, "LYNX", "qjs: cannot allocate JS context");
    exit(2);
  }

//  JS_SetCanBlock(rt, TRUE);
  JS_SetMaxStackSize(ctx, (size_t)ULLONG_MAX);

  js_std_add_helpers(ctx, 0, 0);
  /* system modules */
  js_init_module_std(ctx, "std");
  js_init_module_os(ctx, "os");
  /* make 'std' and 'os' visible to non module code */
  const char *str = "import * as std from 'std';\n"
                    "std.global.std = std;\n import * as os from 'os';\n std.global.os = os;\n";
  eval_buf(ctx, str, strlen(str), "<input>", JS_EVAL_TYPE_MODULE);

  const char* basic = "setTimeout = os.setTimeout; clearTimeout = os.clearTimeout; window = std.global;\n";
  if (eval_buf(ctx, basic, strlen(basic), "<basic>", JS_EVAL_TYPE_GLOBAL) != 0) {
    __android_log_print(ANDROID_LOG_ERROR, "LYNX", "window inject failed.");
  }


  add_helpers(ctx);
//  JS_EnableIsErrorProperty(ctx, TRUE);


  // execute
  char *harness;
  char *error_type;
  int ret, eval_flags;
  BOOL is_negative, is_async, is_module;
  BOOL can_block;

  is_negative = is_async = is_module = FALSE;
  can_block = TRUE;
  error_type = NULL;

//  is_module = true;
  if (is_module) {
    eval_flags = JS_EVAL_TYPE_MODULE;
  } else {
    eval_flags = JS_EVAL_TYPE_GLOBAL;
  }
  std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> start = std::chrono::high_resolution_clock::now();
  ret = run_test_buf(rt, ctx, filename, harness, buf, buf_len,
                     error_type, eval_flags, is_negative, is_async,
                     can_block);
//  js_std_loop(ctx_);
  auto finish = std::chrono::high_resolution_clock::now();
  double cost = std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count()
                / 1000000.0;
  fprintf(outfile, " time: %d ms\n", cost);
  free(error_type);
  js_std_loop(ctx);

#ifdef CONFIG_AGENT
  js_agent_free(ctx);
#endif
  js_std_free_handlers(rt);
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);

  return ret;
}



