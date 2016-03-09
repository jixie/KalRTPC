#include "TKalDetCradle.h"    // from KalTrackLib
#include "TKalTrackState.h"   // from KalTrackLib
#include "TKalTrackSite.h"    // from KalTrackLib
#include "TKalTrack.h"        // from KalTrackLib

#include "EXKalDetector.h"
#include "EXEventGen.h"
#include "EXHit.h"
#include "EXKalRTPC.h"

#include <iomanip>
#include <iostream>
using namespace std;

static const double kMpr = 0.938272013;
static const double kPi = atan(1.0)*4;
static const Bool_t kDir = kIterBackward;
//static const Bool_t kDir = kIterForward;

///////////////////////////////////////////////////////////////////////
#define _EXKalTestDebug_ 3

TFile *gMyFile=0;    //I can not use gFile since it has been used by root
TTree *gMyTree=0;
int _index_=0;
double p_rec,pt_rec,pz_rec,th_rec,ph_rec,x_rec,y_rec,z_rec;
double r_rec,a_rec,b_rec;
int npt;
double step_x[MaxHit],step_y[MaxHit],step_z[MaxHit];
double step_px[MaxHit],step_py[MaxHit],step_pz[MaxHit];
double step_bx[MaxHit],step_by[MaxHit],step_bz[MaxHit];
int step_status[MaxHit];
double step_x_exp[MaxHit],step_y_exp[MaxHit],step_z_exp[MaxHit];
double step_x_fil[MaxHit],step_y_fil[MaxHit],step_z_fil[MaxHit];

//varible from global fitter, come from g4 root tree 
double p0,pt0,pz0,th0,ph0,_x0_,_y0_,_z0_;
double p_hel,pt_hel,pz_hel,th_hel,ph_hel,x_hel,y_hel,z_hel;
double r_hel,a_hel,b_hel;

int ndf;
double chi2,cl;

///////////////////////////////////////////////////////////////////////

int GetVextex(THelicalTrack &hel, Double_t x_bpm, Double_t y_bpm, 
  TVector3 &xx,  Double_t &dfi, double &r_rec, double &a_rec, double &b_rec)
{
  r_rec = fabs(hel.GetRho());
  a_rec = hel.GetXc();
  b_rec = hel.GetYc();

  Double_t Rho2BPM = sqrt((a_rec-x_bpm)*(a_rec-x_bpm)+(b_rec-y_bpm)*(b_rec-y_bpm));
  Double_t pRVx = fabs(Rho2BPM - r_rec) + 1.0E-6;

  TCylinder pCylVx(pRVx,50.0,x_bpm,y_bpm,0.0);

  //get the vertex point from here, xx and dfi are outputs
  const TVTrack &trk = dynamic_cast<const TVTrack &>(hel);
  int n=pCylVx.CalcXingPointWith(trk,xx,dfi,0);
  cout<<"GetVextex():  dfi="<<dfi*57.3<<"deg,   xx=("<<xx.X()<<", "<<xx.y()<<", "<<xx.Z()<<")"<<endl;
  return n;
}


int GetVextex2(THelicalTrack &hel, double x_bpm, double y_bpm, 
  TVector3 &xx,  double &dfi, double &r_rec, double &a_rec, double &b_rec)
{
  TVector3 X0  = hel.GetPivot(); 
  r_rec = fabs(hel.GetRho());
  a_rec = hel.GetXc();
  b_rec = hel.GetYc();
  
  dfi = atan2(y_bpm-b_rec,x_bpm-a_rec)-atan2(X0.Y()-b_rec,X0.X()-a_rec);
  if( fabs(dfi) > kPi ) {
    dfi = (dfi>0) ? dfi-2*kPi : dfi+2*kPi;
  } 
  xx = hel.CalcXAt(dfi);
  cout<<"GetVextex2(): dfi="<<dfi*57.3<<"deg,   xx=("<<xx.X()<<", "<<xx.y()<<", "<<xx.Z()<<")"<<endl;
  return 1;
}


