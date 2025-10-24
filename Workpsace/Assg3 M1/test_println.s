.align 2
.data
_nl: .asciiz "\n"
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
    li $t0, 12345
    sw $t0, -4($fp)
    lw $t0, -4($fp)
    la $sp, -4($sp)
    sw $t0, 0($sp)
    jal _println
    la $sp, 4($sp)
    li $t1, 67890
    sw $t1, -8($fp)
    lw $t1, -8($fp)
    la $sp, -4($sp)
    sw $t1, 0($sp)
    jal _println
    la $sp, 4($sp)
    la $sp, 0($fp)
    lw $ra, 0($sp)
    lw $fp, 4($sp)
    la $sp, 8($sp)
    jr $ra
