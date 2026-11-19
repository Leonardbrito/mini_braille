# Display braille atualizável com micro motor de passo

Iniciação científica. Módulo braille atualizável inspirado no
[projeto do Hackaday](https://hackaday.io/project/191181-electromechanical-refreshable-braille-module),
mas usando um micro motor de passo (com "SM56-F40" impresso; parecido com o Minebea/NMB SMS6-F40, mas o modelo não está confirmado) para mover cada pino.
O mecanismo precisa se autotravar: o motor só é energizado para mudar o pino e depois as
bobinas são desligadas.

**Etapa atual:** acionar UM motor com ESP32 e verificar que ele gira nos dois sentidos.

## Estrutura

```
main/
  main.c              só app_main + app_init + laço do botão de passo
  board_config.h      pinos e constantes de hardware (force-included)
  stepper/            motor de passo bipolar: GPIOs, sequência de fases, anda N passos, solta
  braille_pin/        um ponto braille: estado cima/baixo, sobe/desce via stepper
  button/             botão com debounce (ativo em nível baixo, pull-up interno)
.claude/idf-build.sh  wrapper de build
claude.md             guia de arquitetura e padrão de código
```

Camadas: `main` → `braille_pin` → `stepper`. Uma futura cela 2x3 será um conjunto de seis
`braille_pin_t`. Detalhes de arquitetura e padrão de código no [claude.md](claude.md).

## Hardware

- ESP32 clássico (devkit), ESP-IDF 6.0 (`idf.py`), sem Arduino.
- Driver: módulo "L298N SMD" da AutoCore (B0173), geralmente MX1508. Alimentação 2-10 V,
  lógica aceita 3,3 V. **Não** é o L298N grande com dissipador.
- Motor: bipolar, 2 fases, ~20 Ω por fase, 3 V, **20 passos/volta (18° por passo, medido na bancada)** — o datasheet do SMS6-F40 diz 40.

| ESP32  | Driver        |
|--------|---------------|
| GPIO15 | IN1 (bobina A)|
| GPIO4  | IN2 (bobina A)|
| GPIO5  | IN3 (bobina B)|
| GPIO18 | IN4 (bobina B)|
| GPIO27 | Botão de passo (outro terminal no GND) |
| 3V3    | "+"           |
| GND    | "−" (GND comum obrigatório) |

Fios do motor: MOTOR-A nos dois fios de uma bobina, MOTOR-B nos da outra (medir com
multímetro: ~20 Ω entre os fios da mesma bobina).

Cuidados:
- Eletrolítico de 47-100 µF (+ no 3,3 V) e cerâmico de 100 nF entre "+" e "−" do módulo,
  perto dele, para evitar brownout do ESP32.
- Não alimentar o motor com 5 V. Não inverter + e − (sem proteção de polaridade).
- Não ligar o motor direto nos GPIOs.
- Evitar GPIOs 0, 2, 5, 12, 15 (strapping), 6-11 (flash), 34-39 (só entrada).
- Com vários motores: fonte de 3,3 V separada para os motores, GND comum.

## Compilar e gravar

```bash
bash .claude/idf-build.sh set-target esp32      # só na primeira vez
bash .claude/idf-build.sh reconfigure build     # reconfigure ao adicionar/remover um .c
bash .claude/idf-build.sh flash monitor
```

(Num terminal com o ESP-IDF ativado, `idf.py build flash monitor` também funciona.)

O teste anda **1 passo a cada aperto** do botão em `STEP_BUTTON_GPIO` (GPIO27, botão entre o GPIO e o GND,
pull-up interno; ver [board_config.h](main/board_config.h)). Prenda uma fita no eixo: 10 apertos = meio giro.
A contagem de passos aparece no monitor serial.

Se o motor só vibrar e não girar, os pares de bobina estão trocados ou uma bobina está
invertida: meça de novo com o multímetro. Se perder passos, aumente `STEPPER_STEP_PERIOD_US`
em [board_config.h](main/board_config.h).

## Mecanismo (ainda não decidido)

- **A) Fuso + porca** (M1x0,25, ~56 passos para 0,7 mm): autotravante, lento, difícil de fabricar.
- **B) Came excêntrico** (excentricidade ~0,35 mm, meia volta = 10 passos): o mais parecido
  com o Hackaday, recomendado para o protótipo de 1 pino.
- **C) Barra deslizante com rampa e patamar**: motor longe do pino; a mais promissora para a
  cela completa, já que o motor (Ø6 mm) não cabe sob pontos espaçados de 2,5 mm.

## Próximos passos

1. Comprar o módulo, capacitores e barra de pinos; soldar.
2. Identificar as bobinas e confirmar o modelo (a contagem de passos medida, 20/volta, difere do SMS6-F40).
3. Montar a ligação, gravar e testar.
4. Ajustar `STEPPER_STEP_PERIOD_US`.
5. Prototipar 1 pino com came excêntrico (impressão 3D/resina), depois a cela.
