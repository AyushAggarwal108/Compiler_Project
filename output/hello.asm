; ============================================================
; Pseudo-Assembly for: examples/hello.toy
; ============================================================
SECTION .text

    MOV R0, 5
    STORE a, R0
    ; [DCE] eliminated: ; [dead] (eliminated)
    MOV R1, 7
    STORE b, R1
    MOV R2, 3.14
    STORE x, R2
    PRINT 5
    PRINT 7
    PRINT 3.14

    HALT
