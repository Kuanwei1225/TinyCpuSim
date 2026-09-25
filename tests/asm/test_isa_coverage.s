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

    /* Set up scratch buffer address in RAM (0x00020000) */
    movw r4, #0x0000
    movt r4, #0x0002

    /* Initialize loop counter: 3 iterations */
    movs r7, #3

coverage_loop:
    /* 1. MOV, MVN, MOVW, MOVT */
    movs r1, #10
    mov r2, r1
    mvns r3, r1
    movw r0, #0x1234
    movt r0, #0x5678

    /* 2. ADD, ADC, SUB, SBC, RSB */
    adds r1, r1, #5
    adds r2, r1, r2
    adcs r2, r2, r1
    subs r1, r1, #2
    subs r2, r2, r1
    sbcs r2, r2, r1
    rsbs r3, r1, #0

    /* 3. MUL, MLA */
    movs r1, #3
    movs r2, #4
    muls r1, r2, r1
    movs r0, #5
    mla r0, r1, r2, r0

    /* 4. Logic: AND, ORR, EOR, BIC */
    movs r1, #0xAA
    movs r2, #0x55
    ands r3, r1, r2
    orrs r3, r1, r2
    eors r3, r1, r2
    bics r3, r1, r2

    /* 5. Compares: CMP, CMN, TST, TEQ */
    cmp r1, #0xAA
    cmn r1, r2
    tst r1, r2
    teq r1, r2

    /* 6. Shifts: ASR, LSL, LSR, ROR */
    movs r1, #0x80
    asrs r2, r1, #1
    lsls r2, r1, #1
    lsrs r2, r1, #1
    movs r3, #1
    rors r2, r2, r3

    /* 7. Branching: B, BL, BX, BLX, CBZ, CBNZ */
    b test_b_target
test_b_target:
    bl test_bl_target
    
    /* BLX with register */
    ldr r6, =test_blx_target
    blx r6

    /* CBZ / CBNZ */
    movs r0, #0
    cbz r0, test_cbz_target
    b fail
test_cbz_target:
    movs r0, #1
    cbnz r0, test_cbnz_target
    b fail
test_cbnz_target:

    /* 8. Memory Single: STR, LDR, STRB, LDRB, STRH, LDRH, LDRSB, LDRSH */
    /* Store and load word */
    movs r1, #0x42
    str r1, [r4, #0]
    ldr r2, [r4, #0]
    cmp r1, r2
    bne fail

    /* Store and load byte */
    movs r1, #0x7F
    strb r1, [r4, #4]
    ldrb r2, [r4, #4]
    cmp r1, r2
    bne fail

    /* Store and load halfword */
    movw r1, #0x1234
    strh r1, [r4, #8]
    ldrh r2, [r4, #8]
    cmp r1, r2
    bne fail

    /* LDRSB and LDRSH */
    movs r3, #4
    ldrsb r2, [r4, r3]
    movs r3, #8
    ldrsh r2, [r4, r3]

    /* 9. Memory Multiple: STM, LDM */
    movs r1, #11
    movs r2, #22
    movw r4, #0x0000
    movt r4, #0x0002
    stm r4!, {r1, r2}
    
    movw r4, #0x0000
    movt r4, #0x0002
    ldm r4!, {r1, r2}
    cmp r1, #11
    bne fail
    cmp r2, #22
    bne fail

    /* 10. Stack: PUSH, POP */
    push {r1, r2}
    movs r1, #0
    movs r2, #0
    pop {r1, r2}
    cmp r1, #11
    bne fail
    cmp r2, #22
    bne fail

    /* 11. System: MRS, MSR, NOP */
    nop
    mrs r1, APSR
    msr APSR_nzcvq, r1

    /* Decrement loop counter */
    subs r7, r7, #1
    cmp r7, #0
    bne coverage_loop

    /* All 3 iterations finished successfully */
    movs r0, #0
    svc #0

fail:
    movs r0, #1
    svc #0

/* Functions for BL / BLX / BX testing */
.type test_bl_target, %function
test_bl_target:
    bx lr

.type test_blx_target, %function
test_blx_target:
    bx lr
