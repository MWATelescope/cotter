#! /bin/bash
for (( a=81 ; a<153 ; a++ )) ; do
    if (( $a == 81 )) ; then
	echo \# Central band 93 \(81-104\)
    elif (( $a == 105 )) ; then
	echo \# Central band 117 \(105-128\)
    elif (( $a == 129 )) ; then
	echo \# Central band 141 \(129-152\)
    fi
    echo -n "$a "
    echo -n `echo \($a-0.5\)*1.28 | bc -l`
    if (( $a < 147 )) ; then
	echo " 1"
    else
	echo " 4"
    fi
done
