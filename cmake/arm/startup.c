/* Start-up code for QEMU's mps2-an386 board: the vector table, and a reset handler
   that copies the initialized data, clears the rest, runs the constructors and main. */
#include <stdint.h>

extern uint32_t __stack_top, __data_load, __data_start, __data_end, __bss_start, __bss_end;
extern void __libc_init_array(void);
extern void initialise_monitor_handles(void);
extern int main(void);
extern void exit(int);

/* What a global object's destructor is registered with. One program, so one value. */
void* __dso_handle = 0;

/* newlib calls these around the constructors; the files that define them are left out. */
void _init(void) {}
void _fini(void) {}

void reset(void) {
  uint32_t* from = &__data_load;
  for (uint32_t* to = &__data_start; to < &__data_end;) *to++ = *from++;
  for (uint32_t* to = &__bss_start; to < &__bss_end;) *to++ = 0;
  initialise_monitor_handles(); /* printf goes to the host through semihosting */
  __libc_init_array();
  exit(main());
}

static void fault(void) {
  for (;;) {
  }
}

__attribute__((section(".vectors"), used)) static const void* const vectors[] = {
    &__stack_top, reset, fault, fault, fault, fault, fault,
};
