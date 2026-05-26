# Standard Fix (Driver 23.10)

## Publico alvo
- Usuarios com 8 GB a 12 GB de RAM.
- Prioridade em estabilidade e boot consistente.

## Base tecnica
- Codigo espelhado da branch main em [src/standard/D3D12Proxy.cpp](../src/standard/D3D12Proxy.cpp) e [src/standard/DXGIProxy.cpp](../src/standard/DXGIProxy.cpp).
- Este perfil e mantido sem melhorias agressivas de spoof.

## Recursos principais
- Spoof de VRAM em 4 GB para manter comportamento conservador.
- Feature Level 12.0 como baseline de compatibilidade.
- Estabilidade nativa alinhada ao driver 23.10.

## Build isolada
- d3d12: powershell -ExecutionPolicy Bypass -File .\build_proxy.ps1 -profile standard -target d3d12
- dxgi: powershell -ExecutionPolicy Bypass -File .\build_proxy.ps1 -profile standard -target dxgi
- pacote completo: powershell -ExecutionPolicy Bypass -File .\build_proxy.ps1 -profile standard -target all

## Quando usar
- Quando a prioridade for menor risco de crash em setups com memoria limitada.
- Quando o usuario nao precisa de preset extremo de textura.
