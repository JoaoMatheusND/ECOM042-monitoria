# ECOM042-monitoria

Repositório de atividades práticas da disciplina, em C/Zephyr. Cada
atividade vive numa branch própria (`Atividade-01`, `Atividade-02`, ...),
com o skeleton de código daquela atividade — este branch (`main`) não tem
código, só a explicação de como tudo funciona.

## Como funciona

1. As branches `Atividade-XX` contêm o skeleton de cada atividade (o
   enunciado, no README daquela branch, explica só o problema — sem
   entregar a solução).
2. Aluno faz **fork** deste repositório, cria/usa uma branch com o
   **mesmo nome da atividade** no fork e implementa a solução ali.
3. Aluno abre um **Pull Request** do fork pra este repositório, **contra
   `main`** — é o único branch alvo que dispara o CI (ver abaixo).
4. O CI builda, roda os testes e (quando a atividade tiver) faz uma
   checagem estrutural do código, comentando o resultado direto no PR.

## CI

PRs contra `main` disparam `.github/workflows/pr-check.yml`
(`pull_request_target`, necessário pra secrets chegarem em PR de fork), que
chama o workflow reutilizável do
[ECOM042-CI](https://github.com/JoaoMatheusND/ECOM042-CI) (clang-format,
build e testes internos via Twister, e checagem estrutural opcional via
`check.py`) usando o nome da branch do PR pra identificar a atividade.
Detalhes em `ECOM042-CI/README.md`.

Precisa de um Actions secret `ATIVIDADES_TOKEN` configurado neste repo
(Settings → Secrets and variables → Actions) — token fine-grained,
com acesso de leitura só ao repo privado
[ECOM042-atividades](https://github.com/JoaoMatheusND/ECOM042-atividades),
que guarda o gabarito das atividades.
