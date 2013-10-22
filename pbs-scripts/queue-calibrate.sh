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
    jobname="cal${index}-`date +%H%M%S`"
    jobscript="${HOME}/pbs_work/${jobname}.script"
    sed s/JOB_NAME/${jobname}/ ${HOME}/scripts/pbs-header.txt|sed s/WALLTIME/00:30:00/ > ${jobscript}
    solfile="${ms##*/}"
    solfile="${solfile%/}"
    outputdir="${outputdir%/}"
    solfile="${outputdir}/${solfile%.ms}-sol.bin"
    
    echo "calibrate -i 100 -a 0.00001 0.000001 -applybeam -diag -m ${model} ${ms} ${solfile} > ${HOME}/pbs_work/${jobname}-out.txt" >> ${jobscript}

    index=${index}+1
    chmod u+x ${jobscript}
    echo "qsub ${jobscript}"
  done
  echo "qstat -a -t -u \${USER}"
fi
