# qhadd
Simple application for merging root file by using batch system. qhadd create bash scripts and send them through qsub comment to nodes. Each job merge some fraction of files into one file, last job merge all such files into single file. This application require ROOT framework.
qhadd merge group files in given directory. Final file is stored in merged_dir/merged/all_merged.root or in merged_dir/all_merged.root (if --remove option was used)
in some directiories (merged_dir/dir_000 merged_dir/dir_001 etc.), then merge all files inside them, copy into merged_dir/merge as total_0.root total_1.root etc. and merge into one file called final file merged_dir/all_merged.root.
## usage
qhadd path [OPTIONS]
"path" is folder with ROOT files. Following options are avaiable:
--min-size= where you can specify minimal size of files merged in single job (e.g. --min-size=1M). This option remove small files that probably are corrupted (hadd cannot properly deal with them)
--max-merged= here you specify how much files you merge in single job
--remove - if this option all merged and all temporary files are removed only all_merged.root file is left
--source= specify environmental variables used in jobs, in some clusters ROOT is not configured on nodes, therefore all variables must be "send". Here you can specify path used to source those variables (e.g. --source=/home/user/source.sh) or use currently used environmetal variables (LD_LIBRARY_PATH and PATH) by specify --source=this
--max-hadd= specify maximal number of files merged by single hadd command. In some cases e.g. when you have large files is pointless to send thousand jobs that merge few files, its better to send 100 jobs that call hadd e.g. 10 times.
--e= specify path to log errors (by default is /dev/null)
--o= specify path to log (by default is /dev/null)
