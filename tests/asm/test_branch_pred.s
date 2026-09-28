.syntax unified
.thumb
.text
.global _start
.type _start, %function

_start:
    /* Comprehensive Branch Prediction Benchmark (>5000 instructions) */
    /* Phase 1: 500 iterations of Alternating TNTN pattern (even/odd) */
    movs r0, #0
    movs r1, #0
    movw r2, #500

branch_loop_p1:
    movs r3, r2
    ands r3, r3, #1
    cmp r3, #0
    beq is_even_p1

is_odd_p1:
    adds r0, r0, #1
    b next_iter_p1

is_even_p1:
    adds r1, r1, #1

next_iter_p1:
    subs r2, r2, #1
    bne branch_loop_p1

    /* Verify Phase 1: 250 odd, 250 even */
    movw r3, #250
    cmp r0, r3
    bne fail
    cmp r1, r3
    bne fail

    /* Phase 2: 400 iterations of 4-Step Periodic Pattern */
    movs r4, #0
    movw r5, #400

branch_loop_p2:
    movs r6, r5
    ands r6, r6, #3
    cmp r6, #2
    beq p2_case_skip

p2_case_take:
    adds r4, r4, #1
    b p2_next

p2_case_skip:
    nop

p2_next:
    subs r5, r5, #1
    bne branch_loop_p2

    /* Verify Phase 2: 300 pattern hits */
    movw r3, #300
    cmp r4, r3
    bne fail

    /* Phase 3: 300 iterations of Correlated Branches */
    movs r0, #0
    movw r7, #300

branch_loop_p3:
    movs r1, r7
    ands r1, r1, #1
    cmp r1, #0
    beq p3_even

p3_odd:
    movs r2, r7
    ands r2, r2, #2
    cmp r2, #0
    beq p3_skip_corr
    adds r0, r0, #1
    b p3_next

p3_even:
    movs r2, r7
    ands r2, r2, #2
    cmp r2, #0
    bne p3_skip_corr
    adds r0, r0, #1
    b p3_next

p3_skip_corr:
    nop

p3_next:
    subs r7, r7, #1
    bne branch_loop_p3

    /* Verify Phase 3: r0 must be exactly 150 */
    movw r3, #150
    cmp r0, r3
    bne fail

    /* Success: exit code 0 */
    movs r0, #0
    movs r7, #1
    svc #0

fail:
    movs r0, #1
    movs r7, #1
    svc #0
