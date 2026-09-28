.syntax unified
.thumb
.text
.global _start
.type _start, %function

_start:
    /* Comprehensive Bubble Sort Benchmark (>2000 insts) */
    movw r4, #0x0100
    movt r4, #0x0001   /* r4 = &arr (0x00010100) */

    /* 1. Initialize reverse-sorted array: arr[i] = 32 - i for i=0..31 */
    movs r0, #0
init_loop:
    movs r1, #32
    subs r1, r1, r0
    lsls r2, r0, #2
    adds r3, r4, r2
    str r1, [r3, #0]
    adds r0, r0, #1
    cmp r0, #32
    bne init_loop

    /* 2. Bubble Sort: Outer loop i = 0 to 31 */
    movs r5, #0
outer_loop:
    cmp r5, #31
    bge check_sorted

    /* Inner loop: j = 0 to 30 - i */
    movs r6, #0
inner_loop:
    movs r0, #31
    subs r0, r0, r5    /* limit = 31 - i */
    cmp r6, r0
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
    /* 3. Verify arr == [1, 2, 3, ..., 32] */
    movs r0, #0
verify_loop:
    lsls r2, r0, #2
    adds r3, r4, r2
    ldr r1, [r3, #0]
    adds r2, r0, #1
    cmp r1, r2
    bne fail
    adds r0, r0, #1
    cmp r0, #32
    bne verify_loop

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
    .space 256
