# このビルドはWindows上でVulkan経路を検査するだけで、Linux版ではない
message(FATAL_ERROR "SPACEYAKUZA_HOST_SMOKE is not a distribution build; package Mac/Linux/Web on its actual target toolchain")
