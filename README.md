# O Jogo do Psíquico

- **Juiz (servidor)** — `Juiz_Server.c`
- **Jogador (cliente)** — `Jogador_Cliente.c`

---

### 1. Pré-requisitos

- [MinGW](https://www.mingw-w64.org/) com `gcc` e suporte a `pthread`.
- Garanta que o `gcc` esteja no `PATH` (teste com `gcc --version`).

### 2. Compilar

Abra o terminal (cmd ou PowerShell) na pasta do projeto e rode:

```bash
gcc -Wall Juiz_Server.c    -o Juiz_Server.exe    -lpthread -lws2_32
gcc -Wall Jogador_Cliente.c -o Jogador_Cliente.exe -lpthread -lwsock32
```

### 3. Executar

Primeiro o **juiz** (servidor), escolhendo uma porta livre (ex.: `5000`):

```bash
Juiz_Server.exe 5000
```

Depois, em **dois terminais separados**, inicie os **dois jogadores**,
informando o IP do servidor e a mesma porta:

```bash
Jogador_Cliente.exe 127.0.0.1 5000
```

> Use `127.0.0.1` se tudo estiver na mesma máquina.
> Em rede, troque pelo IP da máquina onde o juiz está rodando.

## Como funciona o jogo

### Objetivo

Cada jogador escolhe secretamente um **objeto**. O objetivo é **adivinhar o
objeto que o oponente escolheu** antes que ele adivinhe o seu. Quem acertar
primeiro vence.

O **juiz** é quem conhece os dois objetos secretos e decide quem venceu. Os
jogadores nunca falam diretamente entre si para checar o palpite: tudo passa
pelo juiz.

### Fluxo de uma partida

1. O juiz é iniciado e aguarda **2 jogadores** se conectarem.
2. Cada jogador define seu objeto secreto com o comando `/secret`.
3. Quando os **dois** objetos estão definidos, o juiz libera os palpites.
4. Os jogadores enviam palpites com `/guess` **a qualquer momento**
   (não há turnos — é simultâneo).
5. Ao acertar o objeto do oponente, o jogador é declarado **vencedor** e a
   partida termina.
6. A qualquer momento os jogadores podem conversar: qualquer texto que não
   seja um comando é enviado como **chat** para o oponente, via juiz.

### Comandos do jogador (no terminal do cliente)

| Comando            | O que faz                                              |
|--------------------|--------------------------------------------------------|
| `/secret <objeto>` | Define o seu objeto secreto.                           |
| `/guess <objeto>`  | Tenta adivinhar o objeto do oponente.                  |
| `/END`             | Sai do jogo e encerra a conexão.                       |
| `<qualquer texto>` | Enviado como mensagem de chat para o outro jogador.    |

A comparação de objetos é **sem distinção de maiúsculas/minúsculas** e ignora
espaços no início e no fim (ex.: `  Cadeira ` é igual a `cadeira`).

## Protocolo de comunicação

Todas as mensagens são texto, uma por linha, terminadas em `\n`, no formato
`TIPO|conteúdo`.

### Cliente → Juiz

| Mensagem          | Significado                               |
|-------------------|-------------------------------------------|
| `SECRET|<objeto>` | Define o objeto secreto do jogador.       |
| `GUESS|<objeto>`  | Palpite sobre o objeto do oponente.       |
| `CHAT|<texto>`    | Mensagem de chat para o oponente.         |
| `END`             | Encerra a conexão.                        |

### Juiz → Cliente

| Mensagem             | Significado                                     |
|----------------------|-------------------------------------------------|
| `MSG|<texto>`        | Mensagem informativa / estado do servidor.      |
| `CHAT|J<n>|<texto>`  | Chat recebido do jogador `n`.                   |
| `RESULT|MISS`        | O último palpite errou.                         |
| `WIN|<n>`            | Você venceu (jogador `n` acertou).              |
| `LOSE|<n>`           | Você perdeu (jogador `n` acertou primeiro).     |

> O cliente traduz os comandos amigáveis (`/secret`, `/guess`, chat) para o
> protocolo acima automaticamente, então o jogador não precisa digitar
> `SECRET|`, `GUESS|` etc. na mão.

---

## Integrantes do grupo

| Nome                   | RA        |
|------------------------|-----------|
| Arthur Santos Correia  | 202210339 |
| Leonardo Trevisan Nery | 202210149 |
| Luiz Furlanis          | 202510005 |
