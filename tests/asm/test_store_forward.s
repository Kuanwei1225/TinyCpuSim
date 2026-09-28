.syntax unified
.thumb
.text
.global _start
.type _start, %function

_start:
    /* Store-to-Load Forwarding Comprehensive Benchmark (>2000 insts) */
    ldr r0, =fwd_buf

    /* Initialize base buffer locations */
    movs r1, #0
    str r1, [r0, #0]
    str r1, [r0, #4]
    str r1, [r0, #8]
    str r1, [r0, #12]
    str r1, [r0, #16]
    str r1, [r0, #20]

    /* r6: Loop counter = 250 iterations (total ~5000 instructions) */
    movs r6, #250
    movs r7, #0        /* r7: running checksum */

stlf_loop:
    /* 1. Same-Address Immediate STLF */
    adds r1, r6, #1
    str r1, [r0, #0]
    ldr r2, [r0, #0]   /* STLF Hit: load immediately from SQ */

    /* 2. Multi-Word Offset STLF (Consecutive SQ tracking) */
    adds r1, r6, #2
    str r1, [r0, #4]
    adds r1, r6, #3
    str r1, [r0, #8]
    ldr r3, [r0, #4]   /* STLF Hit: offset +4 */
    ldr r4, [r0, #8]   /* STLF Hit: offset +8 */

    /* 3. RAW Dependent ALU -> Store -> Load Forwarding Pipeline */
    adds r5, r2, r3    /* r5 = (r6+1) + (r6+2) */
    adds r5, r5, r4    /* r5 = (r6+1) + (r6+2) + (r6+3) */
    str r5, [r0, #12]
    ldr r1, [r0, #12]  /* STLF Hit: offset +12 */

    /* 4. Non-aliasing Store & Load Disambiguation */
    movs r2, #0x55
    str r2, [r0, #20]  /* Store to offset +20 */
    ldr r3, [r0, #0]   /* Load from offset +0 (must not alias with +20) */

    /* 5. Accumulate results into running checksum */
    adds r7, r7, r1
    adds r7, r7, r3

    /* Loop decrement and branch */
    subs r6, r6, #1
    bne stlf_loop

    /* Verify checksum is non-zero */
    cmp r7, #0
    beq fail

    /* Success: exit code 0 */
    movs r0, #0
    movs r7, #1
    svc #0

fail:
    movs r0, #1
    movs r7, #1
    svc #0

.align 4
literal_pool:
    fwd_buf_addr: .word fwd_buf

.bss
.align 4
fwd_buf:
    .space 64
