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

void GetListOfFiles(TString path, TClonesArray *res){
	TSystemDirectory dir(path,path);
	std::cout<<" DIR " <<path<<endl;
	Int_t files_no=0;
	TList *files = dir.GetListOfFiles();
	if (files) {
	      TSystemFile *file;
	      TString fname;
	      TIter next(files);
	      while ((file=(TSystemFile*)next())) {
	         fname = file->GetName();
	         if (!file->IsDirectory() && fname.EndsWith(".root")) {
	            TObjString *strin = (TObjString*)res->ConstructedAt(files_no++);
	            strin->SetString(fname);
	         }
	      }
	   }
	delete files;
}
int main(int argc, char *argv[]) {
	Int_t max_merge = 100;
	TString min_size = "";
	Bool_t remove_temp = kFALSE;
	TString source="";
	TString dir="";
	if(argc<=1){
		cout<<"not enoug arguments"<<endl;
		cout<<"calll qhadd DIR [OPTIONS]"<<endl;
		cout<<"Avaiable options ;"<<endl;
		cout<<"\t-min-size=X where X (e.g. 10k) is minimal size of file, filles smaler than this are removed"<<endl;
		cout<<"\t-max-sample=N where N is maximal number of files merged in single step"<<endl;
		cout<<"\t-remove delete all temporary and all merged files"<<endl;
		cout<<"\t-source=P where P is path to source file with environmental variables, you can set --source=this to use current env"<<endl;
		cout<<"\t i such case PATH and LD_LIBRARY_PATH will be copied from this node"<<endl;
		return 0;
	}
	for(int i=1;i<argc;i++){
		TString temp = argv[i];
		if(temp.BeginsWith("-min-size=")){
			TString size(temp(10,temp.Length()));
			min_size = size;
		}else if(temp.BeginsWith("-max-sample=")){
			TString sample(temp(12,temp.Length()));
			max_merge = sample.Atoi();
		}else if(temp.EqualTo("-remove")){
			remove_temp = kTRUE;
		}else if(temp.BeginsWith("-source=")){
			TString s(temp(8,temp.Length()));
			source = s;
		}else{
			dir  = temp;
		}
	}
	if(!dir.BeginsWith("/")){// not full path
		dir = Form("%s/%s",gSystem->Getenv("PWD"),dir.Data());
	}
	if(min_size.Length()>1){
		std::cout<<"deleting files smaller than "<<min_size<<endl;
		gROOT->ProcessLine(Form(".! find %s/ -name \"*.root\" -size -%s -delete ",dir.Data(),min_size.Data()));
	}
	TClonesArray *array = new TClonesArray("TObjString",10);
	GetListOfFiles(dir,array);
	Int_t files_no = array->GetEntries();
	Int_t jobs_no = TMath::Ceil(((Double_t)files_no)/((Double_t)max_merge));
	std::cout<<"Found "<<files_no<<endl;
	cout<<" will be created "<<jobs_no<<"jobs"<<endl;

	gSystem->mkdir(Form("%s/merged",dir.Data()));
	Int_t file_count = 0;
	for(int i=0;i<jobs_no;i++){
		TString temp_dir = Form("%s/dir_%03d",dir.Data(),i);
		gSystem->mkdir(temp_dir);
		/// move files to given directory
		for(int j=0;j<max_merge;j++){
			TObjString *name = (TObjString*)array->UncheckedAt(file_count++);
			if(name==0x0) continue;
			gROOT->ProcessLine(Form(".! mv %s/%s %s/",dir.Data(),name->GetString().Data(),temp_dir.Data()));
		}
		//create merging script
		std::ofstream minimerger;
		minimerger.open(Form("%s/minimerger.sh",temp_dir.Data()));
		minimerger<<"#!/bin/bash"<<endl;
		if(source=="this"){
			minimerger<<"export PATH=$PATH:"<<gSystem->Getenv("PATH")<<endl;
			minimerger<<"export LD_LIBRARY_PATH="<<gSystem->Getenv("LD_LIBRARY_PATH")<<":$LD_LIBRARY_PATH"<<endl;
		}else if(source.Length()>0){
			if(source.Contains("/")){// probably full path
				minimerger<<"source "<<source<<endl;
			}else{//probably pointing to current file
				minimerger<<"source "<<gSystem->Getenv("PWD")<<"/"<<source<<endl;
			}
		}
		minimerger<<"cd "<<temp_dir<<endl;
		minimerger<<"hadd total.root *.root"<<endl;
		minimerger<<"cp total.root "<<"../merged/"<<i<<".root"<<endl;
		minimerger<<"touch done.txt"<<endl;
		minimerger<<"cd .."<<endl;
		minimerger<<"can_merge=1"<<endl;
		minimerger<<Form("for F in  %s/dir_*;",dir.Data())<<endl;
		minimerger<<"\tif [ ! -f \\$F/done.txt ]; then"<<endl;
		minimerger<<"\tcan_merge=0"<<endl;
		minimerger<<"\tfi"<<endl;
		minimerger<<"done"<<endl;
		minimerger<<"if (( $can_merge == 1 )); then"<<endl;
		minimerger<<Form("\thadd %s/merged/all_merged.root %s/merged/*.root",dir.Data(),dir.Data())<<endl;
		if(remove_temp){
			minimerger<<"\trm -rf "<<dir<<"/dir_*"<<endl;
		}
		minimerger<<"else"<<endl;
		minimerger<<"\techo \"can\'t merge files\""<<endl;
		minimerger<<"fi"<<endl;
		minimerger.close();
		gROOT->ProcessLine(Form(".! chmod a+x %s/minimerger.sh",temp_dir.Data()));
		gROOT->ProcessLine(Form(".! qsub %s/minimerger.sh",temp_dir.Data()));
		cout<<"Job "<<i<<" of "<<jobs_no<<" submitted"<<endl;
	}
	return 0;
}
