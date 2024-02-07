#pragma once
#ifdef __cplusplus
extern "C"
{
#endif

/**
 * How the registered testd meta data is stored
 */
struct TestMeta {
    void (*test)(void); /** actual test case function */
    const char *name; /** Human or IDE friendly test name (must be a valid C function name though) */
    const char *file;  /** file containing the test */
    int line; /** start line in the file of the test */
 };

/**
 * Use this macro to declare a function as a test case
 * 
 * @param x valid C function name
 */
#define REGISTER_TEST(x) \
void x(void);\
static const struct TestMeta x ## _meta = { \
    .test = &x, \
    .name = #x, \
    .file = __FILE__, \
    .line = __LINE__ \
}; \
const struct TestMeta * x ## _meta_ptr __attribute__ ((section ("test_registry"))) = &x ## _meta; \
void x(void)

#define UNITY_UTIL_MAIN()  unity_test_util_main(argc, argv, __FILE__);

/**
 * Utility main function, will scan the "test_registry" ELF file section to discover tests automatically
 */
extern int unity_test_util_main(int argc, const char *argv[], const char *__file__);

#ifdef __cplusplus
}
#endif
