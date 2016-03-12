
#include "EXKalRTPC.h"
#include "GlobalDebuger.hh"

#include <iomanip>
#include <iostream>
using namespace std;

static const double kMpr = 0.938272013;
static const double kPi = atan(1.0)*4;
static const Bool_t kDir = kIterBackward;
//static const Bool_t kDir = kIterForward;

///////////////////////////////////////////////////////////////////////
//#define _EXKalTestDebug_ 3

///////////////////////////////////////////////////////////////////////

EXKalRTPC::EXKalRTPC()
{
#ifdef _EXKalTestDebug_
  Global_Debug_Level=_EXKalTestDebug_;
#endif

  // ===================================================================
  //  Prepare a fDetector
  // ===================================================================

  fKalHits = new TObjArray();       // hit buffer
  fCradle  = new TKalDetCradle();   // detctor system
  fDetector= new EXKalDetector();   // fDetector

  fCradle->Install(*fDetector);     // install fDetector into its fCradle

#ifdef __MS_OFF__
  fCradle.SwitchOffMS();            // switch off multiple scattering
#endif

  // ===================================================================
  //  Prepare a Event Generator
  // ===================================================================
  fEventGen = new EXEventGen(*fCradle, *fKalHits);


  // ===================================================================
  //  Prepare a Root tree output
  // ===================================================================
  fCovMElement=5.0e-2;
  Tree_Init();
  _index_=0;
}

EXKalRTPC::~EXKalRTPC()
{
  delete fEventGen;
  delete fDetector;
  delete fCradle;
  delete fKalHits;
}


int EXKalRTPC::GetVextex(THelicalTrack &hel, Double_t x_bpm, Double_t y_bpm, 
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
#ifdef _EXKalTestDebug_
  cout<<"GetVextex():  dfi="<<dfi*57.3<<"deg,   xx=("<<xx.X()<<", "<<xx.y()<<", "<<xx.Z()<<")"<<endl;
#endif
  return n;
}


int EXKalRTPC::GetVextex2(THelicalTrack &hel, double x_bpm, double y_bpm, 
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
#ifdef _EXKalTestDebug_
  cout<<"GetVextex2(): dfi="<<dfi*57.3<<"deg,   xx=("<<xx.X()<<", "<<xx.y()<<", "<<xx.Z()<<")"<<endl;
#endif  
  return 1;
}


void EXKalRTPC::ReconVertex(TVKalState &state, double &p, double &pt, double &pz, 
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
  //phi_vx_hel is the phi angle of vertex point on the circle coordinate system
  //by definition, positive helix fi0 is differ by pi from circle 
  double phi_vx_hel=(cpa>0)?fi0_vx+kPi:fi0_vx;  
  double phi_c_hall= phi_vx_hel - kPi ;  //phi angle of the helix center at hall coordinate system
  ph = (rho>0) ? phi_c_hall+kPi/2. : phi_c_hall-kPi/2.;
  if(ph> kPi) ph-=2*kPi; 
  if(ph<-kPi) ph+=2*kPi; 

#ifdef _EXKalTestDebug_
  //debug the vertex reconstruction
  if(Global_Debug_Level>=1) {
    TVector3 xl=hel.GetPivot();
    cout<<"   Last Hit=("<<xl.X()<<", "<<xl.y()<<", "<<xl.Z()<<")"
      <<",   Xc="<<hel.GetXc()<<",  Yc="<<hel.GetYc()<<endl;

    cout<<"Rec. to Vextex: p="<<p<<" ph="<<ph*57.3<<", th="<<th*57.3
      <<", rho="<<rho<<", fi0="<<fi0*57.3<<"deg,  fi0_vx="<<fi0_vx*57.3<<"deg"<<endl;
  }
#endif  
}


