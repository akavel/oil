# builtins specific to bash and OSH

#### help
help
echo status=$? >&2
help help
echo status=$? >&2
help -- help
echo status=$? >&2
## STDERR:
status=0
status=0
status=0
## END

#### bad help topic
help ZZZ 2>$TMP/err.txt
echo "help=$?"
cat $TMP/err.txt | grep -i 'no help topics' >/dev/null
echo "grep=$?"
## STDOUT: 
help=1
grep=0
## END

#### type -t -> function
f() { echo hi; }
type -t f
## stdout: function

#### type -t -> alias
shopt -s expand_aliases
alias foo=bar
type -t foo
## stdout: alias

#### type -t -> builtin
type -t echo read : [ declare local
## STDOUT:
builtin
builtin
builtin
builtin
builtin
builtin
## END

#### type -t -> keyword
type -t for time ! fi do {
## STDOUT: 
keyword
keyword
keyword
keyword
keyword
keyword
## END

#### type -t control flow

# this differs from bash, but don't lie!
type -t break continue return exit
## STDOUT:
keyword
keyword
keyword
keyword
## END
## OK bash STDOUT:
builtin
builtin
builtin
builtin
## END


#### type -t -> file
type -t find xargs
## STDOUT: 
file
file
## END

#### type -t doesn't find non-executable (like command -v)
PATH="$TMP:$PATH"
touch $TMP/non-executable
type -t non-executable
## stdout-json: ""
## status: 1
## BUG bash STDOUT:
file
## END
## BUG bash status: 0

#### type -t -> not found
type -t echo ZZZ find =
echo status=$?
## STDOUT: 
builtin
file
status=1
## END

#### type -p and -P builtin -> file
touch /tmp/{mv,tar,grep}
chmod +x /tmp/{mv,tar,grep}
PATH=/tmp:$PATH

type -p mv tar grep
echo --
type -P mv tar grep
## STDOUT:
/tmp/mv
/tmp/tar
/tmp/grep
--
/tmp/mv
/tmp/tar
/tmp/grep
## END

#### type -p builtin -> not found
type -p FOO BAR NOT_FOUND
## status: 1
## stdout-json: ""

#### type -p builtin -> not a file
type -p cd type builtin command
## stdout-json: ""

#### type -P builtin -> not found
type -P FOO BAR NOT_FOUND
## status: 1
## stdout-json: ""

#### type -P builtin -> not a file
type -P cd type builtin command
## stdout-json: ""
## status: 1

#### type -P builtin -> not a file but file found
touch /tmp/{mv,tar,grep}
chmod +x /tmp/{mv,tar,grep}
PATH=/tmp:$PATH

mv () { ls; }
tar () { ls; }
grep () { ls; }
type -P mv tar grep cd builtin command type
## status: 1
## STDOUT:
/tmp/mv
/tmp/tar
/tmp/grep
## END

#### type -f builtin -> not found
type -f FOO BAR NOT FOUND
## status: 1

#### type -f builtin -> function and file exists
touch /tmp/{mv,tar,grep}
chmod +x /tmp/{mv,tar,grep}
PATH=/tmp:$PATH

mv () { ls; }
tar () { ls; }
grep () { ls; }
type -f mv tar grep
## STDOUT:
/tmp/mv is a file
/tmp/tar is a file
/tmp/grep is a file
## OK bash STDOUT:
mv is /tmp/mv
tar is /tmp/tar
grep is /tmp/grep
