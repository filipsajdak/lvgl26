/* Only for fuzzing with AFL++'s GCC plugin on macOS.
 *
 * Code instrumented by the plugin keeps the previous location in the thread-local
 * __afl_prev_loc. GCC on macOS emulates thread-local storage, and Homebrew's AFL++
 * runtime (built with Clang) defines the variable with native thread-local storage,
 * so the two do not link. This file defines it the way GCC expects. */
__thread unsigned int __afl_prev_loc;
