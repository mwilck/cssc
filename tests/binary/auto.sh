#! /bin/sh
# auto.sh:  Tests for "admin"'s detection of binary files.

# Import common functions & definitions.
. ../common/test-common




good() {
    if $expect_fail ; then
	echo UNEXPECTED PASS
    else
	echo passed
    fi
}

bad() {
    if $expect_fail ; then
	echo failed -- but we expected that.
    else
	fail "$@"
    fi
}


# test_bin: 
# Usage:   test_bin LABEL <contents>
#
# create a flie containing the specified argument and check
# that it is encoded as a binary file.  
test_bin() {
label=$1
echo_nonl ${label}...
shift

rm -f infile $s
echo_nonl "$@" > infile
if ${admin} -iinfile ${adminflags} $s >/dev/null 2>&1
then
    if ( ${prt} -f $s 2>/dev/null; echo foo ) | grep encoded >/dev/null 2>&1
    then
	good
    else
	bad $label input did not produce an encoded s-file.
    fi
else
    bad $label ${admin} returned exit status $?.
fi
rm -f infile $s
}

# test_ascii: 
#
# As for test_bin, but the resulting SCCS file must NOT be encoded.
# 
test_ascii() {
label=$1
echo_nonl ${label}...
shift

rm -f infile $s
echo_nonl "$@" > infile
if ${admin} -iinfile ${adminflags} $s >/dev/null 2>&1
then
    if ( ${prt} -f $s 2>/dev/null ; echo foo ) | grep encoded >/dev/null 2>&1
    then
	bad $label input produced an encoded s-file and should not have.
    else
	good
    fi
else
    bad $label ${admin} returned exit status $?.
fi
rm -f infile $s
}

g=testfile
s=s.$g
z=z.$g
x=z.$g
p=p.$g
files="$s $z $x $p"

remove command.log log  base [sxzp].$g

expect_fail=false
adminflags=""

test_ascii a1 "foo\n" 
test_ascii a2 "foo\nbar\n" 
test_ascii a3 ""			# an empty input file should be OK.
test_bin   b4 "\001\n"			# line starts with control-a
test_ascii a5 "x\001\n"			# line contains ^A but doesnt start with it.

adminflags=-b
test_bin   b6 "foo\n"			# Test manual encoding override.
test_bin   b7 "x\000y\n"		# ASCII NUL.
test_bin   b8 "\000"			# Just the ASCII NUL alone.
test_bin   b9 "\001"			# Just the ^A alone.
adminflags=

expect_fail=true
## Real SCCS fails on these inputs:-
test_bin   fb10 "foo"  			# no newline at end of file.
test_ascii fa11 "x\000y\n"		# ASCII NUL.

remove command.log $s $p $g $z $x infile
expect_fail=false
success 

