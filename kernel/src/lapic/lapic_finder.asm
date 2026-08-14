bits 64


global get_lapic_base

get_lapic_base:
        mov ecx, 0x1B
        rdmsr

        shl rdx,32
        or rax, rdx
        and rax, 0xFFFFFFFFFFFFF000;


        ret
