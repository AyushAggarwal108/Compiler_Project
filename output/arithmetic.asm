; ============================================================
; Pseudo-Assembly for: examples/arithmetic.toy
; ============================================================
SECTION .text

    MOV R0, 10
    STORE a, R0
    MOV R1, 3
    STORE b, R1
    ; [DCE] eliminated: ; [dead] (eliminated)
    MOV R2, 13
    STORE c, R2
    ; [DCE] eliminated: ; [dead] (eliminated)
    MOV R3, 7
    STORE d, R3
    ; [DCE] eliminated: ; [dead] (eliminated)
    MOV R4, 30
    STORE e, R4
    ; [DCE] eliminated: ; [dead] (eliminated)
    ; [DCE] eliminated: ; [dead] (eliminated)
    MOV R5, 50
    STORE sum, R5
    ; [DCE] eliminated: ; [dead] (eliminated)
    ; [DCE] eliminated: ; [dead] (eliminated)
    MOV R6, 22
    STORE folded, R6
    ; [DCE] eliminated: ; [dead] (eliminated)
    ; [DCE] eliminated: ; [dead] (eliminated)
    MOV R7, 50
    STORE also_folded, R7
    ; [DCE] eliminated: ; [dead] (eliminated)
    MOV R0, 11
    STORE a, R0
    PRINT 11
    PRINT 3
    PRINT 13
    PRINT 7
    PRINT 30
    PRINT 50
    PRINT 22
    PRINT 50

    HALT
