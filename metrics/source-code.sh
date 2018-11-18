#!/usr/bin/env bash
#
# Count lines of code in various ways.
#
# Usage:
#   ./count.sh <function name>

set -o nounset
set -o pipefail
set -o errexit

filter-py() {
  grep -E -v '__init__.py$|_test.py$'
}

# Oil-only would exclude core/legacy.py, etc.
oil-osh-files() {
  { ls {bin,oil_lang,osh,core,frontend}/*.py native/*.c */*.asdl; } |
    filter-py | grep -E -v '_gen.py$|test_lib.py'
}

# cloc doesn't understand ASDL files.
# Use a wc-like format, filtering out blank lines and comments.
asdl-cloc() {
  python -c '
import sys

total = 0
for path in sys.argv[1:]:
  num_lines = 0
  with open(path) as f:
    for line in f:
      line = line.strip()
      if not line or line.startswith("--"):
        continue
      num_lines += 1

  print "%5d %s" % (num_lines, path)
  total += num_lines

print "%5d %s" % (total, "total")
' "$@"
}

oil-osh-cloc() {
  echo 'OIL AND OSH (non-blank non-comment lines)'
  echo
  oil-osh-files | xargs cloc --quiet "$@"

  # NOTE: --csv option could be parsed into HTML.
  # Or just sum with asdl-cloc!

  echo
  echo 'ASDL SCHEMAS (non-blank non-comment lines)'
  asdl-cloc */*.asdl
}

# TODO: Sum up all the support material.  It's more than Oil itself!  Turn
# everything into an array.  An hash table of arrays would be useful here.
all() {
  echo 'BUILD AUTOMATION'
  wc -l build/*.{mk,sh,py} Makefile *.mk configure install |
    filter-py | sort --numeric
  echo

  echo 'TEST AUTOMATION'
  wc -l test/*.{sh,py,R} | filter-py | sort --numeric
  echo

  echo 'RELEASE AUTOMATION'
  wc -l devtools/release.sh | sort --numeric
  echo

  echo 'BENCHMARKS'
  wc -l benchmarks/*.{sh,py,R} | sort --numeric
  echo

  echo 'METRICS'
  wc -l metrics/*.{sh,R} | sort --numeric
  echo

  echo 'SPEC TESTS'
  wc -l spec/*.test.sh | sort --numeric
  echo

  echo 'GOLD TESTS'
  wc -l gold/*.sh | sort --numeric
  echo

  echo 'ASDL'
  ls asdl/*.py | filter-py | grep -v -E 'arith_|tdop|_demo' |
    xargs wc -l | sort --numeric
  echo

  echo 'CODE GENERATORS'
  wc -l asdl/gen_*.py */*_gen.py | sort --numeric
  echo

  echo 'GENERATED CODE'
  wc -l _devbuild/gen/*.{py,h} | sort --numeric
  echo

  echo 'TOOLS'
  ls tools/*.py | filter-py | xargs wc -l | sort --numeric
  echo

  echo 'WEB'
  ls web/*.js web/*/*.{js,py} | xargs wc -l | sort --numeric
  echo

  echo 'BORROWED FROM STDLIB'
  wc -l pylib/*.py | filter-py | sort --numeric
  echo

  echo 'OTHER UNIT TESTS'
  wc -l {build,test,asdl,pylib,tools}/*_test.py | sort --numeric
  echo

  echo 'OIL UNIT TESTS'
  wc -l {oil_lang,osh,frontend,core,ovm2,native,tools}/*_test.py | sort --numeric
  echo

  echo 'OIL AND OSH'
  oil-osh-files | xargs wc -l | sort --numeric
  echo

  echo 'OVM2'
  wc -l ovm2/*.{py,cc} | filter-py | sort --numeric
  echo

  return
  # TODO: Import docs

  echo 'DOCS'
  wc -l README.md doc/* | sort --numeric
  echo
}

# count instructions, for fun
instructions() {
  # http://pepijndevos.nl/2016/08/24/x86-instruction-distribution.html

  local bin=_build/oil/ovm-opt.stripped
  objdump -d $bin | cut -f3 | grep -oE "^[a-z]+" | hist
}

hist() {
  sort | uniq -c | sort -n
}

stdlib-imports() {
  oil-osh-files | xargs grep --no-filename '^import' | hist
}

imports() {
  oil-osh-files | xargs grep --no-filename -w import | hist
}

imports-not-at-top() {
  oil-osh-files | xargs grep -n -w import | awk -F : ' $2 > 100'
}

# For the compiler, see what's at the top level.
top-level() {
  grep '^[a-zA-Z]' {core,osh}/*.py \
    | grep -v '_test.py'  \
    | egrep -v ':import|from|class|def'  # note: colon is from grep output
}

_python-symbols() {
  local main=$1
  local name=$2
  local out_dir=$3

  mkdir -p $out_dir
  local out=${out_dir}/${name}-symbols.txt

  # Run this from the repository root.
  PYTHONPATH=. CALLGRAPH=1 $main | tee $out

  wc -l $out
  echo 
  echo "Wrote $out"
}

oil-python-symbols() {
  local out_dir=${1:-_tmp/opy-test}
  _python-symbols bin/oil.py oil $out_dir
}

opy-python-symbols() {
  local out_dir=${1:-_tmp/opy-test}
  _python-symbols bin/opy_.py opy $out_dir
}

old-style-classes() {
  oil-python-symbols | grep -v '<'
}

show-pickle() {
  # python2 doesn't have this?
  python3 -m pickletools "$@"
}

# Looks like 18 bytecodes, but PROTO and STOP are trivial.
pickle-bytecodes() {
  # NOTE: 
  # - This regex is very fragile.  It might be better to really parse the
  # pickle stream.
  # - -a is useful for showing what a bytecode does.
  show-pickle _devbuild/*_asdl.pickle | egrep -o '[[:space:]][A-Z_]{2,}' | hist
  #show-pickle _devbuild/*_asdl.pickle 
}

# Some of these are "abstract classes" like ChildStateChange
NotImplementedError() {
  grep NotImplementedError */*.py
}

"$@"
