# qhadd
Simple application for merging root files by using batch system. qhadd create bash scripts and send them via qsub command to nodes. Each job merge some fraction of files into one file, last job merge all such files into single file. This application require ROOT framework.
qhadd merge group files in given directory. Final file is stored in merged_dir/merged/all_merged.root or in merged_dir/all_merged.root (if --remove option was used)
## usage
qhadd path [OPTIONS]
"path" is folder with ROOT files. Following options are avaiable:
<br>--min-size= where you can specify minimal size of files merged in single job (e.g. --min-size=1M). This option remove small files that probably are corrupted (hadd cannot properly deal with them)
<br>--max-merged= here you specify how much files you merge in single job
<br>--remove - remove all temporary and all files used to merging
<br>--source= specify environmental variables used in jobs, in some clusters ROOT is not configured on nodes, therefore all variables must be "sent". Here you can specify path used to source those variables (e.g. --source=/home/user/source.sh) or use current environmetal variables (LD_LIBRARY_PATH and PATH) by specify --source=this
<br>--max-hadd= specify maximal number of files merged by single hadd command. Merging too many files by single hadd can result in crash of program.
<br>--e= specify path to log errors (by default is /dev/null)
<br>--o= specify path to log (by default is /dev/null)
## current limitations
Currently only max-hadd is used only in "first iteraction" of merging. For example if you merge 1000 files in single job and specfify max-hadd=10 then in "first interaction" you obtain 100 "temporary files" that will be merged in one step in "second iteraction".
