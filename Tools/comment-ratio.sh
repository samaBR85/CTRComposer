#!/bin/sh
# Quanto do codigo e comentario explicativo.
#
# Existe porque este projeto foi criticado publicamente por excesso de comentario - o tipo que
# um assistente de IA produz sozinho se ninguem olhar. A regra esta no CLAUDE.md; isto e o
# termometro, para a conversa ser sobre um numero e nao sobre gosto.
#
# NAO conta:
#   - linhas em branco
#   - o cabecalho boilerplate de 7 linhas que todo .inc.c carrega (e estrutural, nao prosa)
#
# Faixa saudavel: 10-12%. Acima disso, os comentarios novos precisam ser armadilhas reais
# (uma restricao que o codigo nao mostra sozinho), nao narrativa de desenvolvimento.
#
# Uso:  sh Tools/comment-ratio.sh

set -eu
cd "$(dirname "$0")/.."

awk '
    FILENAME ~ /(plgldr|sysfont)\.c$/ { next }     # bibliotecas de terceiros

    { s = $0; gsub(/^[ \t]+|[ \t]+$/, "", s) }
    s == "" { next }
    { total++ }

    bloco { com++; if (s ~ /\*\//) bloco = 0; next }
    s ~ /^\/\*/ { com++; if (s !~ /\*\//) bloco = 1; next }

    s ~ /^\/\// {
        com++
        # cabecalho padrao dos .inc.c - estrutural, nao conta como prosa
        if (s ~ /====/ || s ~ /MOTOR - voce/ || s ~ /PARTE DO JOGO/ || s ~ /^\/\/ Incluido/ ||
            s ~ /valendo\. Nao e compilado/ || s ~ /Sources\/(engine|plugin)\/\.$/ ||
            s ~ /Sources\/\*\.c e este/) cab++
        next
    }

    END {
        expl = com - cab
        printf "  linhas nao vazias   %6d\n", total
        printf "  comentario          %6d   (%d de cabecalho de arquivo)\n", com, cab
        printf "  explicativo         %6d\n", expl
        printf "  ---------------------------\n"
        printf "  RAZAO               %6.1f%%   (faixa saudavel: 10-12%%)\n", 100 * expl / total
        if (100 * expl / total > 13.0)
            printf "\n  Acima da faixa. Os comentarios novos sao armadilhas reais, ou narrativa?\n"
    }
' Sources/*.c Sources/plugin/*.inc.c Sources/engine/*.inc.c
