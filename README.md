# ECOM042-monitoria

## CI

PRs disparam `.github/workflows/pr-check.yml`, que chama o workflow
reutilizável do [ECOM042-CI](https://github.com/JoaoMatheusND/ECOM042-CI)
(clang-format, build e testes internos via Twister) usando o nome da branch
do PR pra identificar a atividade. Detalhes em `ECOM042-CI/README.md`.
