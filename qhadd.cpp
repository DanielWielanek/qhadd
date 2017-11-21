//============================================================================
// Name        : qmerger.cpp
// Author      : Daniel Wielanek
// Version     :
// Copyright   : 
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <TString.h>
#include <Rtypes.h>
#include <TSystemDirectory.h>
#include <TROOT.h>
#include <TSystem.h>
#include <TObjString.h>
#include <TClonesArray.h>
#include <TList.h>
#include <TMath.h>
#include <fstream>
using namespace std;
enum submitter{
	qsub,
	slurm,
	condor
};
/**
 * array with fils
 */
TClonesArray *array_files;
/**
 * out log path
 */
TString out_log;
/**
 * error log path
 */
TString err_log;
/**
 * erro args
 */
TString extra_args;
/**
 * environmetal variables path
 */
TString source;
/**
 * directory with files
 */
TString dir;
/**
 *  use slurm
 */
submitter submit_type;
/**
 * remove temp
 */
Bool_t remove_temp;
/**
 * max number of files to merge in single job
 */
Int_t max_merge;
/**
 * max number of files merge by single hadd
 */
Int_t max_hadd;
/**
 * total number of jobs
 */
Int_t jobs_no;
/**
 * minimal size of merged files
 */
TString min_size;

/**
 * overwrite this to not pass all arguments
 */
void SetDefaultSetup(){
	remove_temp = kFALSE;
	source = "";
	dir = "";
	out_log = "/dev/null";
	err_log = "/dev/null";
	extra_args = "";
	submit_type = qsub;
	max_hadd = 100;
	max_merge = 100;
	min_size = "";
}

void GetListOfFiles(TString path) {
	TSystemDirectory dir(path, path);
	Int_t files_no = 0;
	TList *files = dir.GetListOfFiles();
	if (files) {
		TSystemFile *file;
		TString fname;
		TIter next(files);
		while ((file = (TSystemFile*) next())) {
			fname = file->GetName();
			if (!file->IsDirectory() && fname.EndsWith(".root")) {
				TObjString *strin = (TObjString*) array_files->ConstructedAt(
						files_no++);
				strin->SetString(fname);
			}
		}
	}
	delete files;
}
void CreateScriptFile(TString temp_dir, Int_t start_file, Int_t end_file,
		Int_t i) {
	std::ofstream minimerger;
	minimerger.open(Form("%s/minimerger.sh", temp_dir.Data()));
	if (slurm) {
		minimerger << "#!/bin/bash -l" << endl;
	} else {
		minimerger << "#!/bin/bash" << endl;
	}
	if (source == "this") {
		minimerger << "export PATH=$PATH:" << gSystem->Getenv("PATH") << endl;
		minimerger << "export LD_LIBRARY_PATH="<< gSystem->Getenv("LD_LIBRARY_PATH") << ":$LD_LIBRARY_PATH"<< endl;
	} else if (source.Length() > 0) {
		if (source.Contains("/")) {/* probably full path */
			minimerger << "source " << source << endl;
		} else {/*probably pointing to current file*/
			minimerger << "source " << gSystem->Getenv("PWD") << "/" << source<< endl;
		}
	}
	minimerger << "cd " << temp_dir << endl;
	/** merge all in single step **/
	if (max_hadd == max_merge) {
		minimerger << "hadd total.root *.root" << endl;
	} else {
		/** merge in substeps **/
		Int_t substeps = TMath::Ceil(((Double_t) end_file - start_file) / ((Double_t) max_hadd));
		for (int j = 0; j < substeps; j++) {
			TObjString *str = (TObjString*) array_files->UncheckedAt(start_file + max_hadd * j);
			if (str != NULL) {
				minimerger << Form("hadd subtotal_%i.root ", j);
				for (int k = 0; k < max_hadd; k++) {
					Int_t index = start_file + max_hadd * j + k;
					TObjString *str = (TObjString*) array_files->UncheckedAt(index);
					if (index >= end_file)
						break;
					if (str == 0x0) {
						break;
					} else {
						TString name = str->GetString();
						minimerger << name << " ";
					}
				}
				minimerger << endl;
			} else {
				break;
			}
		}
		minimerger << "hadd total.root subtotal_*" << endl;
	}
	minimerger << "cp total.root " << "../merged/total_" << i << ".root"<< endl;
	minimerger << "touch done.txt" << endl;
	minimerger << "cd .." << endl;
	minimerger << "can_merge=1" << endl;
	minimerger << Form("for F in  %s/dir_*;", dir.Data()) << endl;
	minimerger << "do" << endl;
	minimerger << "\tif [ ! -f $F/done.txt ]; then" << endl;
	minimerger << "\tcan_merge=0" << endl;
	minimerger << "\tfi" << endl;
	minimerger << "done" << endl;
	minimerger << "if (( $can_merge == 1 )); then" << endl;
	if (max_hadd == max_merge) {
		minimerger<< Form("\thadd %s/merged/all_merged.root %s/merged/*.root",dir.Data(), dir.Data()) << endl;
	} else {
		Int_t substeps = TMath::Ceil(
				((Double_t) jobs_no) / ((Double_t) max_hadd));
		Int_t subjob_files = 0;
		for (int k = 0; k < substeps; k++) {
			minimerger<< Form("\t hadd %s/merged/all_merged_%i.root ", dir.Data(),k);
			for (int l = 0; l < max_hadd; l++) {
				if (subjob_files < jobs_no) {
					minimerger << " "<< Form("%s/merged/total_%i.root", dir.Data(),subjob_files++);
				} else {
					break;
				}
			}
			minimerger << endl;
		}
		minimerger<< Form("\thadd %s/merged/all_merged.root %s/merged/all_merged_*",dir.Data(), dir.Data()) << endl;
	}

	if (remove_temp) {
		minimerger << "\trm -rf " << dir << "/dir_*" << endl;
		minimerger << "\tmv merged/all_merged.root ." << endl;
		minimerger << "\trm -rf merged" << endl;
	}
	minimerger << "else" << endl;
	minimerger << "\techo \"can\'t merge files\"" << endl;
	minimerger << "fi" << endl;
	minimerger.close();
	gROOT->ProcessLine(Form(".! chmod a+x %s/minimerger.sh", temp_dir.Data()));

}
void SubmitJob(TString temp_dir,Int_t i) {
	TString outputLog_opt = Form("%s/qhadd_%i.o", out_log.Data(), i);
	TString errorLog_opt = Form("%s/qhadd_%i.e ", err_log.Data(), i);
	if (out_log == "/dev/null")
		outputLog_opt = "/dev/null";
	if (err_log == "/dev/null")
		errorLog_opt = "/dev/null";
	TString command = "";
	switch(submit_type){
	case qsub:
		command = "qsub";
		break;
	case slurm:
		command = "sbatch";
		break;
	default:
		command = "condor_submit";
		break;
	}
	TString script_path = Form("%s/minimerger.sh", temp_dir.Data());
	if(submit_type==qsub||submit_type==slurm){
		TString args = Form("-o %s -e %s", outputLog_opt.Data(), err_log.Data());
		if (extra_args.Length() > 1) {
			args = args + " " + extra_args;
		}
		gROOT->ProcessLine(Form(".! %s %s %s", command.Data(), args.Data(),script_path.Data()));
	}else if(submit_type==condor){
		std::ofstream jobfile;
		TString jobfile_name = Form("%s_job.condor",script_path.Data());
		jobfile.open(jobfile_name);
		jobfile<<"######## condor submit ##########"<<endl;
		jobfile<<"executable = "<<script_path<<endl;
		jobfile<<"log = "<< outputLog_opt.Data()<<endl;
		jobfile<<"error = "<<err_log.Data()<<endl;
		jobfile<<"queue"<<endl;
		jobfile.close();
		gROOT->ProcessLine(Form(".! condor_submit %s",jobfile_name.Data()));
	}
	cout << "Job " << i << " of " << jobs_no << " submitted" << endl;
}

