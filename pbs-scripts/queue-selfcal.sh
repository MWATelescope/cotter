listfile=$1
model=$2
outputdir=$3

if [[ "${outputdir}" == "" ]] ; then
  echo Syntax: $0 \<listfile\> \<model\> \<solution outputdir\>
else

  mkdir -p ${outputdir}
  declare -i index=0
  echo To execute:
  for ms in `cat ${listfile}` ; do
    jobname="sc${index}-`date +%H%M%S`"
    jobscript="${HOME}/pbs_work/${jobname}.script"
    sed s/JOB_NAME/${jobname}/ ${HOME}/scripts/pbs-header.txt|sed s/WALLTIME/04:00:00/ > ${jobscript}
    solfile="${ms##*/}"
    solfile="${solfile%/}"
    outputdir="${outputdir%/}"
    solfile="${outputdir}/${solfile%.ms}-sol.bin"
    
    echo "calibrate -t 2 -i 100 -a 0.00001 0.000001 -applybeam -m ${model} ${ms} ${solfile} > ${HOME}/pbs_work/${jobname}-out.txt" >> ${jobscript}
    echo "applysolutions ${ms} ${solfile}" >> ${jobscript}

    index=${index}+1
    chmod u+x ${jobscript}
    echo "qsub ${jobscript}"
  done
  echo "qstat -a -t -u \${USER}"
fi
