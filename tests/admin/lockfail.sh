#! /bin/sh

# lockfail.sh:  Testing for correct behaviour when we fail to 
#               create the lock file.

# Import common functions & definitions.
. ../common/test-common

g=bar
s=s.${g}
z=z.${g}

# If we try to create an SCCS file in a nonexistent directory, 
# we should get a failure, not a core dump.

docommand F1 "${admin} -n foo/s.bar" 1 "" IGNORE


success

