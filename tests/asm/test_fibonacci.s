.syntax unified
.thumb
.text
.global _start
.type _start, %function

_start:
    /* Set up stack pointer in RAM */
    movw r0, #0x0000
    movt r0, #0x0010
    mov sp, r0

    /* Compute Fibonacci(10) */
    movs r0, #10
    bl fib

    /* Fibonacci(10) should equal 55 */
    cmp r0, #55
    bne fail

    /* Success: exit code 0 */
    movs r0, #0
    svc #0

fail:
    movs r0, #1
    svc #0

/* int fib(int n) */
.type fib, %function
fib:
    push {r4, r5, lr}
    cmp r0, #0
    beq fib_zero
    cmp r0, #1
    beq fib_one

    mov r4, r0         /* r4 = n */
    subs r0, r4, #1    /* r0 = n - 1 */
    bl fib
    mov r5, r0         /* r5 = fib(n - 1) */

    subs r0, r4, #2    /* r0 = n - 2 */
    bl fib
    adds r0, r0, r5    /* r0 = fib(n - 2) + fib(n - 1) */

    pop {r4, r5, pc}

fib_zero:
    movs r0, #0
    pop {r4, r5, pc}

fib_one:
    movs r0, #1
    pop {r4, r5, pc}
