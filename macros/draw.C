//this script is used to compare kalman filter result with global
//helix fit result
#include "stdlib.h"
#include <iostream>
#include <fstream>
#include <iomanip>

#include "math.h"
using namespace std;

#include <TH1.h>
#include <TH2.h>
#include <TCanvas.h>
#include <TQObject.h>
#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TVector3.h"
#include "TString.h"
#include "TCut.h"
#include "TCutG.h"
#include "TPaveText.h"
#include "TText.h"
#include "TPad.h"
#include "TF1.h"
#include "TStyle.h"
#include "TGraph.h"
#include "TMultiGraph.h"
#include "TLorentzVector.h"
#include "TCanvas.h"
#include "TGraphErrors.h"


TF1* FitGauss(TH1* h1, double &mean, double &sigma, double range_in_sigma=1.5)
{
	//cout<<"h1->GetEntries()="<<h1->GetEntries()<<endl;
	if(h1->GetEntries()<50) return NULL;

	double xmin=h1->GetMean()-5.0*h1->GetRMS();
	double xmax=h1->GetMean()+5.0*h1->GetRMS();
	//h1->Fit("gaus","RQ","",xmin,xmax);
	h1->Fit("gaus","Q");
	TF1 *f=(TF1*)h1->GetListOfFunctions()->FindObject("gaus");
	mean=f->GetParameter(1);
	sigma=f->GetParameter(2);
	xmin=mean-range_in_sigma*sigma;
	xmax=mean+range_in_sigma*sigma;
	double chi2_1 = f->GetChisquare()/f->GetNDF();

	h1->Fit("gaus","RQ","",xmin,xmax);
	f=(TF1*)h1->GetListOfFunctions()->FindObject("gaus");	
	double chi2_2 = f->GetChisquare()/f->GetNDF();
	//cout<<"chi2_1="<<chi2_1<<"\t chi2_2="<<chi2_2<<endl;
	if(chi2_2>1.1*chi2_1) {
      //2nd iteration is worse than 1st iteratin, roll back to previous fit
	  h1->Fit("gaus","Q");
	  f=(TF1*)h1->GetListOfFunctions()->FindObject("gaus");	
	}
		
	mean=f->GetParameter(1);
	sigma=f->GetParameter(2);
	  

	if(gStyle->GetOptFit()==0)
	{
		char str[100];
		TText *text=0;

		double xx=gStyle->GetPadLeftMargin()+0.03; 
		TPaveText *pt = new TPaveText(xx,0.20,xx+0.45,0.45,"brNDC");
		pt->SetBorderSize(0);
		pt->SetFillColor(0);
		sprintf(str,"Mean = %.3G",mean);
		text=pt->AddText(str);
		text->SetTextColor(2);
		sprintf(str,"Sigma = %.3G",sigma);
		text=pt->AddText(str);
		text->SetTextColor(2);
		pt->Draw("same");
	}

	return f;
}

TF1* FitGauss(TH1* h1,double range_in_sigma=1.5)
{
	double mean,sigma;
	return FitGauss(h1,mean,sigma,range_in_sigma);
}


