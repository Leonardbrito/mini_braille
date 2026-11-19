#!/usr/bin/env bash
# Build de firmware ESP-IDF a partir de sessao nao-interativa.
#
# Uso:  bash .claude/idf-build.sh [args do idf.py...]
#       sem args -> "build"   |   ex.: `bash .claude/idf-build.sh flash`
#
# Versao do IDF: IDF_VER=v6.1 bash .claude/idf-build.sh  (padrao: a mais nova em ~/.espressif)

_ESP="$HOME/.espressif"
_VER="${IDF_VER:-$(ls -d "$_ESP"/v*/esp-idf 2>/dev/null | sed 's#.*/\(v[^/]*\)/esp-idf#\1#' | sort -V | tail -1)}"

export _ACT="$_ESP/tools/activate_idf_$_VER.sh"                       # script de ativacao
export _PY="$_ESP/tools/python/$_VER/venv/bin/python"                 # python do venv
export _IDF="$_ESP/$_VER/esp-idf/tools/idf.py"                        # idf.py real
export _PROJ="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"       # raiz do projeto (-C)
export _ARGS="${*:-build}"                                            # args (padrao: build)

for _f in "$_ACT" "$_PY" "$_IDF"; do
    [ -e "$_f" ] || { echo "idf-build: '$_f' nao existe (IDF_VER=$_VER)" >&2; exit 1; }
done

exec bash -c '. "$_ACT" >/dev/null 2>&1; exec "$_PY" "$_IDF" -C "$_PROJ" $_ARGS'
