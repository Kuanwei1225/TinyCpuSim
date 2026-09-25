.syntax unified
.thumb
.text
.global _start
.type _start, %function

_start:
    /* Set up stack in RAM */
    movw r0, #0x0000
    movt r0, #0x0010
    mov sp, r0

    /* Initialize accumulators and loop counter */
    /* Total loop iterations = 500,000 */
    movw r4, #0xA4B0
    movt r4, #0x0007   /* r4 = 500,000 */
    
    movs r0, #1        /* accumulator 1 */
    movs r1, #2        /* accumulator 2 */
    movs r2, #3        /* accumulator 3 */
    movs r3, #4        /* accumulator 4 */

stress_loop:
    /* Data processing chain */
    adds r0, r0, r1
    subs r1, r2, #1
    muls r2, r3, r2
    eors r3, r0, r1
    ands r1, r1, r2
    orrs r2, r2, r3
    lsls r1, r1, #1
    lsrs r2, r2, #1
    
    /* Loop decrement */
    subs r4, r4, #1
    bne stress_loop

    /* Success: exit code 0 */
    movs r0, #0
    svc #0

fail:
    movs r0, #1
    svc #0