void GetVarFromState(TVKalState &state, double &p, double &pt, double &pz, 
  double &th, double &ph, double &x, double &y, double &z, 
  double &r_rec, double &a_rec, double &b_rec )
{ 
  TVector3 xv(-99,-99,-99);
  Double_t dfi=0;
  THelicalTrack hel = (dynamic_cast<TKalTrackState *>(&state))->GetHelix();
  
  //by helix definition" Pz = Pt * tanLambda, tanLambda = ctan(theta)
  double tanLambda = state(4,0); //or tanLambda = hel.GetTanLambda(); 
  double cpa = state(2,0);       //or cpa = hel.GetKappa(); 
  double fi0 = state(1,0);       //or fi0 = hel.GetPhi0(); 
  double rho = hel.GetRho();

  
  //GetVextex(hel,0,0,xv,dfi,r_rec,a_rec,b_rec);
  GetVextex2(hel,0,0,xv,dfi,r_rec,a_rec,b_rec);
  //this is the vertex
  x=xv.X();
  y=xv.Y();
  z=xv.Z();
  
  pt = fabs(1.0/cpa);
  pz = pt * fabs(tanLambda);
  p  = pt * sqrt(1+tanLambda*tanLambda);   //p = pt / sinTheta 
  th = (tanLambda>0) ? asin(pt/p) : kPi-asin(pt/p);
  double fi0_vx = fi0 + dfi;  //vertex point fi0 angle in helix coordinate system
  if(cpa>0) fi0_vx += kPi;    //by definition, positive helix fi0 is differ by pi
  double phi_c_hall= fi0_vx - kPi ;  //phi angle of the helix center at hall coordinate system
  ph = (rho>0) ? phi_c_hall+kPi/2. : phi_c_hall-kPi/2.;
  if(ph> kPi) ph-=2*kPi; 
  if(ph<-kPi) ph+=2*kPi; 
  
  //debug the vertex reconstruction
  TVector3 xl=hel.GetPivot();
  cout<<"   Last Hit=("<<xl.X()<<", "<<xl.y()<<", "<<xl.Z()<<")"
    <<",   Xc="<<hel.GetXc()<<",  Yc="<<hel.GetYc()<<endl;

  cout<<"Rec. to Vextex: p="<<p<<" th="<<th*57.3<<", ph="<<ph*57.3
    <<", rho="<<rho<<", fi0="<<fi0*57.3<<"deg,  fi0_vx="<<fi0_vx*57.3<<"deg"<<endl;
  
}



void Tree_Init()
{
  gMyFile = new TFile("h.root","RECREATE","Kalman Filter for RTPC track");
  gMyTree = new TTree("t", "Kalman Filter for RTPC track");

  TTree *t=gMyTree;
  t->Branch("index",&_index_,"index/I");

  t->Branch("p_rec",&p_rec,"p_rec/D");
  t->Branch("pt_rec",&pt_rec,"pt_rec/D");
  t->Branch("pz_rec",&pz_rec,"pz_rec/D");
  t->Branch("th_rec",&th_rec,"th_rec/D");
  t->Branch("ph_rec",&ph_rec,"ph_rec/D");
  t->Branch("x_rec",&x_rec,"x_rec/D");
  t->Branch("y_rec",&y_rec,"y_rec/D");
  t->Branch("z_rec",&z_rec,"z_rec/D");
  t->Branch("r_rec",&r_rec,"r_rec/D");
  t->Branch("a_rec",&a_rec,"a_rec/D");
  t->Branch("b_rec",&b_rec,"b_rec/D");

  t->Branch("npt",&npt,"npt/I");
  t->Branch("step_x",step_x,"step_x[npt]/D");
  t->Branch("step_y",step_y,"step_y[npt]/D");
  t->Branch("step_z",step_z,"step_z[npt]/D");
  t->Branch("step_px",step_px,"step_px[npt]/D");
  t->Branch("step_py",step_py,"step_py[npt]/D");
  t->Branch("step_pz",step_pz,"step_pz[npt]/D");
  t->Branch("step_bx",step_bx,"step_bx[npt]/D");
  t->Branch("step_by",step_by,"step_by[npt]/D");
  t->Branch("step_bz",step_bz,"step_bz[npt]/D");
  t->Branch("step_status",step_status,"step_status[npt]/I");

  t->Branch("step_x_exp",step_x_exp,"step_x_exp[npt]/D");
  t->Branch("step_y_exp",step_y_exp,"step_y_exp[npt]/D");
  t->Branch("step_z_exp",step_z_exp,"step_z_exp[npt]/D");

  t->Branch("step_x_fil",step_x_fil,"step_x_fil[npt]/D");
  t->Branch("step_y_fil",step_y_fil,"step_y_fil[npt]/D");
  t->Branch("step_z_fil",step_z_fil,"step_z_fil[npt]/D");

  t->Branch("p0",&p0,"p0/D");
  t->Branch("pt0",&pt0,"pt0/D");
  t->Branch("pz0",&pz0,"pz0/D");
  t->Branch("th0",&th0,"th0/D");
  t->Branch("ph0",&ph0,"ph0/D");
  t->Branch("x0",&_x0_,"x0/D");
  t->Branch("y0",&_y0_,"y0/D");
  t->Branch("z0",&_z0_,"z0/D");

  t->Branch("p_hel",&p_hel,"p_hel/D");
  t->Branch("pt_hel",&pt_hel,"pt_hel/D");
  t->Branch("pz_hel",&pz_hel,"pz_hel/D");
  t->Branch("th_hel",&th_hel,"th_hel/D");
  t->Branch("ph_hel",&ph_hel,"ph_hel/D");
  t->Branch("x_hel",&x_hel,"x_hel/D");
  t->Branch("y_hel",&y_hel,"y_hel/D");
  t->Branch("z_hel",&z_hel,"z_hel/D");
  t->Branch("r_hel",&r_hel,"r_hel/D");
  t->Branch("a_hel",&a_hel,"a_hel/D");
  t->Branch("b_hel",&b_hel,"b_hel/D");

  t->Branch("ndf",&ndf,"ndf/I");
  t->Branch("chi2",&chi2,"chi2/D");
  t->Branch("cl",&cl,"cl/D");

}