void EXKalRTPC::Tree_Init()
{
  fFile = new TFile("h.root","RECREATE","Kalman Filter for RTPC track");
  fTree = new TTree("t", "Kalman Filter for RTPC track");

  TTree *t=fTree;
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
  //t->Branch("step_px",step_px,"step_px[npt]/D");
  //t->Branch("step_py",step_py,"step_py[npt]/D");
  //t->Branch("step_pz",step_pz,"step_pz[npt]/D");
  //t->Branch("step_bx",step_bx,"step_bx[npt]/D");
  //t->Branch("step_by",step_by,"step_by[npt]/D");
  //t->Branch("step_bz",step_bz,"step_bz[npt]/D");
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

void EXKalRTPC::Reset()
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

void EXKalRTPC::BeginOfRun()
{
}

void EXKalRTPC::EndOfRun()
{
}

//prepare a track from xyz array, make sure radii are in increasing order  
bool EXKalRTPC::PrepareATrack(double *x_mm, double *y_mm,double *z_mm, int npt)
{
  bool smearing=false;
  fEventGen->MakeHitsFromTraj(x_mm,y_mm,z_mm,npt,smearing);
  return true;
}

//Let the event generator to generate a track
bool EXKalRTPC::PrepareATrack(int job, double pt_min, double pt_max, double costh_min, double costh_max)
{
  if(job==0) {
    THelicalTrack hel = fEventGen->GenerateHelix(pt_min,pt_max,costh_min,costh_max);
    fEventGen->Swim(hel,kMpr);
  }
  else if(job==1) { 
    //by Jixie: read from root tree
    if(fEventGen->LoadOneTrack()<0) return false;
  }
  else {
    fEventGen->GenCircle(pt_min,pt_max);
  }
  return true;
}

//cerr << "Usage: "<<argv[0] <<" <job=0|1|2> <nevent> [pt_min_gev=0.1] [pt_max_gev=0.1]" << endl;
//cerr << "\t  job: 0 generate helix, 1 loadtrack from geant4 root file, 2 generate circle\n";
//cerr << "\t  nevents: number of events to generate \n";
//cerr << "\t  pt_min_gev and pt_max_gev: specifiy the range of pt in Gev \n";
//cerr << "\t  Note that if pt is negative then anti-clockwise track will be generated \n";
int EXKalRTPC::KalRTPC(int job, int nevents, double pt_min, double pt_max, double costh_min, double costh_max)
{

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

    fKalHits->Delete();
    Reset();
    
    // ============================================================
    //  Generate a partcle and Swim the particle in fDetector
    // ============================================================
    bool ret=PrepareATrack(job, pt_min, pt_max, costh_min, costh_max);
    if(!ret) break;

    // ============================================================
    //  Do Kalman Filter
    // ============================================================

    // ---------------------------
    // Create initial helix
    // ---------------------------
    // initial helix 
    THelicalTrack hel_3point = fEventGen->CreateInitialHelix(kDir); 
    THelicalTrack helstart = fEventGen->DoHelixFit();    

    //for some curve back tracks, global helix fit fail
    if(helstart.GetRho()*hel_3point.GetRho()<0) helstart=hel_3point;

    // ---------------------------
    //  Create a dummy site: sited
    // ---------------------------
    Int_t i1 = (kDir == kIterBackward) ? fKalHits->GetEntries()-1 : 0;
  
    EXHit hitd = *dynamic_cast<EXHit *>(fKalHits->At(i1));
    hitd(0,1) = 1.e6;   // give a huge error to d
    hitd(1,1) = 1.e6;   // give a huge error to z

    TKalTrackSite &sited = *new TKalTrackSite(hitd);
    sited.SetOwner();   // site owns states

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
      //C(i,i) = 1.e0;         // dummy error matrix
      C(i,i) = fCovMElement;   // dummy error matrix
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

    TIter next(fKalHits, kDir);   // come in to IP

    // ---------------------------
    //  Start Kalman Filter
    // ---------------------------

    npt = 0;
    EXHit *hitp = dynamic_cast<EXHit *>(next());
    while (hitp) {     // loop over hits    
      const EXMeasLayer &ml = dynamic_cast<const EXMeasLayer &>(hitp->GetMeasLayer());
      TVector3 xraw = hitp->GetRawXv();
      TVector3 xv = ml.HitToXv(*hitp);

      step_status[npt]=1;

#ifdef _EXKalTestDebug_
      if(Global_Debug_Level >= 3) {
	cerr << "MeasLayer "<<setw(2)<<ml.GetIndex()
	  <<": R="<<setw(6)<<ml.GetR()<<": ";
	cerr << "xraw=("<<setw(8)<<xraw.X()<<",  "<<setw(8)<<xraw.Y()
	  <<", "<<setw(8)<<xraw.Z()<<"): ";
	cerr << "xv=("<<setw(8)<<xv.X()<<",  "<<setw(8)<<xv.Y()
	  <<", "<<setw(8)<<xv.Z()<<"): \n";
      }
#endif

      TKalTrackSite  &site = *new TKalTrackSite(*hitp); // create a site for this hit

      if (!kaltrack.AddAndFilter(site)) {               // add and filter this site
	step_status[npt]=0;
	cerr << "Fitter: site "<<npt<<" discarded: "
	  << " xv=("<< xv.X()<<",  "<<xv.Y()<<", "<<xv.Z()<<")"<< endl;           
	delete &site;                                   // delete this site, if failed      

#ifdef _EXKalTestDebug_                          
	Pause4Debug();
#endif
      }
      else {
#ifdef _EXKalTestDebug_
	//get filtered state then convert it to THelicalTrack 
	//both ways to get state will work
	//TVKalState &state_fil1 = kaltrack.GetState(TVKalSite::kFiltered);
	//TVKalState *state_fil2 = &kaltrack.GetState(TVKalSite::kFiltered);

	if(Global_Debug_Level >= 4) {
	  TVKalState *state_fil = (TVKalState*) &(site.GetCurState());
	  THelicalTrack hel_fil = (dynamic_cast<TKalTrackState *>(state_fil))->GetHelix();
	  TVector3 x_fil=hel_fil.CalcXAt(0.0);

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

    TVKalState *theLastState = (TVKalState*) &(kaltrack.GetCurSite().GetCurState());
    ReconVertex(*theLastState, p_rec, pt_rec, pz_rec, th_rec, ph_rec, 
      x_rec, y_rec, z_rec, r_rec, a_rec, b_rec );
    
#ifdef _EXKalTestDebug_
    if(Global_Debug_Level >= 4) {
      TKalMatrix pC = theLastState->GetCovMat();
      pC.Print(); 
    }
#endif

    ndf  = kaltrack.GetNDF();
    chi2 = kaltrack.GetChi2();
    cl   = TMath::Prob(chi2, ndf);
    
    Tree_Fill(kaltrack);

#ifdef _EXKalTestDebug_
    Stop4Debug(_index_);
#endif

  }

  fFile->Write();

  return 1;
}

//I separate filling the tree into an individual subroutine since 
//filling the tree will not be necessary when provided to CLAS12 software
void EXKalRTPC::Tree_Fill(TKalTrack &kaltrack)
{
  //most of these are coming from  EXEventGen::NtReader, which is in mm

  TIter next(fKalHits, kDir);   

  int npt=0, iSite=0;
  EXHit *hitp = dynamic_cast<EXHit *>(next());
  while (hitp) {     // loop over hits    
    const EXMeasLayer &ml = dynamic_cast<const EXMeasLayer &>(hitp->GetMeasLayer());
    //fill the global variables for root tree
    TVector3 xraw = hitp->GetRawXv();
    TVector3 xv = ml.HitToXv(*hitp);
    step_x[npt]=xraw.X();step_y[npt]=xraw.Y();step_z[npt]=xraw.Z();
    step_x_exp[npt]=xv.X();step_y_exp[npt]=xv.Y();step_z_exp[npt]=xv.Z();

    //filtered state vector exist only if it is not removed
    if(step_status[npt]) {
      TKalTrackSite *site = (TKalTrackSite *)kaltrack.At(iSite++); // load the site
      TVKalState *state_fil = (TVKalState*) &(site->GetCurState());
      THelicalTrack hel_fil = (dynamic_cast<TKalTrackState *>(state_fil))->GetHelix();
      TVector3 x_fil=hel_fil.CalcXAt(0.0);
      step_x_fil[npt]=x_fil.X();step_y_fil[npt]=x_fil.Y();step_z_fil[npt]=x_fil.Z();
    }
    hitp = dynamic_cast<EXHit *>(next());
    npt++;
  }


  p0=fEventGen->P0_p;
  th0=fEventGen->Theta0_p;
  ph0=fEventGen->Phi0_p; 
  if(ph0> kPi) ph0-=2*kPi;
  if(ph0<-kPi) ph0+=2*kPi;
  pt0=p0*sin(th0);
  pz0=p0*cos(th0);
  _x0_=fEventGen->X0/10.;
  _y0_=fEventGen->Y0/10.;
  _z0_=fEventGen->Z0/10.;

  p_hel=fEventGen->P0_rec_p;
  th_hel=fEventGen->Theta0_rec_p;
  ph_hel=fEventGen->Phi0_rec_p; 
  pt_hel=p_hel*sin(th_hel);
  pz_hel=p0*cos(th_hel);
  x_hel=fEventGen->X0_rec_p/10.;
  y_hel=fEventGen->Y0_rec_p/10.;
  z_hel=fEventGen->Z0_rec_p/10.;

  r_hel=fEventGen->R_rec/10.;
  a_hel=fEventGen->A_rec/10.;
  b_hel=fEventGen->B_rec/10.;


  fTree->Fill();
  _index_++;
}


int EXKalRTPC::DoFitAndFilter(double *x_mm, double *y_mm, double *z_mm, int n)
{
  if(n<5) return 0;
  // ============================================================
  //  Reset hit data
  // ============================================================
  fKalHits->Delete();
  Reset();

  // ============================================================
  //  Generate a partcle and Swim the particle in fDetector
  // ============================================================
  PrepareATrack(x_mm, y_mm, z_mm, n);

  // ============================================================
  //  Do Kalman Filter
  // ============================================================

  // ---------------------------
  // Create initial helix
  // ---------------------------

  // initial helix 
  THelicalTrack hel_3point = fEventGen->CreateInitialHelix(kDir); 
  THelicalTrack helstart = fEventGen->DoHelixFit();    

  //for some curve back tracks, global helix fit fail
  if(helstart.GetRho()*hel_3point.GetRho()<0) helstart=hel_3point;

  // ---------------------------
  //  Create a dummy site: sited
  // ---------------------------
  
  Int_t i1 = (kDir == kIterBackward) ? fKalHits->GetEntries() - 1 : 0;
  EXHit hitd = *dynamic_cast<EXHit *>(fKalHits->At(i1));
  hitd(0,1) = 1.e6;   // give a huge error to d
  hitd(1,1) = 1.e6;   // give a huge error to z

  TKalTrackSite &sited = *new TKalTrackSite(hitd);
  sited.SetOwner();   // site owns states


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
    C(i,i) = fCovMElement;   // dummy error matrix
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

  TIter next(fKalHits, kDir);   // come in to IP

  // ---------------------------
  //  Start Kalman Filter
  // ---------------------------

  npt = 0;
  EXHit *hitp = dynamic_cast<EXHit *>(next());
  while (hitp) {     // loop over hits    
    step_status[npt]=1;
    TKalTrackSite  &site = *new TKalTrackSite(*hitp); // create a site for this hit

    if (!kaltrack.AddAndFilter(site)) {               // add and filter this site
      step_status[npt]=0;
      const EXMeasLayer &ml = dynamic_cast<const EXMeasLayer &>(hitp->GetMeasLayer());
      TVector3 xv = ml.HitToXv(*hitp);
      cerr << "Fitter: site "<<npt<<" discarded: "
	<< " xv=("<< xv.X()<<",  "<<xv.Y()<<", "<<xv.Z()<<")"<< endl;           
      delete &site;                                  // delete this site, if failed

    }
    npt++;
    hitp = dynamic_cast<EXHit *>(next());
  }
  //kaltrack.SmoothBackTo(1);                          // smooth back.

  // ============================================================
  //  reconstruct to the vertrex
  // ============================================================

  TVKalState *theLastState = (TVKalState*) &(kaltrack.GetCurSite().GetCurState());
  ReconVertex(*theLastState, p_rec, pt_rec, pz_rec, th_rec, ph_rec, 
    x_rec, y_rec, z_rec, r_rec, a_rec, b_rec);

  ndf  = kaltrack.GetNDF();
  chi2 = kaltrack.GetChi2();
  cl   = TMath::Prob(chi2, ndf);

  return 1;
}
