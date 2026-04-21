; ============================================================
; Pseudo-Assembly for: examples/floats.toy
; ============================================================
SECTION .text

    MOV R0, 3.14159
    STORE pi, R0
    MOV R1, 5
    STORE radius, R1
    ; [DCE] eliminated: ; [dead] (eliminated)
    MOV R2, 15.708
    STORE area, R2
    MOV R3, 3
    STORE count, R3
    ; [DCE] eliminated: ; [dead] (eliminated)
    MOV R4, 5.236
    STORE avg, R4
    PRINT 3.14159
    PRINT 5
    PRINT 15.708
    PRINT 5.236

    HALT
