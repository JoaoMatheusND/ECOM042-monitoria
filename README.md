# ECOM042-monitoria

## Atividade 01 — Hello World

Antes de escrever qualquer lógica embarcada, precisa validar que a
toolchain inteira funciona: compilador cruzado, build e execução do
binário no board (ou simulado, no caso do `native_sim`). É o primeiro
contato com o fluxo que vai se repetir em toda atividade daqui pra frente.

Sua tarefa é imprimir, via console, uma mensagem confirmando isso, usando
a API de log do Zephyr (`printk`), já implementado, junto de uma informação que identifica
pra qual board o firmware foi compilado.

## Como entregar

Faça um **fork** deste repositório.

No seu fork, crie/trabalhe numa branch com o **mesmo nome da
atividade** (ex.: `Atividade-01`) — o CI identifica qual atividade
corrigir pelo nome da branch do PR.

Implemente sua solução nessa branch e dê push pro seu fork.

Abra um **Pull Request** do seu fork pra este repositório, usando essa
branch como origem.

O CI roda automaticamente no PR (clang-format, build e testes). Se algo
falhar, corrija e dê push de novo na mesma branch — o PR atualiza
sozinho, sem precisar abrir um novo.
