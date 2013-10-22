listfile=$1
model=$2
outputdir=$3

if [[ "${outputdir}" == "" ]] ; then
  echo Syntax: $0 \<listfile\> \<model\> \<outputdir\>
else
  declare -i index=0
  for ms in `cat ${listfile}` ; do
    jobname="spectrum-${index}"
    jobscript="${HOME}/pbs_work/${jobname}.script"
    sed s/JOB_NAME/${jobname}/ ${HOME}/scripts/pbs-header.txt|sed s/WALLTIME/01:30:00/ > ${jobscript}
    
    outp="${ms##*/}"
    outp="${outp%/}"
    outputdir="${outputdir%/}"
    outp="${outputdir}/${outp%.ms}-spectrum.txt"

    echo "spectrum -applybeam ${model} ${outp} ${ms} > ${HOME}/pbs_work/${jobname}-out.txt " >> ${jobscript}

    index=${index}+1
    chmod u+x ${jobscript}
    echo "qsub ${jobscript}"
  done
  echo "qstat -a -t -u \${USER}"
fi