void draw(int log_hel=0,int log_rec=1, const char *key="")
{
  double mean_dZ,sigma_dZ;
  double mean_dT,sigma_dT;
  double mean_dF,sigma_dF;
  double mean_dP,sigma_dP;

  system("mkdir -p Graph");
  ofstream fout;
  fout.open("table.txt",ios_base::app);

  TTree *t=(TTree*) gROOT->FindObject("t"); 
  //TTree *t = (TTree*)gDirectory->Get("t");
  //////////////////////////////////////////////////////////////
  TCanvas *c13 = new TCanvas("c13","",900,700);
  c13->Divide(2,2);
  c13->cd(1);
  t->Draw("z0-z_hel>>hd0hz(80,-1,1)","","");
  TH1F *hd0hz = (TH1F*) gROOT->FindObject("hd0hz");
  FitGauss(hd0hz,mean_dZ,sigma_dZ);

  c13->cd(2);
  t->Draw("th0-th_hel>>hd0ht(40,-0.2,0.2)","","");
  TH1F *hd0ht = (TH1F*) gROOT->FindObject("hd0ht");
  FitGauss(hd0ht,mean_dT,sigma_dT);

  c13->cd(3);
  t->Draw("ph0-ph_hel>>hd0hph(100,-0.5,0.5)","","");
  TH1F *hd0hph = (TH1F*) gROOT->FindObject("hd0hph");
  FitGauss(hd0hph,mean_dF,sigma_dF);

  c13->cd(4);
  t->Draw("pt0-pt_hel>>hd0hp(100,-0.05,0.05)","","");
  TH1F *hd0hp = (TH1F*) gROOT->FindObject("hd0hp");
  FitGauss(hd0hp,mean_dP,sigma_dP);

  if(log_hel) {
    fout<<"\n"
	<<setw(18)<<"dZ(cm)"<<"  "
	<<setw(18)<<"dTheta(rad)"<<"  "
	<<setw(18)<<"dPhi(rad)"<<"  "
	<<setw(19)<<"dPt(GeV/c)"<<"  "
	<<"FileName"<<"  "<<endl;
    fout<<fixed<<setprecision(3)
	<<setw(8)<<mean_dZ<<" +/- " <<setw(5)<<sigma_dZ<<"  "
	<<setw(8)<<mean_dT<<" +/- " <<setw(5)<<sigma_dT<<"  "
	<<setw(8)<<mean_dF<<" +/- " <<setw(5)<<sigma_dF<<"  "
	<<setprecision(4)
	<<setw(8)<<mean_dP<<" +/- " <<setw(6)<<sigma_dP<<"  "
	<<"global_helix_fit"<<endl;
  }
  c13->SaveAs(Form("Graph/thrown_vs_hel_%s.png",key));

  //////////////////////////////////////////////////////////////
  TCanvas *c12 = new TCanvas("c12","",900,700);
  c12->Divide(2,2);
  c12->cd(1);
  t->Draw("z0-z_rec>>hd0z(80,-1,1)","","");
  TH1F *hd0z = (TH1F*) gROOT->FindObject("hd0z");
  FitGauss(hd0z,mean_dZ,sigma_dZ);

  c12->cd(2);
  t->Draw("th0-th_rec>>hd0t(40,-0.2,0.2)","","");
  TH1F *hd0t = (TH1F*) gROOT->FindObject("hd0t");
  FitGauss(hd0t,mean_dT,sigma_dT);

  c12->cd(3);
  t->Draw("ph0-ph_rec>>hd0ph(100,-0.5,0.5)","","");
  TH1F *hd0ph = (TH1F*) gROOT->FindObject("hd0ph");
  FitGauss(hd0ph,mean_dF,sigma_dF);

  c12->cd(4);
  t->Draw("pt0-pt_rec>>hd0p(100,-0.05,0.05)","","");
  TH1F *hd0p = (TH1F*) gROOT->FindObject("hd0p");
  FitGauss(hd0p,mean_dP,sigma_dP);
 
  c12->SaveAs(Form("Graph/thrown_vs_rec_%s.png",key));

  if(log_rec) {
	TString filename = gFile->GetName();
	
	if(filename.Contains(".root")) filename.Remove(filename.Index(".root"),5);
	if(filename.Contains("nt_")) filename.Remove(filename.Index("nt_"),3);
	
    fout<<fixed<<setprecision(3)
	<<setw(8)<<mean_dZ<<" +/- " <<setw(5)<<sigma_dZ<<"  "
	<<setw(8)<<mean_dT<<" +/- " <<setw(5)<<sigma_dT<<"  "
	<<setw(8)<<mean_dF<<" +/- " <<setw(5)<<sigma_dF<<"  "
	<<setprecision(4)
	<<setw(8)<<mean_dP<<" +/- " <<setw(6)<<sigma_dP<<"  "
	<<filename.Data()<<endl;
  }
  fout.close();
  
  //////////////////////////////////////////////////////////////
  TCanvas *c11 = new TCanvas("c11","",900,700);
  c11->Divide(2,2);
  c11->cd(1);
  t->Draw("z_hel-z_rec>>hdz(80,-1,1)","","");
  TH1F *hdz = (TH1F*) gROOT->FindObject("hdz");
  FitGauss(hdz);

  c11->cd(2);
  t->Draw("th_hel-th_rec>>hdt(40,-0.2,0.2)","","");
  TH1F *hdt = (TH1F*) gROOT->FindObject("hdt");
  FitGauss(hdt);

  c11->cd(3);
  t->Draw("ph_hel-ph_rec>>hdph(100,-0.5,0.5)","","");
  TH1F *hdph = (TH1F*) gROOT->FindObject("hdph");
  FitGauss(hdph);

  c11->cd(4);
  t->Draw("pt_hel-pt_rec>>hdp(100,-0.05,0.05)","","");
  TH1F *hdp = (TH1F*) gROOT->FindObject("hdp");
  FitGauss(hdp);

  c11->SaveAs(Form("Graph/hel_vs_rec_%s.png",key));

}



