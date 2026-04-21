; === Unit: examples/main.toy ===
; ============================================================
; Pseudo-Assembly for: examples/main.toy
; ============================================================
SECTION .text

    MOV R0, 10
    STORE base, R0
    MOV R1, 2
    STORE scale, R1
    ; [DCE] eliminated: ; [dead] (eliminated)
    MOV R2, 20
    STORE input, R2
    PRINT 20

    HALT
; === Unit: examples/math.toy ===
; ============================================================
; Pseudo-Assembly for: examples/math.toy
; ============================================================
SECTION .text

    MOV R0, 7
    STORE x, R0
    MOV R1, 3
    STORE y, R1
    ; [DCE] eliminated: ; [dead] (eliminated)
    MOV R2, 10
    STORE result, R2
    ; [DCE] eliminated: ; [dead] (eliminated)
    MOV R3, 21
    STORE product, R3
    PRINT 10
    PRINT 21

    HALT
; === Unit: examples/utils.toy ===
; ============================================================
; Pseudo-Assembly for: examples/utils.toy
; ============================================================
SECTION .text

    MOV R0, 42
    STORE helper_val, R0
    MOV R1, 3.14
    STORE helper_pi, R1
    ; [DCE] eliminated: ; [dead] (eliminated)
    MOV R2, 84
    STORE doubled, R2
    PRINT 42
    PRINT 84

    HALT
