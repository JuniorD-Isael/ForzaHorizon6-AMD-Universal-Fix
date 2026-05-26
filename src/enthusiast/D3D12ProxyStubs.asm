OPTION CASEMAP:NONE

EXTERN GetOriginalProcByName:PROC
EXTERN GetOriginalProcByOrdinal:PROC

.data
name_SetAppCompatStringPointer db "SetAppCompatStringPointer",0
name_D3D12GetDebugInterface db "D3D12GetDebugInterface",0
name_D3D12CoreCreateLayeredDevice db "D3D12CoreCreateLayeredDevice",0
name_D3D12CoreGetLayeredDeviceSize db "D3D12CoreGetLayeredDeviceSize",0
name_D3D12CoreRegisterLayers db "D3D12CoreRegisterLayers",0
name_D3D12CreateRootSignatureDeserializer db "D3D12CreateRootSignatureDeserializer",0
name_D3D12CreateVersionedRootSignatureDeserializer db "D3D12CreateVersionedRootSignatureDeserializer",0
name_D3D12DeviceRemovedExtendedData db "D3D12DeviceRemovedExtendedData",0
name_D3D12EnableExperimentalFeatures db "D3D12EnableExperimentalFeatures",0
name_D3D12GetInterface db "D3D12GetInterface",0
name_D3D12PIXEventsReplaceBlock db "D3D12PIXEventsReplaceBlock",0
name_D3D12PIXGetThreadInfo db "D3D12PIXGetThreadInfo",0
name_D3D12PIXNotifyWakeFromFenceSignal db "D3D12PIXNotifyWakeFromFenceSignal",0
name_D3D12PIXReportCounter db "D3D12PIXReportCounter",0
name_D3D12SerializeRootSignature db "D3D12SerializeRootSignature",0
name_D3D12SerializeVersionedRootSignature db "D3D12SerializeVersionedRootSignature",0
name_GetBehaviorValue db "GetBehaviorValue",0

.code

JMP_BY_NAME MACRO procName, nameLabel
procName PROC
    sub rsp, 58h
    mov [rsp+20h], rcx
    mov [rsp+28h], rdx
    mov [rsp+30h], r8
    mov [rsp+38h], r9
    lea rcx, nameLabel
    call GetOriginalProcByName
    mov r10, rax
    mov rcx, [rsp+20h]
    mov rdx, [rsp+28h]
    mov r8,  [rsp+30h]
    mov r9,  [rsp+38h]
    add rsp, 58h
    jmp r10
procName ENDP
ENDM

D3D12Ordinal99 PROC
    sub rsp, 58h
    mov [rsp+20h], rcx
    mov [rsp+28h], rdx
    mov [rsp+30h], r8
    mov [rsp+38h], r9
    mov ecx, 99
    call GetOriginalProcByOrdinal
    mov r10, rax
    mov rcx, [rsp+20h]
    mov rdx, [rsp+28h]
    mov r8,  [rsp+30h]
    mov r9,  [rsp+38h]
    add rsp, 58h
    jmp r10
D3D12Ordinal99 ENDP

JMP_BY_NAME SetAppCompatStringPointer, name_SetAppCompatStringPointer
JMP_BY_NAME D3D12GetDebugInterface, name_D3D12GetDebugInterface
JMP_BY_NAME D3D12CoreCreateLayeredDevice, name_D3D12CoreCreateLayeredDevice
JMP_BY_NAME D3D12CoreGetLayeredDeviceSize, name_D3D12CoreGetLayeredDeviceSize
JMP_BY_NAME D3D12CoreRegisterLayers, name_D3D12CoreRegisterLayers
JMP_BY_NAME D3D12CreateRootSignatureDeserializer, name_D3D12CreateRootSignatureDeserializer
JMP_BY_NAME D3D12CreateVersionedRootSignatureDeserializer, name_D3D12CreateVersionedRootSignatureDeserializer
JMP_BY_NAME D3D12DeviceRemovedExtendedData, name_D3D12DeviceRemovedExtendedData
JMP_BY_NAME D3D12EnableExperimentalFeatures, name_D3D12EnableExperimentalFeatures
JMP_BY_NAME D3D12GetInterface, name_D3D12GetInterface
JMP_BY_NAME D3D12PIXEventsReplaceBlock, name_D3D12PIXEventsReplaceBlock
JMP_BY_NAME D3D12PIXGetThreadInfo, name_D3D12PIXGetThreadInfo
JMP_BY_NAME D3D12PIXNotifyWakeFromFenceSignal, name_D3D12PIXNotifyWakeFromFenceSignal
JMP_BY_NAME D3D12PIXReportCounter, name_D3D12PIXReportCounter
JMP_BY_NAME D3D12SerializeRootSignature, name_D3D12SerializeRootSignature
JMP_BY_NAME D3D12SerializeVersionedRootSignature, name_D3D12SerializeVersionedRootSignature
JMP_BY_NAME GetBehaviorValue, name_GetBehaviorValue

END