void DrawFromMultiFiles(const char* tgStr,const char *cutStr, 
const char *file0, int color1, const char *key0 )
{
  double mean_dZ,sigma_dZ;
  double mean_dT,sigma_dT;
  double mean_dF,sigma_dF;
  double mean_dP,sigma_dP;

  system("mkdir -p Graph");
  ofstream fout;
  fout.open("table.txt",ios_base::app);

  TTree *t=(TTree*) gROOT->FindObject("t"); 
  //TTree *t = (TTree*)gDirectory->Get("t");
  //////////////////////////////////////////////////////////////
  TCanvas *c13 = new TCanvas("c13","",900,700);
  c13->Divide(2,2);
  c13->cd(1);
  t->Draw("z0-z_hel>>hd0hz(80,-1,1)","","");
  TH1F *hd0hz = (TH1F*) gROOT->FindObject("hd0hz");
  FitGauss(hd0hz,mean_dZ,sigma_dZ);

  c13->cd(2);
  t->Draw("th0-th_hel>>hd0ht(40,-0.2,0.2)","","");
  TH1F *hd0ht = (TH1F*) gROOT->FindObject("hd0ht");
  FitGauss(hd0ht,mean_dT,sigma_dT);

  c13->cd(3);
  t->Draw("ph0-ph_hel>>hd0hph(100,-0.5,0.5)","","");
  TH1F *hd0hph = (TH1F*) gROOT->FindObject("hd0hph");
  FitGauss(hd0hph,mean_dF,sigma_dF);

  c13->cd(4);
  t->Draw("pt0-pt_hel>>hd0hp(100,-0.05,0.05)","","");
  TH1F *hd0hp = (TH1F*) gROOT->FindObject("hd0hp");
  FitGauss(hd0hp,mean_dP,sigma_dP);

  if(log_hel) {
    fout<<"\n"
	<<setw(18)<<"dZ(cm)"<<"  "
	<<setw(18)<<"dTheta(rad)"<<"  "
	<<setw(18)<<"dPhi(rad)"<<"  "
	<<setw(19)<<"dPt(GeV/c)"<<"  "
	<<"FileName"<<"  "<<endl;
    fout<<fixed<<setprecision(3)
	<<setw(8)<<mean_dZ<<" +/- " <<setw(5)<<sigma_dZ<<"  "
	<<setw(8)<<mean_dT<<" +/- " <<setw(5)<<sigma_dT<<"  "
	<<setw(8)<<mean_dF<<" +/- " <<setw(5)<<sigma_dF<<"  "
	<<setprecision(4)
	<<setw(8)<<mean_dP<<" +/- " <<setw(6)<<sigma_dP<<"  "
	<<"global_helix_fit"<<endl;
  }
  c13->SaveAs(Form("Graph/thrown_vs_hel_%s.png",key));

  //////////////////////////////////////////////////////////////
  TCanvas *c12 = new TCanvas("c12","",900,700);
  c12->Divide(2,2);
  c12->cd(1);
  t->Draw("z0-z_rec>>hd0z(80,-1,1)","","");
  TH1F *hd0z = (TH1F*) gROOT->FindObject("hd0z");
  FitGauss(hd0z,mean_dZ,sigma_dZ);

  c12->cd(2);
  t->Draw("th0-th_rec>>hd0t(40,-0.2,0.2)","","");
  TH1F *hd0t = (TH1F*) gROOT->FindObject("hd0t");
  FitGauss(hd0t,mean_dT,sigma_dT);

  c12->cd(3);
  t->Draw("ph0-ph_rec>>hd0ph(100,-0.5,0.5)","","");
  TH1F *hd0ph = (TH1F*) gROOT->FindObject("hd0ph");
  FitGauss(hd0ph,mean_dF,sigma_dF);

  c12->cd(4);
  t->Draw("pt0-pt_rec>>hd0p(100,-0.05,0.05)","","");
  TH1F *hd0p = (TH1F*) gROOT->FindObject("hd0p");
  FitGauss(hd0p,mean_dP,sigma_dP);
 
  c12->SaveAs(Form("Graph/thrown_vs_rec_%s.png",key));

  if(log_rec) {
	TString filename = gFile->GetName();
	
	if(filename.Contains(".root")) filename.Remove(filename.Index(".root"),5);
	if(filename.Contains("nt_")) filename.Remove(filename.Index("nt_"),3);
	
    fout<<fixed<<setprecision(3)
	<<setw(8)<<mean_dZ<<" +/- " <<setw(5)<<sigma_dZ<<"  "
	<<setw(8)<<mean_dT<<" +/- " <<setw(5)<<sigma_dT<<"  "
	<<setw(8)<<mean_dF<<" +/- " <<setw(5)<<sigma_dF<<"  "
	<<setprecision(4)
	<<setw(8)<<mean_dP<<" +/- " <<setw(6)<<sigma_dP<<"  "
	<<filename.Data()<<endl;
  }
  fout.close();
  
  //////////////////////////////////////////////////////////////
  TCanvas *c11 = new TCanvas("c11","",900,700);
  c11->Divide(2,2);
  c11->cd(1);
  t->Draw("z_hel-z_rec>>hdz(80,-1,1)","","");
  TH1F *hdz = (TH1F*) gROOT->FindObject("hdz");
  FitGauss(hdz);

  c11->cd(2);
  t->Draw("th_hel-th_rec>>hdt(40,-0.2,0.2)","","");
  TH1F *hdt = (TH1F*) gROOT->FindObject("hdt");
  FitGauss(hdt);

  c11->cd(3);
  t->Draw("ph_hel-ph_rec>>hdph(100,-0.5,0.5)","","");
  TH1F *hdph = (TH1F*) gROOT->FindObject("hdph");
  FitGauss(hdph);

  c11->cd(4);
  t->Draw("pt_hel-pt_rec>>hdp(100,-0.05,0.05)","","");
  TH1F *hdp = (TH1F*) gROOT->FindObject("hdp");
  FitGauss(hdp);

  c11->SaveAs(Form("Graph/hel_vs_rec_%s.png",key));

}
