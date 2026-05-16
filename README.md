# Forza AMD Universal Fix (APUs & Dedicadas)

Este é um fix experimental focado em contornar os erros **FH201**, **FH205** e os congelamentos em tela preta causados pelo motor gráfico *ForzaTech* ao lidar com limitações de silício e subalocação de memória (VRAM).

**Projeto Original:** [João Lucas](https://github.com/Megadroidgames/Forza-Horizon-6-RX-580-FH201-FH205-Fix)  
**Variante e Manutenção deste Fork:** [JuniorD-Isael](https://github.com/JuniorD-Isael)  
**Hardware Base de Testes:** AMD Radeon Vega 7 (Ryzen 5 5500U) rodando em ambiente Windows Insider Preview.

---

# 🚀 O Diferencial desta Versão

O projeto original cumpre muito bem o papel de mascarar o *Feature Level* em placas dedicadas. No entanto, ao tentar rodar o jogo em gráficos integrados (APUs), o motor gráfico costuma derrubar a aplicação por violação de acesso (`ACCESS_VIOLATION_READ`) devido ao orçamento de memória compartilhada ser reportado como zero (`VRAM_BUDGET: 0`).

Esta versão adiciona hooks avançados diretamente na *vtable* do DirectX 12 para resolver o problema da VRAM, além de forçar a exposição de suporte a Shader Model 6.6 e *Enhanced Barriers* para compatibilidade com o motor gráfico.

---

# 📋 Pré-requisitos Fundamentais

A DLL proxy atua como camada de compatibilidade para requisitos modernos do motor gráfico, mas a tradução física das instruções modernas ainda depende obrigatoriamente do driver modificado instalado no sistema.

O driver utilizado e homologado durante os testes foi:

- [AMD Software Adrenalin Edition - Agility SDK / Work Graphs](https://www.amd.com/en/resources/support-articles/release-notes/RN-RAD-MS-AGILITY-SDK-2023-6-711.html)

---

# 🛠️ Como Instalar

1. Baixe os arquivos da última versão deste repositório.
2. Acesse a pasta `bin`.
3. Copie o arquivo `d3d12.dll`.
4. Cole o arquivo diretamente no diretório principal do jogo (onde está localizado o executável `.exe`).

   **Exemplo:**

   ```text
   ...\ForzaHorizon6\d3d12.dll
   ```

5. Inicialize o jogo normalmente.

---

# ⚠️ Nota Importante de Inicialização

Na primeiríssima tentativa após colocar a DLL, o jogo pode entrar em tela preta e travar.

Isso é esperado devido à recalibração do cache gráfico e do shader cache do Windows.

Se isso acontecer:

1. Feche o jogo pelo Gerenciador de Tarefas.
2. Aguarde alguns segundos.
3. Abra novamente.

Na maioria dos casos, a segunda inicialização permitirá que o jogo avance corretamente para a tela de otimização.

---

# 💡 Dicas de Uso e Solução de Problemas

## Jogo fecha em tela preta se aberto logo após ser fechado

### Comportamento do Cache

O Windows pode levar algum tempo para limpar completamente o cache de shaders acumulado na memória após longas sessões de jogo.

Se o jogo for reaberto imediatamente, o motor gráfico pode colidir com recursos ainda bloqueados em memória.

### Solução

Após fechar o jogo, aguarde aproximadamente 1 minuto antes de iniciar novamente.

---

## O erro FH205 continua aparecendo

Isso normalmente indica que o driver modificado não ativou corretamente o suporte esperado pelo motor gráfico.

### Verificações recomendadas

- Reinicie o computador após instalar o driver AMD.
- Valide o suporte utilizando:

```text
tools\D3D12Caps.exe
```

---

## Fechamentos repentinos na abertura (Crash on Launch)

### Verificações recomendadas

- Verifique a integridade dos arquivos do jogo.
- Remova temporariamente:
  - ReShade
  - OptiScaler
  - overlays
  - outros injetores
  - arquivos adicionais como `dxgi.dll`

---

# ⚠️ Limitações Conhecidas

Este projeto realiza spoofing de capacidades modernas do Direct3D 12, porém não implementa completamente todos os recursos em nível de hardware.

Isso significa que:

- Mesh Shaders reais não são implementados.
- Alguns pipelines gráficos podem continuar instáveis.
- Crashes durante compilação de shaders ainda podem ocorrer.
- Algumas cutscenes ou transições podem falhar dependendo da build do Windows e do driver.
- O suporte depende fortemente do driver Agility SDK utilizado.

---

# 💻 Espaço do Desenvolvedor (Compilação)

Caso queira modificar o código-fonte, a lógica principal reside em:

```text
src/D3D12Proxy.cpp
```

## Ambiente Requerido

- Visual Studio com ferramentas de build C++
- Windows SDK atualizado

---

## Compilação via Script

Utilize PowerShell no ambiente de desenvolvedor do Visual Studio:

```powershell
powershell -ExecutionPolicy Bypass -File .\build_proxy.ps1
```
---

# 📜 Histórico de Modificações e Créditos

## v1.0 — Base Original (João Lucas)

- Engenharia reversa inicial e criação da estrutura de proxy via DLL.
- Implementação do bypass focado no erro FH201 para arquitetura Polaris (RX 580).

### Referências

- [Canal Megadroidgames](https://www.youtube.com/@MEGADROIDGAMESS)
- [Repositório Original](https://github.com/Megadroidgames/Forza-Horizon-6-RX-580-FH201-FH205-Fix)

---

## v1.1+ — Variante de Compatibilidade Universal AMD (JuniorD-Isael)

### Bypass de VRAM para APUs

Adicionado hook no índice 56 da *vtable* (`QueryVideoMemoryInfo`) utilizando mapeamento direto de memória via ponteiros (`uint64_t`).

Isso contorna o problema de orçamento zero de memória compartilhada que causava falhas críticas de leitura em gráficos integrados.

---

### Spoofing Avançado no Dispositivo

Forçada a exposição de capacidades críticas do Direct3D 12:

- `MaxSupportedFeatureLevel` → `12_1`
- `HighestShaderModel` → `6.6`
- `EnhancedBarriersSupported` → `TRUE`
- `MeshShaderTier` → `Tier 1`
- `SamplerFeedbackTier` → `Tier 1`

---

### Refatoração Estrutural

- Centralização do ciclo de hooks em `PatchDeviceInterfaces`
- Limpeza de helpers redundantes
- Expansão do teto de buffer para logs de monitoramento

---

Este fork mantém os devidos créditos ao trabalho original de João Lucas, expandindo a compatibilidade prática para novos cenários envolvendo APUs AMD e hardware integrado.
