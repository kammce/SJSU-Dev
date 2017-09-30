    .syntax unified

    @ --------------------------------

@ char buff[5] = {0};
@ while(1)
@ {
@     for(int i = 0; i < 16; i++)
@     {
@         ctoba(buff, (char)i);
@         for(int j = 1; j < 5; j++)
@         {
@             LE.set((5-j), CHECK_BIT(i,j-1));
@         }
@         LD.setNumber(i);
@         printf("Hello Preet 0x%X\n", i);
@         delay_ms(1000);
@     }
@ }

#define counter     r12
#define ptr         r11
#define variable    r10
#define tmp         r9
#define FIO1DIR     0x2009C020
#define FIO1PIN     0x2009C034


.global main
main:
    @ Stack the return address (lr) in addition to a dummy register (ip) to
    @ keep the stack 8-byte aligned.
    push    {ip, lr}
    @ Load the argument and perform the call. This is like 'printf("...")' in C.
    ldr     r0, =intro_message
    bl      printf
    @ Clear counter
    mov     counter, #0x0
    @ Set Direction register for Port 1 for LED bits
    ldr     ptr, =FIO1DIR
    ldr     variable, [ptr]
    orr     variable, #0b100010011
    str     variable, [ptr]
    @ Set Pins to LEDOFF (1, active low) for port 0 LED BITS
    ldr     ptr, =FIO1PIN
    ldr     variable, [ptr]
    mov     tmp, #0b100010011
    orr     variable, tmp
    str     variable, [ptr]
loop:
    ldr     r0, =counter_message
    mov     r1, counter
    bl      printf
    @ Set 7-segment display to counter value
    @ An example of how to run a C++ class method by using an
    @ extern "C" function within a C++ file to run it
    push    {counter}
    mov     r0, counter
    bl      led_display_set
    pop     {counter}
    @ Increment counter register
    add     counter, counter, #0x1
    @ Wait for 1 second
    mov     r0, #1000
    bl      delay_ms
    @ Set pin LEDs
    ldr     ptr, =FIO1PIN
    ldr     variable, [ptr]
    @ Set LED 3 by checking counter_bit[3]
    mov     tmp, counter
    and     tmp, tmp, #0b1000
    cmp     tmp, #0b1000
    ite     eq
    biceq   variable, variable, #0b1
    orrne   variable, variable, #0b1
    @ Set LED 2 by checking counter_bit[2]
    mov     tmp, counter
    and     tmp, tmp, #0b100
    cmp     tmp, #0b100
    ite     eq
    biceq   variable, variable, #0b10
    orrne   variable, variable, #0b10
    @ Set LED 1 by checking counter_bit[1]
    mov     tmp, counter
    and     tmp, tmp, #0b10
    cmp     tmp, #0b10
    ite     eq
    biceq   variable, variable, #0b10000
    orrne   variable, variable, #0b10000
    @ Set LED 0 by checking counter_bit[0]
    mov     tmp, counter
    and     tmp, tmp, #0b1
    cmp     tmp, #0b1
    ite     eq
    biceq   variable, variable, #0b100000000
    orrne   variable, variable, #0b100000000
    @ Write port 0 pin register back into memory
    str     variable, [ptr]
    @ Check if counter is == 0xF, if so, set it to 0
    cmp     counter, #0x10
    bne     loop
    mov     counter, #0x0
    ldr     r0, =reset_counter_message
    bl      printf
    @ Continue to loop
    b       loop
    @ Exit from 'main'. This is like 'return 0' in C.
    mov     r0, #0    @ Return 0.
    @ Pop the dummy ip to reverse our alignment fix, and pop the original lr
    @ value directly into pc — the Program Counter — to return.
    pop     {ip, pc}

@ --------------------------------
@ Data for the printf calls. The GNU assembler's ".asciz" directive
@ automatically adds a NULL character termination.
intro_message:
    .asciz "Hello, World from Assembly Code World!\n"

counter_message:
    .asciz "Counter = 0x%X\n"

reset_counter_message:
    .asciz "Resetting Counter\n"

variable_debug_message:
    .asciz "Variable = 0x%X\n"