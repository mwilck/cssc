#! /bin/sh
# included.sh:  Tests for included deltas.

# Import common functions & definitions.
. ../common/test-common


g=gfile
s=s.$g
x=x.$g 
z=z.$g
p=p.$g

remove $g $s $x $z $p

# create an empty history file.
echo "seq=1 1.1" > $g
docommand C1 "$admin -i$g $s" 0 "" IGNORE
remove $g


# Put some versions on the trunk.

docommand I2 "$get -s -e $s"  0 IGNORE IGNORE
echo "seq=2 1.2" >> $g
docommand I3 "$delta  -y $s"  0 IGNORE IGNORE

docommand I4 "$get -s -e $s"  0 IGNORE IGNORE
echo "seq=3 1.3" >> $g
docommand I5 "$delta  -y $s"  0 IGNORE IGNORE

docommand I6 "$get -s -e $s"  0 IGNORE IGNORE
echo "seq=4 1.4" >> $g
docommand I7 "$delta  -y $s"  0 IGNORE IGNORE

docommand I8 "$get -s -e $s"  0 IGNORE IGNORE
echo "seq=5 1.5" >> $g
docommand I9 "$delta  -y $s"  0 IGNORE IGNORE

docommand I10 "$get -s -e $s" 0 IGNORE IGNORE
echo "seq=6 1.6" >> $g
docommand I11 "$delta  -y $s" 0 IGNORE IGNORE

docommand I12 "$get -s -e $s" 0 IGNORE IGNORE
echo "seq=7 1.7" >> $g
docommand I13 "$delta  -y $s" 0 IGNORE IGNORE


# Create a branch.
docommand B1 "$get -s -e -r1.2 $s" 0 IGNORE IGNORE
docommand B2 "mv $g $g.old"        0 IGNORE IGNORE
docommand B3 "sed -e  '1 a\\
seq=8 1.2.1.1'  < $g.old > $g"     0 IGNORE IGNORE
remove $g.old
docommand I14 "$delta  -y $s" 0 IGNORE IGNORE

# Add to the head of the revision tree a delta which includes the
# delta we just made.
docommand I14 "$get -s -e -i1.2.1.1 $s" 0 IGNORE IGNORE

docommand I15 "$delta -y            $s" 0 IGNORE IGNORE

# Make sure we get the right result...
docommand I16 "$get -s -p $s" 0 \
"seq=1 1.1\nseq=8 1.2.1.1\nseq=2 1.2\nseq=3 1.3\nseq=4 1.4\nseq=5 1.5\nseq=6 1.6\nseq=7 1.7\n" IGNORE

remove $g $s $x $z $p
success
