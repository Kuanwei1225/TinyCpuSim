.syntax unified
.thumb
.text
.global _start
.type _start, %function

_start:
    /* Set up stack */
    ldr r0, =stack_top
    mov sp, r0

    /* Initialize array of 4 words in memory: [40, 10, 30, 20] */
    ldr r4, =arr

    movs r0, #40
    str r0, [r4, #0]
    movs r0, #10
    str r0, [r4, #4]
    movs r0, #30
    str r0, [r4, #8]
    movs r0, #20
    str r0, [r4, #12]

    /* Simple Bubble Sort on 4 elements */
    /* Outer loop: i = 0 to 3 */
    movs r5, #0
outer_loop:
    cmp r5, #3
    bge check_sorted

    /* Inner loop: j = 0 to 2 */
    movs r6, #0
inner_loop:
    cmp r6, #3
    bge next_outer

    /* Calculate address = r4 + (r6 * 4) */
    lsls r7, r6, #2
    adds r7, r4, r7    /* r7 = &arr[j] */

    ldr r0, [r7, #0]   /* r0 = arr[j] */
    ldr r1, [r7, #4]   /* r1 = arr[j+1] */

    cmp r0, r1
    ble no_swap

    /* Swap arr[j] and arr[j+1] */
    str r1, [r7, #0]
    str r0, [r7, #4]

no_swap:
    adds r6, r6, #1
    b inner_loop

next_outer:
    adds r5, r5, #1
    b outer_loop

check_sorted:
    /* Verify arr == [10, 20, 30, 40] */
    ldr r0, [r4, #0]
    cmp r0, #10
    bne fail

    ldr r0, [r4, #4]
    cmp r0, #20
    bne fail

    ldr r0, [r4, #8]
    cmp r0, #30
    bne fail

    ldr r0, [r4, #12]
    cmp r0, #40
    bne fail

    /* Success: exit code 0 */
    movs r0, #0
    movs r7, #1
    svc #0

fail:
    movs r0, #1
    movs r7, #1
    svc #0

.bss
.align 4
arr:
    .space 64
stack_mem:
    .space 4096
stack_top:
