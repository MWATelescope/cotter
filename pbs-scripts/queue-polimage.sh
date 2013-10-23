ms=$1

if [[ "${ms}" == "" ]] ; then
  echo Syntax: $0 \<ms-full-path\>
else
  niter=10000
  size=2560
  scale=0.01

  outputdir="${ms%/}"
  outputdir="${outputdir%.ms}"
  filename=`basename ${outputdir}`
  outputdir=`dirname ${outputdir}`
  rootname="${outputdir}/${filename}"

  polpar[0]="-pol xx -name ${rootname}-xx -niter ${niter}"
  polpar[1]="-pol xy -name ${rootname}-xy"
  polpar[2]="-pol yx -name ${rootname}-yx"
  polpar[3]="-pol yy -name ${rootname}-yy -niter ${niter}"
  polpar[4]="-pol xy -imaginarypart -name ${rootname}-xyi"
  polpar[5]="-pol yx -imaginarypart -name ${rootname}-yxi"
  dependencies=""
  for (( p=0 ; p!=6 ; p++ )) ; do
    jobname="pimg${p}"
    jobscript="${HOME}/pbs_work/job-${jobname}-${filename}.script"
    sed s/JOB_NAME/${jobname}/ ${HOME}/scripts/pbs-header.txt|sed s/WALLTIME/00:30:00/ > ${jobscript}
    echo "wsclean ${polpar[p]} -size ${size} ${size} -scale ${scale} >& ${HOME}/pbs_work/log-${jobname}.txt ${ms}" >> ${jobscript}
    chmod u+x ${jobscript}
    echo "qsub ${jobscript}"
    jobnum=`qsub ${jobscript}`
    if [[ ${p} == 0 ]] ; then
	dependencies="depend=afterok:${jobnum}"
    else
	dependencies="${dependencies},afterok:${jobnum}"
    fi
  done

  jobname="pimgpb"
  jobscript="${HOME}/pbs_work/job-${jobname}-${filename}.script"
  sed s/JOB_NAME/${jobname}/ ${HOME}/scripts/pbs-header.txt|sed s/WALLTIME/00:15:00/ > ${jobscript}
  echo "cd ${outputdir}" >> ${jobscript}
  echo "beam -proto ${rootname}-xx-image.fits -ms ${ms}"  >> ${jobscript}
  echo -e -n "pbcorrect " >> ${jobscript}
  echo -e -n "${rootname}-xx-image.fits " >> ${jobscript}
  echo -e -n "${rootname}-xy-image.fits " >> ${jobscript}
  echo -e -n "${rootname}-xyi-image.fits " >> ${jobscript}
  echo -e -n "${rootname}-yx-image.fits " >> ${jobscript}
  echo -e -n "${rootname}-yxi-image.fits " >> ${jobscript}
  echo -e -n "${rootname}-yy-image.fits " >> ${jobscript}
  echo -e -n "${outputdir}/beam-xxr.fits ${outputdir}/beam-xxi.fits ${outputdir}/beam-xyr.fits ${outputdir}/beam-xyi.fits ${outputdir}/beam-yxr.fits ${outputdir}/beam-yxi.fits ${outputdir}/beam-yyr.fits ${outputdir}/beam-yyi.fits "  >> ${jobscript}
  echo -e -n "${rootname}-image-i.fits " >> ${jobscript}
  echo -e -n "${rootname}-image-q.fits " >> ${jobscript}
  echo -e -n "${rootname}-image-u.fits " >> ${jobscript}
  echo -e -n "${rootname}-image-v.fits " >> ${jobscript}
  echo >> ${jobscript}
  echo qsub -W ${dependencies} ${jobscript}
  qsub -W ${dependencies} ${jobscript}
  echo "qstat -a -t -u \${USER}"
  qstat -a -t -u ${USER}
fi