void Reset()
{
  p0=pt0=pz0=th0=ph0=_x0_=_y0_=_z0_=0.0;
  p_rec=pt_rec=pz_rec=th_rec=ph_rec=x_rec=y_rec=z_rec=0.0;
  r_rec=a_rec=b_rec=0.0;
  for(int i=0;i<MaxHit;i++) {
    step_x[i]=step_y[i]=step_z[i]=0.0;
    step_px[i]=step_py[i]=step_pz[i]=0.0;
    step_bx[i]=step_by[i]=step_bz[i]=0.0;
    step_status[i]=0;
    step_x_exp[i]=step_y_exp[i]=step_z_exp[i]=0.0;
    step_x_fil[i]=step_y_fil[i]=step_z_fil[i]=0.0;
  }

  p_hel=pt_hel=pz_hel=th_hel=ph_hel=x_hel=y_hel=z_hel=9999.0;
  r_hel=a_hel=b_hel=0.0; 
  ndf=0;
  chi2=cl=0.0;
}

//cerr << "Usage: "<<argv[0] <<" <job=0|1|2> <nevent> [pt_min_gev=0.1] [pt_max_gev=0.1]" << endl;
//cerr << "\t  job: 0 generate helix, 1 loadtrack from geant4 root file, 2 generate circle\n";
//cerr << "\t  nevents: number of events to generate \n";
//cerr << "\t  pt_min_gev and pt_max_gev: specifiy the range of pt in Gev \n";
//cerr << "\t  Note that if pt is negative then anti-clockwise track will be generated \n";
int KalRTPC (int job, int nevents, double pt_min, double pt_max)
{
  Tree_Init();

  double cosmin  = -0.000001; //-0.9;
  double cosmax  =  0.000001; //+0.9;
  // ===================================================================
  //  Prepare a detector
  // ===================================================================

  TObjArray     kalhits;    // hit buffer
  TKalDetCradle cradle;     // detctor system
  EXKalDetector detector;   // detector

  cradle.Install(detector); // install detector into its cradle
#ifdef __MS_OFF__
  cradle.SwitchOffMS();     // switch off multiple scattering
#endif

  // ===================================================================
  //  Prepare a Event Generator
  // ===================================================================

  EXEventGen gen(cradle, kalhits);

  // ===================================================================
  //  Event loop
  // ===================================================================

  for (Int_t eventno = 0; eventno < nevents; eventno++) { 
    
#ifdef _EXKalTestDebug_
    cerr << "\n------ Event " << eventno << " ------" << endl;
#endif
    // ---------------------------
    //  Reset hit data
    // ---------------------------

    kalhits.Delete();
    Reset();

    // ============================================================
    //  Generate a partcle and Swim the particle in detector
    // ============================================================

    if(job==0) {
      THelicalTrack hel = gen.GenerateHelix(pt_min,pt_max,cosmin,cosmax);
      gen.Swim(hel,kMpr);
    }
    else if(job==1) { 
      //by Jixie: read from root tree
      if(gen.LoadOneTrack()<0) break;
    }
    else {
      gen.GenCircle(pt_min,pt_max);
    }

    // ============================================================
    //  Do Kalman Filter
    // ============================================================

    Int_t i1, i2, i3;
    if (kDir == kIterBackward) {
      i3 = 0;
      i1 = kalhits.GetEntries() - 1;
      i2 = i1 / 2;
    } else {
      i1 = 0;
      i3 = kalhits.GetEntries() - 1;
      i2 = i3 / 2;
    }

    // ---------------------------
    //  Create a dummy site: sited
    // ---------------------------

    EXHit hitd = *dynamic_cast<EXHit *>(kalhits.At(i1));
    hitd(0,1) = 1.e6;   // give a huge error to d
    hitd(1,1) = 1.e6;   // give a huge error to z

    TKalTrackSite &sited = *new TKalTrackSite(hitd);
    sited.SetOwner();   // site owns states

    // ---------------------------
    // Create initial helix
    // ---------------------------

    EXHit   &h1 = *dynamic_cast<EXHit *>(kalhits.At(i1));   // first hit
    EXHit   &h2 = *dynamic_cast<EXHit *>(kalhits.At(i2));   // middle hit
    EXHit   &h3 = *dynamic_cast<EXHit *>(kalhits.At(i3));   // last hit
    TVector3 x1 = h1.GetMeasLayer().HitToXv(h1);
    TVector3 x2 = h2.GetMeasLayer().HitToXv(h2);
    TVector3 x3 = h3.GetMeasLayer().HitToXv(h3);
    double bfield = h1.GetBfield();  //in kGauss
    THelicalTrack helstart(x1, x2, x3, bfield, kDir); // initial helix 


    THelicalTrack g4hel=gen.DoHelixFit();

    //just for debug
    cout<<"Event "<<eventno<<",  Initial Helix Rho="<<helstart.GetRho()
      <<" ==>  Pt="<<0.3*bfield/10.*fabs(helstart.GetRho())/100.<<endl;
    cout<<"Event "<<eventno<<",  g4 Helix Rho="<<g4hel.GetRho()
      <<" ==>  Pt="<<0.3*bfield/10.*fabs(g4hel.GetRho())/100.<<endl;

    if(job==1) helstart=g4hel;
    // ---------------------------
    //  Set dummy state to sited
    // ---------------------------

    static TKalMatrix svd(kSdim,1);
    svd(0,0) = 0.;
    svd(1,0) = helstart.GetPhi0();
    svd(2,0) = helstart.GetKappa();
    svd(3,0) = 0.;
    svd(4,0) = helstart.GetTanLambda();
    if (kSdim == 6) svd(5,0) = 0.;

    static TKalMatrix C(kSdim,kSdim);
    for (Int_t i=0; i<kSdim; i++) {
      //C(i,i) = 1.e4;   // dummy error matrix
      C(i,i) = 1.e0;   // dummy error matrix
    }

    sited.Add(new TKalTrackState(svd,C,sited,TVKalSite::kPredicted));
    sited.Add(new TKalTrackState(svd,C,sited,TVKalSite::kFiltered));

    // ---------------------------
    //  Add sited to the kaltrack
    // ---------------------------

    TKalTrack kaltrack;    // a track is a kal system
    kaltrack.SetMass(kMpr);
    kaltrack.SetOwner();   // kaltrack owns sites
    kaltrack.Add(&sited);  // add the dummy site to the track

    // ---------------------------
    //  Prepare hit iterrator
    // ---------------------------

    TIter next(&kalhits, kDir);   // come in to IP

    // ---------------------------
    //  Start Kalman Filter
    // ---------------------------

    npt = 0;
    EXHit *hitp = dynamic_cast<EXHit *>(next());
    while (hitp) {     // loop over hits    
      const EXMeasLayer &ml = dynamic_cast<const EXMeasLayer &>(hitp->GetMeasLayer());
      //fill the global variables for root tree
      TVector3 xraw = hitp->GetRawXv();
      TVector3 xv = ml.HitToXv(*hitp);
      step_x[npt]=xraw.X();step_y[npt]=xraw.Y();step_z[npt]=xraw.Z();
      step_x_exp[npt]=xv.X();step_y_exp[npt]=xv.Y();step_z_exp[npt]=xv.Z();
      step_status[npt]=1;

#ifdef _EXKalTestDebug_
      if(_EXKalTestDebug_ >= 3) {
	cerr << "Main(): MeasLayer "<<setw(2)<<ml.GetIndex()
	  <<": R="<<setw(6)<<ml.GetR()<<": ";
	cerr << "xv=("<<setw(8)<<xv.X()<<",  "<<setw(8)<<xv.Y()
	  <<", "<<setw(8)<<xv.Z()<<"): \n";
      }
#endif

      TKalTrackSite  &site = *new TKalTrackSite(*hitp); // create a site for this hit

      if (!kaltrack.AddAndFilter(site)) {               // add and filter this site
	step_status[npt]=0;
	cerr << "Fitter: site "<<npt<<" discarded: "
	  << " xv=("<< xv.X()<<",  "<<xv.Y()<<", "<<xv.Z()<<")"<< endl;           
	delete &site;                                  // delete this site, if failed
      }
      else {
	//get filtered state then convert it to THelicalTrack 
	//both ways to get state will work
	//TVKalState &state_fil1 = kaltrack.GetState(TVKalSite::kFiltered);
	//TVKalState *state_fil2 = &kaltrack.GetState(TVKalSite::kFiltered);
	TVKalState *state_fil = (TVKalState*) &(site.GetCurState());
	THelicalTrack hel_fil = (dynamic_cast<TKalTrackState *>(state_fil))->GetHelix();
	TVector3 x_fil=hel_fil.CalcXAt(0.0);
	step_x_fil[npt]=x_fil.X();step_y_fil[npt]=x_fil.Y();step_z_fil[npt]=x_fil.Z();

#ifdef _EXKalTestDebug_
	if(_EXKalTestDebug_ >= 4) {
	  TVKalState *state_exp = &site.GetState(TVKalSite::kPredicted);
	  THelicalTrack hel_exp = (dynamic_cast<TKalTrackState *>(state_exp))->GetHelix();
	  TVector3 x_exp=hel_exp.CalcXAt(0.0);

	  //debug: comparing the expected and filtered points
	  cerr << "Event "<<eventno<<": site "<<npt<<endl;
	  cerr << "\t xraw =("<< xraw.X()<<",  "<<xraw.Y()<<", "<<xraw.Z()<<"): \n";
	  cerr << "\t xv   =("<< xv.X()<<",  "<<xv.Y()<<", "<<xv.Z()<<"): \n";
	  cerr << "\t x_exp=("<< x_exp.X()<<",  "<<x_exp.Y()<<", "<<x_exp.Z()<<"): \n";
	  cerr << "\t x_fil=("<< x_fil.X()<<",  "<<x_fil.Y()<<", "<<x_fil.Z()<<"): \n";
	}
#endif
      }
      npt++;
      hitp = dynamic_cast<EXHit *>(next());
    }
    //kaltrack.SmoothBackTo(1);                          // smooth back.


    // ============================================================
    //  Monitor Fit Result
    // ============================================================
    //most of these are coming from  EXEventGen::NtReader, which is in mm
    p0=gen.P0_p;
    th0=gen.Theta0_p;
    ph0=gen.Phi0_p; 
    if(ph0> kPi) ph0-=2*kPi;
    if(ph0<-kPi) ph0+=2*kPi;
    pt0=p0*sin(th0);
    pz0=p0*cos(th0);
    _x0_=gen.X0/10.;
    _y0_=gen.Y0/10.;
    _z0_=gen.Z0/10.;

    p_hel=gen.P0_rec_p;
    th_hel=gen.Theta0_rec_p;
    ph_hel=gen.Phi0_rec_p; 
    pt_hel=p_hel*sin(th_hel);
    pz_hel=p0*cos(th_hel);
    x_hel=gen.X0_rec_p/10.;
    y_hel=gen.Y0_rec_p/10.;
    z_hel=gen.Z0_rec_p/10.;

    r_hel=gen.R_rec/10.;
    a_hel=gen.A_rec/10.;
    b_hel=gen.B_rec/10.;


    TVKalState *theLastState = (TVKalState*) &(kaltrack.GetCurSite().GetCurState());
    GetVarFromState(*theLastState, p_rec, pt_rec, pz_rec, th_rec, ph_rec, 
      x_rec, y_rec, z_rec, r_rec, a_rec, b_rec );

    if(job==1) {
      //G4 tracks phi angle is differed by PI 
      //ph_rec+=kPi;
      if(ph_rec> kPi) ph_rec-=2*kPi; 
      if(ph_rec<-kPi) ph_rec+=2*kPi; 
    }

    ndf  = kaltrack.GetNDF();
    chi2 = kaltrack.GetChi2();
    cl   = TMath::Prob(chi2, ndf);

    gMyTree->Fill();
    _index_++;
  }

  gMyFile->Write();

  return 0;
}
