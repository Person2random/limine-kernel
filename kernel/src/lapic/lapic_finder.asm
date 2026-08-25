bits 64


global get_lapic_base

get_lapic_base:
        mov ecx, 0x1B
        rdmsr

        shl rdx,32
        or rax, rdx
        and rax, 0xFFFFFFFFFFFFF000;


        ret


global get_lapic_msr

get_lapic_msr:
    mov ecx, 0x1B
    rdmsr
    shl rdx, 32
    or rax, rdx
    ret
