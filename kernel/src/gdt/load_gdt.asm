global load_gdt
global reload_segments

section .text

load_gdt:
    lgdt [rdi]
    ret

reload_segments:
    push 0x08
    lea rax, [rel .reload]
    push rax
    retfq

.reload:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    ret
