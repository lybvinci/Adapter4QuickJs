//
// Created by 李岩波 on 2019-09-02.
//

#ifndef ADAPTER4QUICKJS_QUICKJS_MEMORY_TEST_H
#define ADAPTER4QUICKJS_QUICKJS_MEMORY_TEST_H

extern "C" {
#include "./quickjs/quickjs.h"
};


class QuickMemoryTest {
public:
  void prepare();
  void run(const char* testScript);
  void destroy();

private:
  JSRuntime* rt_;
  JSContext* ctx_;

};


#endif //ADAPTER4QUICKJS_QUICKJS_MEMORY_TEST_H
