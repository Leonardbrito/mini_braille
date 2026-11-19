# Guia para o Agente Claude — Projeto Piezo Braille Firmware

Firmware (ESP32 clássico, ESP-IDF v6.0/FreeRTOS) de um **módulo braille atualizável**: cada ponto
sobe e desce por um micro motor de passo, e o mecanismo se autotrava (o motor só é energizado para
mudar o pino). Projeto de iniciação científica, inspirado no
[módulo eletromecânico do Hackaday](https://hackaday.io/project/191181-electromechanical-refreshable-braille-module),
que usa came excêntrico + ímã em vez de motor.

Hardware, ligação e mecanismo estão no [README.md](README.md) — este arquivo é autocontido e cobre
as diretrizes de como trabalhar **neste** repositório: build, arquitetura, comunicação entre tasks e
padrão de código.

**Etapa atual:** acionar UM motor e verificar que gira nos dois sentidos. Mecanismo (fuso, came
excêntrico ou barra com rampa) e cela 2x3 vêm depois.

## Processo de Trabalho — Planejar Antes de Corrigir

Antes de fazer qualquer correção ou implementação direto no projeto:

1. **Traçar um plano** e apresentá-lo antes de executar
2. **Explicitar o entendimento** da solicitação com as próprias palavras
3. **Verificar brechas** na solicitação: edge cases, pontos cegos, interações com o restante do projeto
4. **Não assumir** nada que não seja óbvio — perguntar ao usuário
5. Ser **minucioso** sobre casos de borda tanto na solicitação quanto no projeto em geral

Exceção: hotfix de produção quebrada (corrigir primeiro, explicar depois).

## Segurança do Hardware

Este firmware aciona bobinas reais; um erro pode esquentar ou queimar o motor. Antes de mexer em
pinos, tempos ou sequência de fases, respeite:

- Motor de **3 V** ("SM56-F40", modelo não confirmado; ~20 Ω/fase, ~150 mA no datasheet do SMS6-F40): nunca alimentar com 5 V; manter 3,0-3,3 V.
- **Nunca** ligar o motor direto nos GPIOs — sempre pela ponte H (MX1508, "L298N SMD" da AutoCore).
- Depois de cada movimento, **soltar as bobinas** (`stepper_release()`, IN = 0/0). Bobina energizada
  parada esquenta; o mecanismo é que segura o pino. (Testado na bancada em 2026-11-18 mantendo as
  duas bobinas ligadas: o motor ficou bem quente, ~330 mA contínuos. Se precisar travar por
  eletrônica, só com PWM de retenção, não corrente total.)
- Evitar GPIOs 0, 2, 5, 12, 15 (strapping/boot), 6-11 (flash) e 34-39 (só entrada).
- Com vários motores: fonte de 3,3 V separada para eles, com GND comum.
- Se o motor só vibra e não gira, os pares de bobina estão trocados (medir ~20 Ω entre os fios da
  mesma bobina); se perde passos, aumentar `STEPPER_STEP_PERIOD_US`.

## Economia de Tokens — Comandos com Saída Longa

Antes de executar um comando que possa produzir saída longa/cara em tokens (logs extensos, builds
completos, dumps de dados), **filtre a saída** (`| tail -30`, greps direcionados) ou peça ao usuário
que rode e cole o trecho relevante. Nunca rode um build do ESP-IDF sem filtro — o log completo do
CMake/ninja estoura o contexto. Padrão que funciona:

```bash
bash .claude/idf-build.sh build 2>&1 | grep -v '^-- ' | grep -iE 'error|warning|Project build|binary size|fail'
```

## Como Buildar

Use sempre o wrapper — **não monte o comando `idf.py` na mão**:

```bash
bash .claude/idf-build.sh                    # build
bash .claude/idf-build.sh reconfigure build  # após adicionar/remover um .c
bash .claude/idf-build.sh flash monitor
bash .claude/idf-build.sh fullclean
```

O target é o `esp32` (`bash .claude/idf-build.sh set-target esp32` só na primeira vez).

**Adicionou ou removeu um `.c`? Rode `reconfigure` antes.** O `main/CMakeLists.txt` coleta os fontes
com `file(GLOB_RECURSE ...)`, avaliado em tempo de configuração do CMake. Um `build` puro não enxerga
o arquivo novo → erros de link `undefined reference`. Editar arquivo existente não precisa. Pasta
nova de módulo também precisa entrar em `INCLUDE_DIRS` do mesmo arquivo.

### Por que precisa de um wrapper — as duas pegadinhas

**Pegadinha 1 — `idf.py` é só um *alias*, não um executável no PATH.** O `idf.py` só vira "comando"
porque o script de ativação do ESP-IDF define um alias de shell. O `Bash` do agente roda de forma
não-interativa, e nesse modo a expansão de aliases fica desligada → `idf.py: comando não encontrado`.
Solução: chamar o Python do venv + o caminho real do `idf.py`, sem depender do alias.

**Pegadinha 2 — o script de ativação aborta se for *sourced* de dentro de um arquivo.** O
`activate_idf_<ver>.sh` (e o `export.sh` em geral) tem uma checagem `is_sourced()` que inspeciona o
`$0`. Sourcado de dentro de um script, o `$0` é o nome do script, a checagem entende que não foi
"sourced", imprime *"This script should be sourced"* e dá `exit 1`, matando o processo que chamou.
Solução: sourcar o activate dentro de `bash -c '...'` **sem argumentos posicionais** — aí o `$0` cai
no padrão `"bash"` e a checagem passa.

O [.claude/idf-build.sh](.claude/idf-build.sh) já resolve as duas e está versionado. Ele **não tem
caminho de máquina**: a raiz do projeto sai da pasta do próprio script e a versão do IDF é a mais
nova instalada em `~/.espressif` (layout `v<ver>/esp-idf`, `tools/activate_idf_<ver>.sh`,
`tools/python/<ver>/venv`). Para forçar outra versão: `IDF_VER=v6.0.1 bash .claude/idf-build.sh`.
Se algum dos três caminhos não existir, o script aborta dizendo qual.

**Trocou a versão do IDF? Rode `fullclean` antes do build** — o `build/` guarda em cache os caminhos
da versão anterior.

### O que NÃO fazer

- ❌ Rodar `idf.py ...` direto — o alias não existe fora do shell interativo.
- ❌ Sourcar o `export.sh` do esp-idf num script — além da pegadinha 2, em algumas instalações ele
  procura um venv em `~/.espressif/python_env/...` que não existe e aborta. Use o `activate_idf_*.sh`.
- ❌ `set -e`/`set -u` **antes** de sourcar o activate — ele referencia variáveis não definidas e
  aborta (exit 127).
- ❌ Confiar no `.vscode/settings.json` como fonte dos caminhos (`idf.currentSetup` pode apontar para
  versão antiga). Confira em `~/.espressif`.
- ❌ Usar `driver` em `REQUIRES` para GPIO: no IDF 6 o `driver/gpio.h` vem de `esp_driver_gpio`.

## Estrutura do Projeto

```
main/
  main.c              só app_main + app_init (ordem de dependência) + laço do botão de passo
  board_config.h      pinos e constantes de hardware, force-included em todo .c do main
  stepper/            motor de passo bipolar: GPIOs, sequência de fases, anda N passos, solta
  braille_pin/        um ponto braille: estado cima/baixo, sobe/desce via stepper
  button/             botão com debounce por polling (BUTTON_POLL_MS), ativo em nível baixo
.claude/idf-build.sh  wrapper de build
```

Camadas, de cima para baixo: `main` → `braille_pin` → `stepper`. Cada camada só conhece a de baixo:
`braille_pin` sabe quantos passos separam as posições estáveis, mas nada de GPIO ou fase; `stepper`
nada sabe de braille. Uma cela 2x3 será um conjunto de seis `braille_pin_t`.

- Cada módulo é uma pasta `main/<modulo>/` com `<modulo>.h` (API pública) e `<modulo>.c`
  (implementação).
- Módulo por **instância** (`stepper_t`, `braille_pin_t`, alocados pelo chamador), não por singleton
  — é o que permite vários motores/pinos.
- `*_init()` retorna `uint8_t` (0 sucesso, 1 falha), com `ESP_LOGE` no próprio módulo; é chamado por
  `app_init()` em [main/main.c](main/main.c), na ordem de dependência. Módulo novo entra lá, com log
  de erro no mesmo padrão dos existentes.
- Módulo novo = pasta nova em `main/`, entrada em `INCLUDE_DIRS` do
  [main/CMakeLists.txt](main/CMakeLists.txt) e `reconfigure`.
- O `main.c` não tem lógica de motor: só inicializa e chama a camada de cima.

## Comunicação Entre Tasks — Comandos por Fila

> **Ainda não aplicado.** Hoje `stepper` e `braille_pin` são **síncronos**: executam e bloqueiam
> (~22 ms por movimento a 500 pps) na task de quem chama, e há uma task só (`app_main`). O padrão abaixo passa a
> valer quando houver mais de uma task — em especial na cela de 6 pinos, onde mover um pino por vez
> (como no Hackaday, ~1 W por vez) pede um único dono do acionamento.

**Regra central:** um módulo que detém estado e roda uma task própria é o **único escritor** desse
estado. Nenhuma outra task escreve nele direto. Quem quer uma mudança chama uma **função pública
nomeada pela ação**, que monta o evento e o entrega à fila do módulo. O **mecanismo (fila, tipos de
evento) nunca aparece no header** — nada de um `<modulo>_send_event()` genérico que obrigue o
chamador a montar a struct.

Ganhos: uma porta de entrada por ação; ponto natural para validar antes de enfileirar; estado
serializado numa task só (sem mutex espalhado); transporte trocável sem tocar em quem chama.

### Anatomia (5 partes)

1. **Tipos do evento vivem no `.c`**, nunca no `.h`:
   `<mod>_event_type_t` (enum, um valor por ação), `<mod>_event_data_u` (union, um campo por ação
   que carrega dado) e `<mod>_event_t` (`{ type, data }`).
2. **Fila `static`** no `.c` (`static QueueHandle_t <mod>_event_queue`), tamanho em
   `#define <MOD>_QUEUE_LEN` **com o cálculo documentado no `@brief`**, criada no `<mod>_init()`,
   documentada como "escrita só por `send_event()`".
3. **Um helper `static send_event(const <mod>_event_t *event)`** — único ponto do arquivo que chama
   `xQueueSend`, sempre sem bloquear, logando e devolvendo erro quando a fila está cheia. O retorno
   é `<mod>_status_t` quando há enum de motivos de recusa, `uint8_t` quando só existe aceito/cheio,
   `void` quando o descarte é silencioso por natureza.
4. **Um wrapper público por ação**, nomeado pelo que se pede (não pelo evento): `<mod>_set_x()`,
   `<mod>_cancel_x()`, `<mod>_notify_x()`. Corpo típico: monta o literal com inicializadores
   designados e chama `send_event`.
5. **Task consumidora** com `xQueueReceive` + `switch (event.type)`, um `handle_<evento>()` por case.

```c
/* braille_cell.c — transporte e tipos, invisiveis para quem chama */
typedef enum
{
    BRAILLE_CELL_EVENT_SET_PATTERN,
} braille_cell_event_type_t;

typedef union
{
    uint8_t pattern;
} braille_cell_event_data_u;

typedef struct
{
    braille_cell_event_type_t type;
    braille_cell_event_data_u data;
} braille_cell_event_t;

static braille_cell_status_t send_event(const braille_cell_event_t *event)
{
    if (braille_cell_event_queue == NULL || xQueueSend(braille_cell_event_queue, event, 0) != pdTRUE)
    {
        ESP_LOGE(TAG, "Cell queue full, event %d dropped", event->type);
        return BRAILLE_CELL_BUSY;
    }

    return BRAILLE_CELL_OK;
}

braille_cell_status_t braille_cell_set_pattern(uint8_t pattern)
{
    if (pattern > BRAILLE_PATTERN_MAX)
    {
        return BRAILLE_CELL_INVALID_PATTERN;
    }

    braille_cell_event_t event = {
        .type = BRAILLE_CELL_EVENT_SET_PATTERN,
        .data.pattern = pattern,
    };

    return send_event(&event);
}
```

```c
/* Correto — o header diz o que se pede, nao como chega la */
braille_cell_status_t braille_cell_set_pattern(uint8_t pattern);

/* Errado — obriga todo chamador a conhecer a fila e os tipos do evento */
uint8_t braille_cell_send_event(const braille_cell_event_t *event);
```

### Regras que acompanham o padrão

- **Validar no wrapper, não no handler.** O wrapper roda na task do chamador, então é onde dá para
  devolver veredito na hora. Só entra na fila o que já foi aceito; o handler confia no evento.
  Códigos de erro viram um `enum` no `.h` (`<mod>_status_t`, valores `<MOD>_<MOTIVO>`), com
  `<MOD>_OK = 0` = aceito.
- **Envio nunca bloqueia — e o retorno é checado.** `portMAX_DELAY` no `xQueueSend` **não** evita
  perda: troca perda visível por travamento invisível do produtor, e o retorno vira sempre `pdTRUE`,
  então a anomalia nunca aparece no log. Sempre `0` no timeout, com `ESP_LOGE` e código de erro no
  `send_event()`. De ISR, `xQueueSendFromISR` + `portYIELD_FROM_ISR`.
- **Dimensione a fila e documente a conta.** `<MOD>_QUEUE_LEN` sai do pior burst somado de todos os
  produtores durante o pior tempo de serviço do consumidor, com margem, escrito no `/** @brief */`
  do `#define`. Fila bem dimensionada que enche é **anomalia** — consumidor travado ou produtor em
  loop —, não backpressure normal.
- **Propague a falha só para quem sabe reagir.** Wrapper devolve status quando o chamador pode
  reportar; quando não tem o que fazer com o erro (vindo de ISR/timer) fica `void` com
  `(void)send_event(&event)` — o log do `send_event` já registra.
- **Nunca bloqueie em contexto de callback/event loop** (timer, ISR, callbacks do IDF).
- **Handler não pode bloquear longo.** Fila nenhuma salva consumidor preso segundos dentro de um
  handler. Espera por hardware externo leva timeout obrigatório; trabalho longo sai do handler para
  task própria. Cuidado específico deste projeto: `stepper_move()` bloqueia ~22 ms (10 passos a 500
  pps) ou ~114 ms (fuso, 56 passos) — é o pior tempo de serviço a usar no dimensionamento da fila.
- **Comando vai pela fila; leitura é getter.** Os getters copiam o último valor publicado pela task
  dona — assíncrono na escrita, síncrono na leitura. Se o único leitor tolera um valor rasgado, a
  cópia sem lock é aceitável — documente isso na Doxygen do getter.
- **Timeout do `xQueueReceive` como tick periódico**, quando o módulo precisa reavaliar algo de
  tempos em tempos.
- **Só sobrevive no header** o tipo que aparece na assinatura de alguma função pública.
- **Nome de tipo tomado pelo IDF:** quando o nome canônico já existe no ESP-IDF, use outro e explique
  no comentário.

### Quando NÃO usar

- Módulo sem task própria e sem estado mutável compartilhado: chamada direta, sem fila.
- Resposta imediata obrigatória ao chamador: função síncrona com mutex próprio, declarada no bloco
  "síncronas" do header.
- Notificação de mão única de baixa frequência para quem não detém estado: callback registrado
  (`<mod>_set_cb`) já basta.

## Configuração de Hardware — `board_config.h`

Pinos, prioridades de task e stacks ficam **exclusivamente** em
[main/board_config.h](main/board_config.h), que é force-included em toda unidade de tradução do
`main` (ver `target_compile_options` em [main/CMakeLists.txt](main/CMakeLists.txt)). Também mora lá
o que descreve o hardware/mecanismo: tempo por passo (`STEPPER_STEP_PERIOD_US`) e passos de curso do
pino (`BRAILLE_PIN_STROKE_STEPS`: 10 = came excêntrico, 56 = fuso M1x0,25; o motor tem 20 passos/volta, 18°/passo, medido).

- **Não** hardcode número de GPIO, prioridade ou stack dentro de um módulo — defina lá e use a macro.
- Constantes que só interessam ao módulo (timeouts, tamanhos de buffer, limiares) ficam como
  `#define` no próprio `.c`, com `/** @brief ... */`.
- Os módulos recebem pinos e tempos por struct de configuração (`stepper_config_t`); quem lê as
  macros do `board_config.h` é o `app_init()`.

## Padrão de Código

Leia esta seção **antes de escrever ou editar qualquer `.c`/`.h`**.

### Estrutura do arquivo `.h`

```
1. Docblock de arquivo   — /** @file ... @brief ... */
2. #pragma once
3. Includes              — <sys> primeiro, depois "projeto"
4. Typedefs              — structs e enums com campos documentados
5. Declarações públicas  — funções com Doxygen completo (@brief, @param, @return, @note)
```

Exemplo real: [main/stepper/stepper.h](main/stepper/stepper.h).

### Estrutura do arquivo `.c`

```
1. Includes              — <std> → FreeRTOS → ESP-IDF → "proprio_header.h" por último
2. #defines              — cada um com /** @brief ... */
3. Typedefs internos     — enums ou structs usados só no .c (evento, união, struct do evento)
4. Static consts         — TAG e constantes com /** @brief ... */
5. Statics               — variáveis de estado com /** @brief ... */ e detalhe se necessário
6. Declarações statics   — forward declarations com Doxygen completo (EXCETO ISRs)
7. Handlers ISR          — definidos diretamente acima do init, sem forward declaration
8. Definições públicas   — na ordem em que aparecem no .h
9. Definições statics    — na mesma ordem das declarações do passo 6
```

Exemplo real: [main/stepper/stepper.c](main/stepper/stepper.c).

**Exceção: handlers de interrupção (ISR).** Funções `IRAM_ATTR` **não podem ter forward declaration**
— o compilador gera erro quando o atributo é repetido entre declaração e definição. Não declarar o
ISR junto com as outras forward declarations; defini-lo diretamente acima da função `*_init()` que
faz o `gpio_isr_handler_add()`, com o Doxygen na própria definição. O corpo é mínimo: dar
semáforo/fila `FromISR` e retornar.

### Ordem das funções públicas (`.h` e `.c`)

Ordene por camada — do que age para o que só lê:

1. **Lifecycle** — `<mod>_init()`
2. **Comandos** — pedem uma ação; enfileiram um evento e voltam
3. **Getters** — leem o estado que o módulo detém
4. **Síncronas** — executam na task do chamador (bloqueiam, usam mutex próprio)

Dentro de cada bloco, agrupe por assunto. O `.c` repete exatamente a ordem do `.h`. **Sem comentário
de banner** separando os blocos: a sequência e a Doxygen de cada declaração já são a estrutura.

Uma função pode acabar num bloco que não é o óbvio pela assinatura — decida pelo que ela é para quem
chama. Hoje `stepper_move()`, `stepper_release()`, `braille_pin_raise()` e `braille_pin_lower()` são
**síncronas** (bloqueiam o chamador); `braille_pin_is_raised()` é getter e vem antes delas.

### Ordem e nomes das funções `static` (`.c`)

Regra do *stepdown*: descendo no arquivo, cada função é menos abstrata que a anterior, e **logo
depois de cada função vêm as funções que ela chama** — em profundidade, não em largura. A ordem é a
mesma nas forward declarations do topo e nas definições. Em `stepper.c`: `apply_phase` (chamada por
`stepper_move`) e logo depois `set_coil` (chamada por ela, e reusada por `stepper_release`).

Helper chamado por várias funções: vai depois do **primeiro** chamador na ordem de leitura, não
repetido nem jogado para o fim.

**Exceção — o miolo do módulo vem antes dos helpers de `<mod>_init()`.** Quem abre o arquivo procura
o comportamento do módulo, não a configuração de periférico. Ordem: **plumbing e miolo primeiro**
(`send_event` → `<mod>_task` → handlers com seus callees, em profundidade), **helpers de `init`
depois**, e callbacks de timer/evento do IDF por último. Dentro de cada bloco vale o stepdown normal.
Plumbing é mais que o `send_event`: helper genérico que todos os handlers usam fica no topo junto dele.

**Nomes sem prefixo de módulo** — dentro do `.c` o arquivo já é o contexto, e `stepper_apply_phase`
só repete o nome do arquivo. Handler de evento da fila é `handle_<evento>`, nunca
`<evento>_event_handler`. Duas exceções mantêm o prefixo:

- **Entry point de task FreeRTOS** (`<mod>_task`): o nome bate com a string passada ao
  `xTaskCreate` e com a TAG de log.
- **ISR** (`<mod>_isr_handler`): é o nome que aparece no `gpio_isr_handler_add()` e no backtrace de
  um crash em interrupção, onde o arquivo não está à vista.

### Comentários e Doxygen

**Regra principal:** documentar no ponto de declaração; nunca duplicar na implementação. Em `.h`,
documentar a API pública. Em `.c`, documentar os elementos internos (`static`) na declaração no topo
— exceto ISR, documentado na própria definição.

**Comentar:** funções públicas no `.h`; tipos públicos (`typedef struct`, `enum`) e seus campos;
`#define` relevantes (limites, timeouts, tamanhos de fila, com o cálculo); variáveis `static`
importantes no `.c` (estado, handles, buffers); declarações de funções `static` no topo do `.c`.

**Não comentar:** o óbvio linha a linha (`// increment i`); a mesma função em dois lugares;
comentário genérico sem contexto técnico; bloco enorme de comentário para lógica simples.

### Comentários inline

Use `/* */` (não `//`) dentro do corpo de funções.

Ao **gerar** código, documente apenas o **porquê**, não o **o quê** — o código já diz o quê. A
posteriori, ao revisar, é válido acrescentar comentários de **o quê** sob demanda.

```c
/* Correct — explains a non-obvious decision */
/* the FreeRTOS tick (10 ms by default) is far too coarse for steps of ~2 ms */
esp_rom_delay_us(motor->config.step_period_us);

/* Wrong — repeats what the code already says */
/* wait for the step period */
esp_rom_delay_us(motor->config.step_period_us);
```

### Resumo das regras de formatação

| Item | Regra |
|---|---|
| Idioma da área de código | **Inglês** — comentários, identificadores, strings de log. Só este CLAUDE.md e o README são em PT |
| Ordem no `.h` | docblock `@file` → `#pragma once` → includes → typedefs → declarações públicas |
| Ordem no `.c` | includes → defines → typedefs internos → static consts → statics → forward declarations → ISRs → definições públicas → definições statics |
| Ordem das públicas | init → comandos → getters → síncronas; o `.c` repete a ordem do `.h` |
| Ordem das statics | *Stepdown* — callees logo depois do chamador, em profundidade; miolo antes dos helpers de `init`, callbacks do IDF por último. Mesma ordem nas declarações e nas definições |
| Documentação | Público no `.h`; `static` na declaração no topo do `.c`; ISR na própria definição; implementação limpa |
| Comentários inline | `/* */`, nunca `//`; explique o **porquê**, não o **o quê** |
| Comentário de banner | **Não usar** — a ordem e a Doxygen já são a estrutura |
| Chave de abertura | Linha própria |
| `if`/`else` sem chaves | Não usar — sempre `{}` |
| Loop infinito | `for (;;)` |
| Retorno de init | `uint8_t` — 0 sucesso, 1 falha |
| Tag de log | `static const char *TAG = "modulo"`; módulo com task: `"modulo_task"` |
| Task FreeRTOS | nome `modulo_task` |
| Prefixo de variável static | Nenhum — nome descritivo com contexto do módulo (`step_sequence`, `battery_semaphore`) |
| Prefixo de função static | Nenhum — exceto entry point de task (`<mod>_task`) e ISR (`<mod>_isr_handler`) |
| Handler de evento | `handle_<evento>`, nunca `<evento>_event_handler` |
| Includes de sistema | `<angulares>` antes dos `"aspas"` |
| Mecanismo de fila no header | **Não expor** — cada ação vira um wrapper nomeado; tipos do evento ficam no `.c` |
| ISR (`IRAM_ATTR`) | **Sem forward declaration** — definir direto acima do `*_init()`; corpo mínimo |
| Casts | Não inserir por iniciativa própria — só quando o compilador exigir |
| `REQUIRES` de GPIO | `esp_driver_gpio`, não `driver` |

### Casts

Não inserir casts (`(uint64_t)`, `(size_t)`, `(time_t)` etc.) por iniciativa própria. A maioria é
redundante na plataforma alvo e só polui o código.

Antes de adicionar um cast, verifique: o tipo de origem já satisfaz o destino (se sim, sem cast)? O
compilador realmente avisa de narrowing (se não, sem cast)? É um dos casos legítimos abaixo?

**Exceções — sempre manter:**

- **Pointer-sign** (`(char *) ↔ (uint8_t *)`) quando a API força.
- **Narrowing real** de tipo maior para menor (`(uint8_t)` sobre um `int` de cálculo).
- **Sentinel e adição de `const`**: `(time_t)-1`, `(const uint8_t *)`.
- **Format-string de printf** quando o tipo do argumento não bate com o especificador (ex.:
  `(int)len` para `%d` quando `len` é `size_t`).

### Idioma: área de código em inglês

Toda a **área de código** é escrita em **inglês**: comentários (docblocks Doxygen, comentários de
campo `/**< ... */`, inline `/* ... */`), identificadores (funções, variáveis, `#define`, tipos, tags
de log, nomes de task FreeRTOS) e strings de log/erro (`ESP_LOGI/W/E`, `printf`, mensagens passadas a
APIs do ESP-IDF). **Este CLAUDE.md e o README são os únicos documentos em português.**

Não misturar os dois idiomas dentro de um arquivo de código.

```c
/* Wrong — comment and log string in Portuguese */
/* solta as bobinas depois do movimento */
ESP_LOGI(TAG, "Subindo");

/* Correct — code area in English */
/* releases the coils after the move */
ESP_LOGI(TAG, "Raising (%d steps)", pin->stroke_steps);
```

Nenhum texto de usuário final vive no firmware, então **não há exceção** de português na área de
código.

## Git — Commits

O projeto ainda não é um repositório git. Não commitar nada sem o usuário pedir. Quando pedir, seguir
o estilo: mensagem em inglês, formato `tipo: descrição`, com o verbo na 3ª pessoa do singular —
`feat: adds braille pin module`, `fix: releases coils after a move`, `refactor: splits stepper from
braille pin`, `docs: documents the wiring`. Tipos em uso: `feat`, `fix`, `refactor`, `docs`.
