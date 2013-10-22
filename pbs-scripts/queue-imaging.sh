listfile=$1
outputdir=$2

if [[ "${outputdir}" == "" ]] ; then
  echo Syntax: $0 \<listfile\> \<image outputdir\>
else
  mkdir -p ${outputdir}
  jobname="img-`date +%H%M%S`"
  jobscript="${HOME}/pbs_work/job-${jobname}.script"
  sed s/JOB_NAME/${jobname}/ ${HOME}/scripts/pbs-header-arr.txt > ${jobscript}
  declare -i index=0
  for ms in `cat ${listfile}` ; do
    outputprefix="${ms##*/}"
    outputprefix="${outputprefix%/}"
    outputdir="${outputdir%/}"
    outputprefix="${outputdir}/${outputprefix%.ms}"

    
    echo "imgname[${index}]=${outputprefix}" >> ${jobscript}
    echo "ms[${index}]=${ms}" >> ${jobscript}

    index=${index}+1
  done
  declare -i maxindex=${index}-1
  chmod u+x ${jobscript}

  echo "wsclean -size 4096 4096 -scale 0.012 -nwlayers 256 -niter 20000 -gain 0.08 -mgain 0.9 -negative -gridmode kb -threshold 0.3 -weight uniform -name \${imgname[\${JOBID}]} \${ms[\${JOBID}]} >& ${HOME}/pbs_work/log-${jobname}-\${JOBID}.txt" >> ${jobscript}
  echo To execute:
  echo "qsub -J 0-${maxindex} ${jobscript} ; qstat -a -t -u \${USER}"
fi
