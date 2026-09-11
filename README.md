# ECOM042-monitoria

## CI

PRs disparam `.github/workflows/pr-check.yml`, que chama o workflow
reutilizável do [ECOM042-CI](https://github.com/JoaoMatheusND/ECOM042-CI)
(clang-format, build e testes internos via Twister) usando o nome da branch
do PR pra identificar a atividade. Detalhes em `ECOM042-CI/README.md`.

Precisa de um Actions secret `ATIVIDADES_TOKEN` configurado neste repo
(Settings → Secrets and variables → Actions) — token com acesso de leitura
ao repo privado [ECOM042-atividades](https://github.com/JoaoMatheusND/ECOM042-atividades),
que guarda o gabarito das atividades.
