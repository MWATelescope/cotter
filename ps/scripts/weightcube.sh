#! /bin/bash
if [[ "$1" == "" ]] ; then
		echo Syntax: weightcube.sh \<image-prefix\>
else
		imgprefix="$1"
		declare -i index=0
		for fits in `ls ${imgprefix}-*-XX-image.fits` ; do
				chanprefix="${fits%-XX-image.fits}"
				beamname="${chanprefix}-beam"
				if [[ "${chanprefix}" != "${imgprefix}-MFS" ]] ; then
						echo ${chanprefix}...
						~/projects/ps/build/psweight ${chanprefix} image.fits ${beamname} ${chanprefix}
				fi
		done
		wait
fi
