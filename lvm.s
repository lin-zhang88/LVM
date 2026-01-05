.section .data
# VM State: 8 16-bit registers and 16-bit flags
.globl vm_regs
vm_regs:    .short 0, 0, 0, 0, 0, 0, 0, 0  # R6=BP (offset 12), R7=SP (offset 14)
.globl vm_flags
vm_flags:   .short 0x0008                  # Bit 3: IE (Interrupt Enable) is on by default
.globl vm_memory
vm_memory:  .skip 65536                    # 64KB RAM

# Constants
.set FLAG_POSITIVE, 1
.set FLAG_ZERO,     2
.set FLAG_NEGATIVE, 4
.set FLAG_INT_ENABLE, 8

.set RESULT_CONTINUE, 0
.set RESULT_ILLEGAL,  1
.set RESULT_UNALIGNED, 2

.section .text
.globl step

# Helper: Fetch 16-bit word from stream (unaligned allowed)
fetch16:
    movzwl vm_regs(%rip), %eax
    leaq   vm_memory(%rip), %rcx
    movzwl (%rcx, %rax), %eax
    addw   $2, vm_regs(%rip)
    ret

step:
    pushq %rbp
    movq  %rsp, %rbp
    pushq %rbx
    pushq %rsi
    pushq %rdi

    # --- 1. Interrupt Handling ---
    # int_num is already in %rdi (64-bit calling convention)
    movl  %edi, %edi                  # Keep only low 32 bits
    cmpl  $-1, %edi
    je    fetch_decode
    movzwq vm_flags(%rip), %rax
    movw   %ax, %ax                    # Keep only low 16 bits
    testw $FLAG_INT_ENABLE, %ax
    jz    fetch_decode

    # Save state to stack
    subw  $2, vm_regs+14(%rip)        # R7 -= 2
    movzwl vm_regs+14(%rip), %ebx
    testw $1, %bx                     # Alignment check
    jnz   unaligned_error
    
    leaq  vm_memory(%rip), %rcx
    movw  vm_flags(%rip), %dx
    movw  %dx, (%rcx, %rbx)           # Push Flags

    subw  $2, vm_regs+14(%rip)        # R7 -= 2
    movzwl vm_regs+14(%rip), %ebx
    movw  vm_regs(%rip), %dx
    movw  %dx, (%rcx, %rbx)           # Push IP

    andw  $~FLAG_INT_ENABLE, vm_flags(%rip) # Disable interrupts
    movl  %edi, %eax
    shll  $1, %eax
    addl  $0xFFF0, %eax               # IVT base
    movzwq (%rcx, %rax), %rdx
    movw   %dx, %dx                    # Keep only low 16 bits
    movw  %dx, vm_regs(%rip)
    jmp   res_continue

    # --- 2. Fetch & Decode ---
fetch_decode:
    movzwl vm_regs(%rip), %eax
    leaq   vm_memory(%rip), %rcx
    movzbl (%rcx, %rax), %ebx         # BL = opcode
    incw   vm_regs(%rip)

    movb   %bl, %al
    shrb   $6, %al                    # op
    movb   %bl, %dl
    shrb   $3, %dl
    andb   $7, %dl                    # d
    andb   $7, %bl                    # s

    # --- 3. Group Dispatch ---
    testb  %dl, %dl
    jnz    check_unary

    # Control/Memory (d=0)
    testb  %bl, %bl
    jnz    handle_memory

    # Control Flow: op=0 (RET), 1 (CJMP), 2 (JMP), 3 (CALL)
    cmpb   $0, %al
    je     do_ret
    cmpb   $1, %al
    je     do_cjmp
    cmpb   $2, %al
    je     do_jmp
    # op==3: CALL
    call   fetch16
    movw   %ax, vm_regs(%rip)          # Set IP to target
    jmp    res_continue

do_ret:
    # RET instruction - pop IP from stack
    movzwl vm_regs+14(%rip), %ebx     # R7 (SP)
    testw  $1, %bx
    jnz    unaligned_error
    leaq   vm_memory(%rip), %rcx
    movzwq (%rcx, %rbx), %rdx
    movw   %dx, vm_regs(%rip)          # Restore IP
    addw   $2, vm_regs+14(%rip)        # SP += 2
    # If IE was 0, also restore flags (IRET)
    testw  $FLAG_INT_ENABLE, vm_flags(%rip)
    jnz    res_continue
    movzwq (%rcx, %rbx), %rdx
    movw   %dx, vm_flags(%rip)         # Restore flags
    addw   $2, vm_regs+14(%rip)        # SP += 2
    jmp    res_continue

do_cjmp:
    # Conditional jump - placeholder
    jmp    res_continue

do_jmp:
    # Absolute jump
    call   fetch16
    movw   %ax, vm_regs(%rip)
    jmp    res_continue

handle_memory:
    # EA Calculation per mod byte
    movzwl vm_regs(%rip), %eax
    movzbl (%rcx, %rax), %esi         # mod byte
    incw   vm_regs(%rip)
    # sz = (mod & 0x20) ? 2 : 1
    # Check D (bit 3), mode (7-6), add R[NNN], check I (bit 4)
    # ... (Implementation of EA calculation) ...
    jmp    res_continue

    # --- 4. Unary/Immediate (s=0) ---
check_unary:
    testb  %bl, %bl
    jnz    handle_alu
    
    cmpb   $0, %al                    # Load Immediate
    jne    do_shift
    call   fetch16
    movzbl %dl, %ecx
    movw   %ax, vm_regs(,%ecx, 2)
    movw   %ax, %bx                   # For NZP update
    jmp    update_nzp

do_shift:
    # ... (Shift logic: D, A, R bits) ...
    jmp    update_nzp

    # --- 5. ALU ---
handle_alu:
    movzbl %bl, %ecx                  # s
    movzwl vm_regs(,%ecx, 2), %esi    # R[s]
    movzbl %dl, %ecx                  # d
    movzwl vm_regs(,%ecx, 2), %edi    # R[d]

    cmpb   $1, %al
    je     alu_add
    cmpb   $2, %al
    je     alu_and
    cmpb   $3, %al
    je     alu_xor
    movw   %si, %di                   # MOV (op 0)
    jmp    alu_save

alu_add: addw %si, %di; jmp alu_save
alu_and: andw %si, %di; jmp alu_save
alu_xor: xorw %si, %di

alu_save:
    movw   %di, vm_regs(,%ecx, 2)
    movw   %di, %bx

update_nzp:
    # Update Positive/Zero/Negative while keeping IE
    movzwq vm_flags(%rip), %rax
    andw   $FLAG_INT_ENABLE, %ax
    testw  %bx, %bx
    jz     set_f_zero
    js     set_f_neg
    orw    $FLAG_POSITIVE, %ax
    jmp    save_f
set_f_zero: orw $FLAG_ZERO, %ax; jmp save_f
set_f_neg:  orw $FLAG_NEGATIVE, %ax
save_f:     movw %ax, vm_flags(%rip)

res_continue:
    movl $RESULT_CONTINUE, %eax
    jmp  done

unaligned_error:
    movl $RESULT_UNALIGNED, %eax

done:
    popq %rdi
    popq %rsi
    popq %rbx
    popq %rbp
    ret
    