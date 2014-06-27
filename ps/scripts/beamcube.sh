#! /bin/bash
if [[ "$2" == "" ]] ; then
		echo Syntax: beamcube.sh \<image-prefix\> \<ms\>
else
		imgprefix="$1"
		ms="$2"
		declare -i index=0
		for fits in `ls ${imgprefix}-*-XX-image.fits` ; do
				chanprefix="${fits%-XX-image.fits}"
				beamname="${chanprefix}-beam"
				if [[ "${chanprefix}" != "${imgprefix}-MFS" ]] ; then
						echo Making ${beamname}...
				    beam -2014i -ms ${ms} -proto ${fits} -name ${beamname} &
						index=${index}+1
						if [[ ${index} == 12 ]] ; then
								index=0
								wait
						fi
				fi
		done
		wait
fi
