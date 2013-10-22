listfile=$1
solfile=$2
jobid=$3

if [[ "${solfile}" == "" ]] ; then
  echo Syntax: $0 \<listfile\> \<solution file\> \<jobid\>
else
  declare -i index=0
  for ms in `cat ${listfile}` ; do
    jobname="apply-${jobid}-${index}"
    jobscript="${HOME}/pbs_work/${jobname}.script"
    sed s/JOB_NAME/${jobname}/ ${HOME}/scripts/pbs-header.txt|sed s/WALLTIME/00:15:00/ > ${jobscript}
    
    echo "applysolutions ${ms} ${solfile}" >> ${jobscript}

    index=${index}+1
    chmod u+x ${jobscript}
    echo "qsub ${jobscript}"
  done
  echo "qstat -a -t -u \${USER}"
fi
