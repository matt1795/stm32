//#include "stm32l0xx.h"
#include "clock.hpp"
#include "config.hpp"

#include <algorithm>
#include <cstdint>

/* This variable is updated in three ways:
    1) by calling CMSIS function SystemCoreClockUpdate() 2) by calling HAL API
    function HAL_RCC_GetHCLKFreq() 3) each time HAL_RCC_ClockConfig() is called
    to configure the system clock frequency Note: If you use this function to
    configure the system clock; then there is no need to call the 2 first
    functions listed above, since SystemCoreClock variable is updated
    automatically.
*/

extern void (*_spreinit_array[])(void) __attribute__((weak));
extern void (*_epreinit_array[])(void) __attribute__((weak));
extern void (*_sinit_array[])(void) __attribute__((weak));
extern void (*_einit_array[])(void) __attribute__((weak));

extern "C" {
inline void static_initializers() {
    // Call C++ static initializers.
    // ('preinit_array' functions are unlikely if the user
    //  doesn't define any, I think. But check for them anyways.)
    int cpp_count = 0;
    int cpp_size = &(_epreinit_array[0]) - &(_spreinit_array[0]);
    for (cpp_count = 0; cpp_count < cpp_size; ++cpp_count) {
        _spreinit_array[cpp_count]();
    }
    // ('init_array' sections call static constructors)
    cpp_size = &(_einit_array[0]) - &(_sinit_array[0]);
    for (cpp_count = 0; cpp_count < cpp_size; ++cpp_count) {
        _sinit_array[cpp_count]();
    }
}

void system_init() {
    Mcu::clock_init();
    static_initializers();
}
}
