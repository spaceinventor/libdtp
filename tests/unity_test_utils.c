#include <stdio.h>
#include "unity.h"
#include "unity_test_utils.h"

int unity_test_util_main(int argc, const char *argv[], const char *__file__) {
    extern struct TestMeta *__start_test_registry;
    extern struct TestMeta *__stop_test_registry;
    struct TestMeta **start = &__start_test_registry;
    struct TestMeta **stop = &__stop_test_registry;
    if(argc > 1) {
        while (start != stop) {
            printf("file: %s, name \"%s\", line: %d\n", (*start)->file, (*start)->name, (*start)->line);
            start++;
        }
        return 0;
    } else {
        UnityBegin(__file__);
        while (start != stop) {
            UnityDefaultTestRun((*start)->test, (*start)->name, (*start)->line);
            start++;
        }
        return UnityEnd();
    }
}
