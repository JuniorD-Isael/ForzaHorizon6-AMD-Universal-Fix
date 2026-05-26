OPTION CASEMAP:NONE

EXTERN GetOriginalDXGIProcByName:PROC
EXTERN GetOriginalDXGIProcByOrdinal:PROC

.code

JMP_BY_NAME MACRO procName, nameLabel
procName PROC
    sub rsp, 58h
    mov [rsp+20h], rcx
    mov [rsp+28h], rdx
    mov [rsp+30h], r8
    mov [rsp+38h], r9
    lea rcx, nameLabel
    call GetOriginalDXGIProcByName
    mov r10, rax
    mov rcx, [rsp+20h]
    mov rdx, [rsp+28h]
    mov r8,  [rsp+30h]
    mov r9,  [rsp+38h]
    add rsp, 58h
    jmp r10
procName ENDP
ENDM

; No forwarders for symbols implemented directly in C++ (CreateDXGIFactory/1/2 and DXGIGetDebugInterface1)

END
