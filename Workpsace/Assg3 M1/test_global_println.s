.align 2
.data
_nl: .asciiz "\n"
_y: .word 0
_x: .word 0

.align 2
.text

.globl main
main: j _main

_println:
    li $v0, 1
    lw $a0, 0($sp)
    syscall
    li $v0, 4
    la $a0, _nl
    syscall
    jr $ra

_main:
    la $sp, -8($sp)
    sw $fp, 4($sp)
    sw $ra, 0($sp)
    la $fp, 0($sp)
    li $t0, 111
    sw $t0, -4($fp)
    lw $t0, -4($fp)
    sw $t0, _x
    li $t1, 222
    sw $t1, -8($fp)
    lw $t1, -8($fp)
    sw $t1, _y
    lw $t2, _x
    la $sp, -4($sp)
    sw $t2, 0($sp)
    jal _println
    la $sp, 4($sp)
    lw $t3, _y
    la $sp, -4($sp)
    sw $t3, 0($sp)
    jal _println
    la $sp, 4($sp)
    la $sp, 0($fp)
    lw $ra, 0($sp)
    lw $fp, 4($sp)
    la $sp, 8($sp)
    jr $ra
