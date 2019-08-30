#ifndef QUICKJS_TEST_H
#define QUICKJS_TEST_H

void initJSEngine();
void destroy();
int run_test(const char *filename, const char* buf, size_t len);
int eval_buf(JSContext *ctx, const char *buf, int buf_len,
                    const char *filename, int eval_flags);
int run_test_clean(const char *filename, const char* buf, size_t buf_len);

#endif



