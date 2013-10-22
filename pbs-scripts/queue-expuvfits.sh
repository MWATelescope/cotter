listfile=$1
outdir=$2

if [[ "${outdir}" == "" ]] ; then
  echo Syntax: $0 \<listfile\> \<outputdir\>
else
  declare -i index=0
  for ms in `cat ${listfile}` ; do
    jobname="exp-${index}"
    jobscript="${HOME}/pbs_work/${jobname}.script"
    sed s/JOB_NAME/${jobname}/ ${HOME}/scripts/pbs-header.txt|sed s/WALLTIME/00:15:00/ > ${jobscript}
    
    fitsfile="${ms##*/}"
    fitsfile="${fitsfile%/}"
    outdir="${outdir%/}"
    fitsfile="${outdir}/${fitsfile%.ms}.uvfits"

    echo "casapy --nologger --nologfile --nogui -c \"exportuvfits(vis='${ms}',fitsfile='${fitsfile}',datacolumn='data')\"" >> ${jobscript}

    index=${index}+1
    chmod u+x ${jobscript}
    echo "qsub ${jobscript}"
  done
  echo "qstat -a -t -u \${USER}"
fi
