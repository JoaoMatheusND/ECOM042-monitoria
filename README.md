# ECOM042-monitoria

## Atividade 02 — Command Pattern

Firmware real recebe comandos de várias fontes (UART, BLE, botão, timer) e
precisa reagir a cada um com uma ação diferente. Do jeito simples, isso vira
um `if/else`/`switch` gigante dentro de `main()`: cresce a cada comando
novo, mistura "quem pediu a ação" com "quem sabe executá-la" e fica cada
vez mais arriscado de mexer sem quebrar um caso que já funcionava.

Sua tarefa é desacoplar isso usando o **Command Pattern** (GoF): o código
que despacha um comando não deve conhecer os comandos concretos que
existem — ele só recebe algo que sabe "executar a si mesmo" e chama isso,
sem saber o que tem por trás. Adicionar um comando novo deve significar
escrever um arquivo novo, não editar a lógica de despacho já existente.

Nenhum arquivo de código é fornecido — cabe a você decidir a interface
(quais arquivos criar, como representar um comando, como estruturar a
tabela) que resolve isso. O CI (`clang-format`, build e testes) roda no seu
PR e valida se a solução está correta.

Como entregar

 Faça um **fork** deste repositório.

No seu fork, crie/trabalhe numa branch com o **mesmo nome da
atividade** (ex.: `Atividade-02`) — o CI identifica qual atividade
corrigir pelo nome da branch do PR.

Implemente sua solução nessa branch e dê push pro seu fork.

Abra um **Pull Request** do seu fork pra este repositório, usando essa
branch como origem.

O CI roda automaticamente no PR (clang-format, build e testes). Se algo
falhar, corrija e dê push de novo na mesma branch — o PR atualiza
sozinho, sem precisar abrir um novo.
