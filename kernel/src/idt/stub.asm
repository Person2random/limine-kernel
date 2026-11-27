; stub.asm
bits 64

global isr_stub_table
extern interrupt_handler
extern user_saved_rsp
extern user_saved_cr3
extern user_saved_rflags


; exceptions without cpu error code
%macro isr_no_err_stub 1
isr_stub_%+%1:
    push qword 0        ; fake error code
    push qword %1       ; exception number
    jmp isr_common_stub
%endmacro


; exceptions with cpu error code
%macro isr_err_stub 1
isr_stub_%+%1:
    push qword %1       ; exception number
    jmp isr_common_stub
%endmacro

%macro irq_stub 1
isr_stub_%+%1:
    push qword 0
    push qword %1
    jmp isr_common_stub
%endmacro


isr_common_stub:

    ; save registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15


    ; build a normalized 22-qword frame in separate stack storage:
    ; 15 gprs, vector, error, rip, cs, rflags, rsp, ss.
    ; the source frame starts immediately after the saved gprs. copy its
    ; first 20 qwords (gprs through rflags), then fill the final two fields.
    sub rsp, 176
    mov rdi, rsp
    lea rsi, [rsp + 176]
    mov rcx, 20
    cld
    rep movsq

    ; the copied cs is at byte offset 144. on an outer-privilege interrupt
    ; the hardware's old rsp/ss follow rflags in the source frame.
    test qword [rsp + 144], 3
    jz .same_privilege
    mov rax, [rsp + 160 + 176]
    mov [rsp + 160], rax
    mov rax, [rsp + 168 + 176]
    mov [rsp + 168], rax
    jmp .frame_ready
.same_privilege:
    mov qword [rsp + 160], 0
    mov qword [rsp + 168], 0
.frame_ready:
    ; keep the normalized-frame base in a callee-saved register, then align
    ; the temporary call stack as required by the system v abi. the interrupted
    ; value of rbx is already saved in the original register frame.
    mov rbx, rsp
    mov rdi, rbx
    and rsp, -16

    call interrupt_handler

    cmp qword [rbx + 120], -1
    je .user_exit

    ; Copy syscall return registers back to the interrupted user context.
    lea rsi, [rbx]
    lea rdi, [rbx + 176]
    mov rcx, 15
    cld
    rep movsq

    ; discard normalized copy; the original saved-register and cpu frames are
    ; still intact below it.
    mov rsp, rbx
    add rsp, 176


    ; restore registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax


    ; discard vector and error code, then let iretq consume the original cpu
    ; frame (three words for same privilege, five after a privilege change).
    add rsp, 16              ; discard vector and error code
    iretq

.user_exit:
    mov rax, [rel user_saved_cr3]
    mov cr3, rax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov rsp, [rel user_saved_rsp]
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    push qword [rel user_saved_rflags]
    popfq
    ret

global user_enter
user_enter:
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov [rel user_saved_rsp], rsp
    pushfq
    pop rax
    mov [rel user_saved_rflags], rax
    mov rax, cr3
    mov [rel user_saved_cr3], rax
    mov cr3, rdx

    mov ax, 0x23
    mov ds, ax
    mov es, ax
    push qword 0x23
    push rsi
    pushfq
    pop rax
    or rax, 0x200
    and rax, ~0x3000
    push rax
    push qword 0x1B
    push rdi
    iretq

section .userdemo progbits alloc exec nowrite align=4096
global user_demo_start
global user_demo_end
user_demo_start:
    mov eax, 1
    lea rdi, [rel user_demo_message]
    mov esi, user_demo_message_end - user_demo_message
    int 0x80
    mov eax, 2
    xor edi, edi
    int 0x80
.hang:
    pause
    jmp .hang
user_demo_message:
    db "Hello from ring 3! Syscalls work."
user_demo_message_end:
user_demo_end:

section .bss
align 8
global user_saved_rsp
user_saved_rsp: resq 1
global user_saved_cr3
user_saved_cr3: resq 1
global user_saved_rflags
user_saved_rflags: resq 1

section .text


; cpu exceptions 0-31

isr_no_err_stub 0
isr_no_err_stub 1
isr_no_err_stub 2
isr_no_err_stub 3
isr_no_err_stub 4
isr_no_err_stub 5
isr_no_err_stub 6
isr_no_err_stub 7

isr_err_stub 8

isr_no_err_stub 9

isr_err_stub 10
isr_err_stub 11
isr_err_stub 12
isr_err_stub 13
isr_err_stub 14

isr_no_err_stub 15
isr_no_err_stub 16

isr_err_stub 17

isr_no_err_stub 18
isr_no_err_stub 19
isr_no_err_stub 20
isr_err_stub 21
isr_no_err_stub 22
isr_no_err_stub 23
isr_no_err_stub 24
isr_no_err_stub 25
isr_no_err_stub 26
isr_no_err_stub 27
isr_no_err_stub 28
isr_err_stub 29

isr_err_stub 30

isr_no_err_stub 31
irq_stub 32
irq_stub 33
%assign i 34
%rep 94
irq_stub i
%assign i i+1
%endrep
irq_stub 128

; table of addresses for idt setup

isr_stub_table:

%assign i 0
%rep 129
    dq isr_stub_%+i
%assign i i+1
%endrep
