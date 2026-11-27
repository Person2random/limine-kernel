bits 64
global ring0_context_switch
global ring3_context_switch
ring0_context_switch:
	cli
        mov rsp, rdi
        pop r15
        pop r14
        pop r13
        pop r12
        pop r11
        pop r10
        pop r9
        pop r8
        pop rbp
        add rsp,8
        pop rsi
        pop rdx
        pop rcx
        pop rbx
        pop rax
        add rsp, 24
        pop rsp
        push qword [rdi+136]
        push qword [rdi+128]
        push qword [rdi+120]
        mov rdi, [rdi+72]
        iretq

ring3_context_switch:
	cli
	mov rsp, rdi
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
	iretq
