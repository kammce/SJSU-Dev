    .syntax unified
@ --------------------------------
@ printf("Hello, World from Assembly Code World!\n");
@
@ while(1)
@ {
@     for(int i = 0; i < 16; i++)
@     {
@         for(int j = 1; j < 5; j++)
@         {
@             LE.set((5-j), CHECK_BIT(i,j-1));
@         }
@         LD.setNumber(i);
@         printf("Counter = 0x%X\n", i);
@         delay_ms(1000);
@     }
@     printf("Resetting Counter\n");
@ }

#define counter     r12
#define ptr         r11
#define variable    r10
#define tmp         r9
#define FIO1DIR     0x2009C020
#define FIO1PIN     0x2009C034

.macro bit_map source:req, source_bit_mask:req, destination:req, dest_bit_mask:req
    @ tmp & 0b1000 to check if 3rd bit is set
    and     \source, \source, \source_bit_mask
    @ If tmp
    cmp     \source, #0
    @ is equal to 0
    ite     eq
    @ then set led bit HIGH/LEDOFF (active low)
    orreq   \destination, \destination, \dest_bit_mask
    @ else clear led bit LOW/LEDON
    bicne   \destination, \destination, \dest_bit_mask
.endm

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
    @ ========================================================
    @ Set 7-segment display to counter value
    @ An example of how to run a C++ class method by using an
    @ extern "C" function within a C++ file to run it
    @ ========================================================
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
    @ ========================================
    @ Set LED 3 by checking counter_bit[3]
    @ ========================================
    @ Set LED 2 by checking counter_bit[2]
    mov     tmp, counter
    bit_map tmp, #0b1000, variable, #0b1
    @ Set LED 2 by checking counter_bit[2]
    mov     tmp, counter
    bit_map tmp, #0b100, variable, #0b10
    @ Set LED 1 by checking counter_bit[1]
    mov     tmp, counter
    bit_map tmp, #0b10, variable, #0b10000
    @ Set LED 0 by checking counter_bit[0]
    mov     tmp, counter
    bit_map tmp, #0b1, variable, #0b100000000
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