int main(int argc, char *argv[]) {
	SetDefaultSetup();
	if (argc <= 1) {
		cout << "not enoug arguments" << endl;
		cout << "calll qhadd DIR [OPTIONS]" << endl;
		cout << "Avaiable options ;" << endl;
		cout<< "\t--min-size=X where X (e.g. 10k) is minimal size of file, filles smaler than this are removed"<< endl;
		cout<< "\t--max-merged=N where N is maximal number of files merged in single job"<< endl;
		cout << "\t--remove delete all temporary and all files used for merging"<< endl;
		cout<< "\t--source=P where P is path to source file with environmental variables, you can set --source=this to use current env"<< endl;
		cout<< "\t i such case PATH and LD_LIBRARY_PATH will be copied from this node"<< endl;
		cout<< "\t--max-hadd=N where N is maximual number of files merged by calling single hadd"<< endl;
		cout << "\t--e=X path to error logs (by default error log is supresed)"<< endl;
		cout << "\t--o=X path to output logs (by default  log is supresed)"<< endl;
		cout << "\t--slurm use slurm system instead of SGE" << endl;
		cout<< "\t--condor - use condor submitter instead of SG " <<endl;
		cout<< "\t--args=\"X\" where X are additional arguments during submitting job (not supported for condor)"<< endl;
		return 0;
	}
	for (int i = 1; i < argc; i++) {
		TString temp = argv[i];
		if (temp.BeginsWith("--min-size=")) {
			TString size(temp(11, temp.Length()));
			min_size = size;
		} else if (temp.BeginsWith("--max-merged=")) {
			TString sample(temp(13, temp.Length()));
			max_merge = sample.Atoi();
		} else if (temp.EqualTo("--remove")) {
			remove_temp = kTRUE;
		} else if (temp.BeginsWith("--source=")) {
			TString s(temp(9, temp.Length()));
			source = s;
		} else if (temp.BeginsWith("--max-hadd=")) {
			TString s(temp(11, temp.Length()));
			max_hadd = s.Atoi();
		} else if (temp.BeginsWith("--o=")) {
			TString s(temp(4, temp.Length()));
			out_log = s;
		} else if (temp.BeginsWith("--e=")) {
			TString s(temp(4, temp.Length()));
			err_log = s;
		} else if (temp.BeginsWith("--slurm")) {
			submit_type = slurm;
		}else if(temp.BeginsWith("--condor")){
			submit_type = condor;
		} else if (temp.BeginsWith("--args=")) {
			TString s(temp(7, temp.Length()));
			extra_args = s;
		} else {
			dir = temp;
		}
	}
	if (max_hadd == 0)
		max_hadd = max_merge;
	if (!dir.BeginsWith("/")) {/* not full path */
		dir = Form("%s/%s", gSystem->Getenv("PWD"), dir.Data());
	}
	if (min_size.Length() > 1) {
		std::cout << "deleting files smaller than " << min_size << endl;
		gROOT->ProcessLine(Form(".! find %s/ -name \"*.root\" -size -%s -delete ",dir.Data(), min_size.Data()));
	}
	array_files = new TClonesArray("TObjString", 10);
	GetListOfFiles(dir);
	Int_t files_no = array_files->GetEntries();
	jobs_no = TMath::Ceil(((Double_t) files_no) / ((Double_t) max_merge));
	std::cout<<"======= qhadd ==============="<<std::endl;
	std::cout << "Found " << files_no << " root files "<<endl;
	cout << " will be created " << jobs_no << " jobs" << endl;

	gSystem->mkdir(Form("%s/merged", dir.Data()));
	Int_t file_count = 0;
	for (int i = 0; i < jobs_no; i++) {
		TString temp_dir = Form("%s/dir_%04d", dir.Data(), i);
		gSystem->mkdir(temp_dir);
		/* move files to given directory */
		Int_t start_file = file_count;
		Int_t end_file = start_file;
		for (int j = 0; j < max_merge; j++) {
			TObjString *name = (TObjString*) array_files->UncheckedAt(file_count++);
			if (name == NULL)
				break;
			end_file++;
			gROOT->ProcessLine(Form(".! mv %s/%s %s/", dir.Data(),name->GetString().Data(), temp_dir.Data()));
		}
		/*create merging script*/
		CreateScriptFile(temp_dir, start_file, end_file, i);
		SubmitJob(temp_dir,i);
	}
	delete array_files;
	return 0;
}
