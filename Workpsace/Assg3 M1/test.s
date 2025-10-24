.align 2
.data
_nl: .asciiz "\n"
.align 2
.text

_println:
    li $v0, 1
    lw $a0, 0($sp)
    syscall
    li $v0, 4
    la $a0, _nl
    syscall
    jr $ra

_g:
    la $sp, -8($sp)
    sw $fp, 4($sp)
    sw $ra, 0($sp)
    la $fp, 0($sp)
    la $sp, -8($sp)
    lw $t0, 8($fp)
    sw $t0, -4($fp)
    lw $t1, -4($fp)
    sw $t1, -8($fp)
    lw $t2, -4($fp)
    la $sp, -4($sp)
    sw $t2, 0($sp)
    jal _println
    la $sp, 4($sp)
    lw $t3, 12($fp)
    sw $t3, -4($fp)
    lw $t4, -4($fp)
    la $sp, -4($sp)
    sw $t4, 0($sp)
    jal _println
    la $sp, 4($sp)
    la $sp, 0($fp)
    lw $ra, 0($sp)
    lw $fp, 4($sp)
    la $sp, 8($sp)
    jr $ra
.align 2
.text

_f:
    la $sp, -8($sp)
    sw $fp, 4($sp)
    sw $ra, 0($sp)
    la $fp, 0($sp)
    la $sp, -8($sp)
    lw $t0, 8($fp)
    sw $t0, -4($fp)
    lw $t1, -4($fp)
    la $sp, -4($sp)
    sw $t1, 0($sp)
    lw $t2, 12($fp)
    la $sp, -4($sp)
    sw $t2, 0($sp)
    jal _g
    la $sp, 8($sp)
    lw $t3, 12($fp)
    sw $t3, -8($fp)
    lw $t4, -8($fp)
    la $sp, -4($sp)
    sw $t4, 0($sp)
    lw $t5, 8($fp)
    la $sp, -4($sp)
    sw $t5, 0($sp)
    jal _g
    la $sp, 8($sp)
    la $sp, 0($fp)
    lw $ra, 0($sp)
    lw $fp, 4($sp)
    la $sp, 8($sp)
    jr $ra
.align 2
.text

.globl main
main: j _main

_main:
    la $sp, -8($sp)
    sw $fp, 4($sp)
    sw $ra, 0($sp)
    la $fp, 0($sp)
    la $sp, -8($sp)
    li $t0, 12345
    sw $t0, -12($fp)
    lw $t0, -12($fp)
    sw $t0, -4($fp)
    li $t1, 23456
    sw $t1, -16($fp)
    lw $t1, -16($fp)
    sw $t1, -8($fp)
    lw $t2, -4($fp)
    la $sp, -4($sp)
    sw $t2, 0($sp)
    lw $t3, -8($fp)
    la $sp, -4($sp)
    sw $t3, 0($sp)
    jal _f
    la $sp, 8($sp)
    la $sp, 0($fp)
    lw $ra, 0($sp)
    lw $fp, 4($sp)
    la $sp, 8($sp)
    jr $ra
