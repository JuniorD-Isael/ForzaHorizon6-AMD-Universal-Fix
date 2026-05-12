# Forza RX 580 FH201/FH205 Fix

Fix experimental para placas **AMD RX 580 / Polaris** que recebem erro **FH201** ou **FH205** ao abrir o jogo.

Criado por **João Lucas**.

Canal no YouTube: https://www.youtube.com/@MEGADROIDGAMESS


## Antes de começar

Este fix foi feito para quem instalou o driver AMD Agility SDK / Work Graphs e ainda fica preso no erro **FH201**.

O driver usado nos testes foi:

```text
amd-software-adrenalin-edition-23.10.01.14-win10-win11-work-graphs
```

Importante:

- O fix é experimental.
- Pode não funcionar em todas as placas ou versões do jogo.
- Não inclui arquivos do jogo.
- Não remove DRM.
- Use por sua conta e risco.

## Como instalar

1. Baixe este repositório.

2. Abra a pasta:

```text
bin
```

3. Copie o arquivo:

```text
d3d12.dll
```

4. Cole esse arquivo na pasta principal do jogo, onde fica o executável.

Exemplo:

```text
...\Forza Horizon 6\d3d12.dll
```

A pasta certa é a mesma onde ficam vários arquivos `.dll` do jogo e o executável principal.

5. Abra o jogo normalmente.

## Como saber se está funcionando

Se antes aparecia:

```text
FH201
FH205
```

e agora o jogo abre, o fix está carregando corretamente.

O fix também cria um arquivo de log na pasta do jogo:

```text
ForzaFix_RX580.log
```

Se esse arquivo aparecer, significa que a DLL foi carregada.

## Se ainda der FH205

O erro **FH205** normalmente significa que o driver ainda não está reportando suporte a **Enhanced Barriers**.

Nesse caso, instale o driver AMD Agility SDK / Work Graphs e reinicie o PC.

Depois rode:

```text
tools\D3D12Caps.exe
```

Procure esta linha:

```text
OPTIONS12: OK EnhancedBarriersSupported=TRUE
```

Se aparecer `FALSE`, o driver não ativou o suporte necessário.

## Se der FHE01 ou fechar sozinho

Tente:

- verificar a integridade dos arquivos do jogo;
- remover outros mods/fixes;
- remover `dxgi.dll`, ReShade, OptiScaler ou outras DLLs de terceiros da pasta do jogo;
- testar com o jogo limpo e só este `d3d12.dll`.

## Como remover o fix

Apague este arquivo da pasta do jogo:

```text
d3d12.dll
```

Pronto. O jogo volta a usar o DirectX normal do Windows.

## Para desenvolvedores

O código fonte está em:

```text
src
```

Para compilar, instale:

- Visual Studio 2022 Build Tools;
- Desktop development with C++;
- Windows SDK.

Depois rode:

```powershell
powershell -ExecutionPolicy Bypass -File .\build_proxy.ps1
```

A DLL compilada vai sair em:

```text
proxy_build\d3d12.dll
```

## O que o fix faz

Ele cria uma DLL proxy chamada `d3d12.dll`.

Ela intercepta algumas chamadas do DirectX 12 e força o jogo a enxergar suporte a `Feature Level 12_1`, ajudando a passar pelo erro **FH201**.

O suporte a **Enhanced Barriers** precisa vir do driver. Por isso o driver AMD Agility SDK / Work Graphs é importante.
