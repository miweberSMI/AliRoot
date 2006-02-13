/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

/*$Log$
author: Chiara Zampolli, zampolli@bo.infn.it
 */  

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// class for TOF calibration                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "AliTOFcalib.h"
#include "AliRun.h"
#include <TTask.h>
#include <TFile.h>
#include <TROOT.h>
#include <TSystem.h>
#include "AliTOF.h"
#include "AliTOFcalibESD.h"
#include "AliESD.h"
#include <TObject.h>
#include "TF1.h"
#include "TH1F.h"
#include "TH2F.h"
#include "AliESDtrack.h"
#include "AliTOFChannel.h"
#include "AliTOFChSim.h"
#include "AliTOFGeometry.h"
#include "AliTOFdigit.h"
#include "TClonesArray.h"
#include "AliTOFCal.h"
#include "TRandom.h"
#include "AliTOFcluster.h"
#include "TList.h"
#include "AliCDBManager.h"
#include "AliCDBMetaData.h"
#include "AliCDBStorage.h"
#include "AliCDBId.h"
#include "AliCDBEntry.h"

extern TROOT *gROOT;
extern TStyle *gStyle;

ClassImp(AliTOFcalib)

const Int_t AliTOFcalib::fgkchannel = 5000;
const Char_t* AliTOFcalib::ffile[6]={"$ALICE_ROOT/TOF/Spectra/spectrum0_1.root","$ALICE_ROOT/TOF/Spectra/spectrum0_2.root","$ALICE_ROOT/TOF/Spectra/spectrum1_1.root","$ALICE_ROOT/TOF/Spectra/spectrum1_2.root","$ALICE_ROOT/TOF/Spectra/spectrum2_1.root","$ALICE_ROOT/TOF/Spectra/spectrum2_2.root"};
//_______________________________________________________________________
AliTOFcalib::AliTOFcalib():TTask("AliTOFcalib",""){ 
  fNSector = 0;
  fNPlate  = 0;
  fNStripA = 0;
  fNStripB = 0;
  fNStripC = 0;
  fNpadZ = 0;
  fNpadX = 0;
  fsize = 0; 
  fArrayToT = 0x0;
  fArrayTime = 0x0;
  flistFunc = 0x0;
  fTOFCal = 0x0;
  fESDsel = 0x0;
  for (Int_t i = 0;i<6;i++){
    fhToT[i]=0x0;
  }
  fGeom=0x0; 
}
//____________________________________________________________________________ 

AliTOFcalib::AliTOFcalib(char* headerFile, Int_t nEvents):TTask("AliTOFcalib","") 
{
  AliRunLoader *rl = AliRunLoader::Open("galice.root",AliConfig::GetDefaultEventFolderName(),"read");  
  rl->CdGAFile();
  TFile *in=(TFile*)gFile;
  in->cd();
  fGeom = (AliTOFGeometry*)in->Get("TOFgeometry");
  fNSector = fGeom->NSectors();
  fNPlate  = fGeom->NPlates();
  fNStripA = fGeom->NStripA();
  fNStripB = fGeom->NStripB();
  fNStripC = fGeom->NStripC();
  fNpadZ = fGeom->NpadZ();
  fNpadX = fGeom->NpadX();
  fsize = 2*(fNStripC+fNStripB) + fNStripA; 
  for (Int_t i = 0;i<6;i++){
    fhToT[i]=0x0;
  }
  fArrayToT = 0x0;
  fArrayTime = 0x0;
  flistFunc = 0x0;
  fNevents=nEvents;
  fHeadersFile=headerFile;
  fTOFCal = 0x0;
  fESDsel = 0x0;
  TFile* file = (TFile*) gROOT->GetFile(fHeadersFile.Data()) ;
  if(file == 0){
    if(fHeadersFile.Contains("rfio"))
      file = TFile::Open(fHeadersFile,"update") ;
    else
      file = new TFile(fHeadersFile.Data(),"update") ;
    gAlice = (AliRun*)file->Get("gAlice") ;
  }
  
  TTask * roottasks = (TTask*)gROOT->GetRootFolder()->FindObject("Tasks") ; 
  roottasks->Add(this) ; 
}
//____________________________________________________________________________ 

AliTOFcalib::AliTOFcalib(const AliTOFcalib & calib):TTask("AliTOFcalib","")
{
  fNSector = calib.fNSector;
  fNPlate = calib.fNPlate;
  fNStripA = calib.fNStripA;
  fNStripB = calib.fNStripB;
  fNStripC = calib.fNStripC;
  fNpadZ = calib.fNpadZ;
  fNpadX = calib.fNpadX;
  fsize = calib.fsize;
  fArrayToT = calib.fArrayToT;
  fArrayTime = calib.fArrayTime;
  flistFunc = calib.flistFunc;
  for (Int_t i = 0;i<6;i++){
    fhToT[i]=calib.fhToT[i];
  }
  fTOFCal=calib.fTOFCal;
  fESDsel = calib.fESDsel;
  fGeom = calib.fGeom;
}

//____________________________________________________________________________ 

AliTOFcalib::~AliTOFcalib()
{
  delete fArrayToT;
  delete fArrayTime;
  delete flistFunc;
  delete[] fhToT; 
  delete fTOFCal;
  delete fESDsel;
}
//____________________________________________________________________________

void AliTOFcalib::Init(){
  SetHistos();
  SetFitFunctions();
  fTOFCal = new AliTOFCal();
  fTOFCal->CreateArray();
  fNSector = 18;
  fNPlate  = 5;
  fNStripA = 15;
  fNStripB = 19;
  fNStripC = 20;
  fNpadZ = 2;
  fNpadX = 96;
  fsize = 1; 
  //fsize = fNSector*(2*(fNStripC+fNStripB)+fNStripA())*fNpadZ*fNpadX; //generalized version
}
//____________________________________________________________________________

void AliTOFcalib::SetHistos(){
  TFile * spFile;
  TH1F * hToT;
  for (Int_t i =0;i<6;i++){
    //for the time being, only one spectrum is used
    spFile = new TFile("$ALICE_ROOT/TOF/Spectra/spectrumtest0_1.root","read");
    //otherwise
    //spFile = new TFile(ffile[i],"read");
    spFile->GetObject("ToT",hToT);
    fhToT[i]=hToT;
    Int_t nbins = hToT->GetNbinsX();
    Float_t Delta = hToT->GetBinWidth(1);
    Float_t maxch = hToT->GetBinLowEdge(nbins)+Delta;
    Float_t minch = hToT->GetBinLowEdge(1);
    Float_t max=0,min=0; //maximum and minimum value of the distribution
    Int_t maxbin=0,minbin=0; //maximum and minimum bin of the distribution
    for (Int_t ii=nbins; ii>0; ii--){
      if (hToT->GetBinContent(ii)!= 0) {
	max = maxch - (nbins-ii-1)*Delta;
	maxbin = ii; 
	break;}
    }
    for (Int_t j=1; j<nbins; j++){
      if (hToT->GetBinContent(j)!= 0) {
	min = minch + (j-1)*Delta;
	minbin = j; 
	break;}
    }
    fMaxToT[i]=max;
    fMinToT[i]=min;
    fMaxToTDistr[i]=hToT->GetMaximum();
  }
}
//__________________________________________________________________________

void AliTOFcalib::SetFitFunctions(){
  TFile * spFile;
  flistFunc = new TList();
  for (Int_t i =0;i<6;i++){
    //only one type of spectrum used for the time being
    spFile = new TFile("$ALICE_ROOT/TOF/Spectra/spectrumtest0_1.root","read");
    //otherwise
    //spFile = new TFile(ffile[i],"read");
    TH1F * h = (TH1F*)spFile->Get("TimeToTFit");
    TList * list = (TList*)h->GetListOfFunctions();
    TF1* f = (TF1*)list->At(0);
    Double_t par[6] = {0,0,0,0,0,0};
    Int_t npar=f->GetNpar();
    for (Int_t ipar=0;ipar<npar;ipar++){
      par[ipar]=f->GetParameter(ipar);
    }
    flistFunc->AddLast(f);
  }
  return;
}
//__________________________________________________________________________

TF1* AliTOFcalib::SetFitFunctions(TH1F *histo){
  TF1 * fpol[3];
  const Int_t nbins = histo->GetNbinsX();
  Float_t Delta = histo->GetBinWidth(1);  //all the bins have the same width
  Double_t max = histo->GetBinLowEdge(nbins)+Delta;
  max = 15;
  fpol[0]=new TF1("poly3","pol3",5,max);
  fpol[1]=new TF1("poly4","pol4",5,max);
  fpol[2]=new TF1("poly5","pol5",5,max);
  char npoly[10];
  Double_t chi[3]={1E6,1E6,1E6};
  Int_t ndf[3]={-1,-1,-1};
  Double_t Nchi[3]={1E6,1E6,1E6};
  Double_t bestchi=1E6;
  TF1 * fGold=0x0;
  Int_t nonzero =0;
  Int_t numberOfpar =0;
  for (Int_t j=0; j<nbins; j++){
    if (histo->GetBinContent(j)!=0) {
      nonzero++;
    }
  }
  Int_t norderfit = 0;
  if (nonzero<=4) {
    AliError(" Too few points in the histo. No fit performed.");
    return 0x0;
  }
  else if (nonzero<=5) {
    norderfit = 3;
    AliInfo(" Only 3rd order polynomial fit possible.");
  }
  else if (nonzero<=6) {
    norderfit = 4;
    AliInfo(" Only 3rd and 4th order polynomial fit possible.");
  }
  else {
    norderfit = 5;
    AliInfo(" All 3rd, 4th and 5th order polynomial fit possible.");
  }
  for (Int_t ifun=norderfit-3;ifun<norderfit-2;ifun++){
    sprintf(npoly,"poly%i",ifun+3);
    histo->Fit(npoly, "ERN", " ", 5.,14.);
    chi[ifun] = fpol[ifun]->GetChisquare();
    ndf[ifun] = fpol[ifun]->GetNDF();
    Nchi[ifun] = (Double_t)chi[ifun]/ndf[ifun];
    if (Nchi[ifun]<bestchi) {
      bestchi=Nchi[ifun];
      fGold = fpol[ifun];
      numberOfpar = fGold->GetNpar();
    }
  }
  fGold=fpol[2];  //Gold fit function set to pol5 in any case
  histo->Fit(fGold,"ER " ," ",5.,15.);
  return fGold;
}
//____________________________________________________________________________

TClonesArray* AliTOFcalib::DecalibrateDigits(TClonesArray * digits){

  TObjArray ChArray(fsize);
  ChArray.SetOwner();
  for (Int_t kk = 0 ; kk < fsize; kk++){
    AliTOFChSim * channel = new AliTOFChSim();
    ChArray.Add(channel);
  }
  Int_t ndigits = digits->GetEntriesFast();    
  for (Int_t i=0;i<ndigits;i++){
    Float_t trix = 0;
    Float_t triy = 0;
    Float_t SimToT = 0;
    AliTOFdigit * element = (AliTOFdigit*)digits->At(i);
    /*
    Int_t *detId[5];
    detId[0] = element->GetSector();
    detId[1] = element->GetPlate();
    detId[2] = element->GetStrip();
    detId[3] = element->GetPadz();
    detId[4] = element->GetPadx();
    Int_t index = GetIndex(detId);
    */
    //select the corresponding channel with its simulated ToT spectrum
    //summing up everything, index = 0 for all channels:
    Int_t index = 0;
    AliTOFChSim *ch = (AliTOFChSim*)ChArray.At(index);
    Int_t itype = -1;
    if (!ch->IsSlewed()){
      Double_t rand = gRandom->Uniform(0,6);
      itype = (Int_t)(6-rand);
      ch->SetSpectrum(itype);
      ch->SetSlewedStatus(kTRUE);
    }
    else itype = ch->GetSpectrum();
    TH1F * hToT = fhToT[itype];
    TF1 * f = (TF1*)flistFunc->At(itype);
    while (SimToT <= triy){
      trix = gRandom->Rndm(i);
      triy = gRandom->Rndm(i);
      trix = (fMaxToT[itype]-fMinToT[itype])*trix + fMinToT[itype]; 
      triy = fMaxToTDistr[itype]*triy;
      Int_t binx=hToT->FindBin(trix);
      SimToT=hToT->GetBinContent(binx);
    }
    
    Float_t par[6];    
    for(Int_t kk=0;kk<6;kk++){
      par[kk]=0;
    }
    element->SetToT(trix);
    Int_t nfpar = f->GetNpar();
    for (Int_t kk = 0; kk< nfpar; kk++){
      par[kk]=f->GetParameter(kk);
    }
    Float_t ToT = element->GetToT();
    element->SetTdcND(element->GetTdc());
    Float_t tdc = ((element->GetTdc())*AliTOFGeometry::TdcBinWidth()+32)*1.E-3; //tof signal in ns
    Float_t timeoffset=par[0] + ToT*par[1] +ToT*ToT*par[2] +ToT*ToT*ToT*par[3] +ToT*ToT*ToT*ToT*par[4] +ToT*ToT*ToT*ToT*ToT*par[5]; 
    timeoffset=0;
    Float_t timeSlewed = tdc + timeoffset;
    element->SetTdc((timeSlewed*1E3-32)/AliTOFGeometry::TdcBinWidth());   
  }
  TFile * file = new TFile("ToT.root", "recreate");
  file->cd();
  ChArray.Write("ToT",TObject::kSingleKey);
  file->Close();
  delete file;
  ChArray.Clear();
  return digits;
}
//____________________________________________________________________________

void AliTOFcalib::SelectESD(AliESD *event, AliRunLoader *rl) 
{
  AliLoader *tofl = rl->GetLoader("TOFLoader");
  if (tofl == 0x0) {
    AliError("Can not get the TOF Loader");
    delete rl;
  }
  tofl->LoadRecPoints("read");
  TClonesArray *fClusters =new TClonesArray("AliTOFcluster",  1000);
  TTree *rTree=tofl->TreeR();
  if (rTree == 0) {
    cerr<<"Can't get the Rec Points tree !\n";
    delete rl;
  }
  TBranch *branch=rTree->GetBranch("TOF");
  if (!branch) {
    cerr<<"Cant' get the branch !\n";
    delete rl;
  }
  branch->SetAddress(&fClusters);
  rTree->GetEvent(0);
  Float_t LowerMomBound=0.8; // [GeV/c] default value Pb-Pb
  Float_t UpperMomBound=1.8 ; // [GeV/c] default value Pb-Pb
  Int_t ntrk =0;
  Int_t ngoodtrkfinalToT = 0;
  ntrk=event->GetNumberOfTracks();
  fESDsel = new TObjArray(ntrk);
  fESDsel->SetOwner();
  TObjArray  UCdatatemp(ntrk);
  Int_t ngoodtrk = 0;
  Int_t ngoodtrkfinal = 0;
  Float_t mintime =1E6;
  for (Int_t itrk=0; itrk<ntrk; itrk++) {
    AliESDtrack *t=event->GetTrack(itrk);
    //track selection: reconstrution to TOF:
    if ((t->GetStatus()&AliESDtrack::kTOFout)==0) {
      continue;
    }
    //IsStartedTimeIntegral
    if ((t->GetStatus()&AliESDtrack::kTIME)==0) {
      continue;
    }
    Double_t time=t->GetTOFsignal();	
    time*=1.E-3; // tof given in nanoseconds
    if(time <= mintime)mintime=time;
    Double_t mom=t->GetP();
    if (!(mom<=UpperMomBound && mom>=LowerMomBound))continue;
    UInt_t AssignedTOFcluster=t->GetTOFcluster();//index of the assigned TOF cluster, >0 ?
    if(AssignedTOFcluster==0){ // not matched
      continue;
    }
    AliTOFcluster * tofcl = (AliTOFcluster *)fClusters->UncheckedAt(AssignedTOFcluster);	
    Int_t isector = tofcl->GetDetInd(0);
    Int_t iplate = tofcl->GetDetInd(1);
    Int_t istrip = tofcl->GetDetInd(2);
    Int_t ipadz = tofcl->GetDetInd(3);
    Int_t ipadx = tofcl->GetDetInd(4);
    Float_t ToT = tofcl->GetToT();
    AliTOFcalibESD *unc = new AliTOFcalibESD;
    unc->CopyFromAliESD(t);
    unc->SetSector(isector);
    unc->SetPlate(iplate);
    unc->SetStrip(istrip);
    unc->SetPadz(ipadz);
    unc->SetPadx(ipadx);
    unc->SetToT(ToT);
    Double_t c1[15]; 
    unc->GetExternalCovariance(c1);
    UCdatatemp.Add(unc);
    ngoodtrk++;
  }
  for (Int_t i = 0; i < ngoodtrk ; i ++){
    AliTOFcalibESD *unc = (AliTOFcalibESD*)UCdatatemp.At(i);
    if((unc->GetTOFsignal()-mintime*1.E3)<5.E3){
      fESDsel->Add(unc);
      ngoodtrkfinal++;
      ngoodtrkfinalToT++;
    }
  }
  fESDsel->Sort();
}
//_____________________________________________________________________________

void AliTOFcalib::CombESDId(){
  Float_t t0offset=0;
  Float_t loffset=0;
  Int_t   ntracksinset=6;
  Float_t exptof[6][3];
  Float_t momentum[6]={0.,0.,0.,0.,0.,0.};
  Int_t   assparticle[6]={3,3,3,3,3,3};
  Float_t massarray[3]={0.13957,0.493677,0.9382723};
  Float_t timeofflight[6]={0.,0.,0.,0.,0.,0.};
  Float_t beta[6]={0.,0.,0.,0.,0.,0.};
  Float_t texp[6]={0.,0.,0.,0.,0.,0.};
  Float_t sqMomError[6]={0.,0.,0.,0.,0.,0.};
  Float_t sqTrackError[6]={0.,0.,0.,0.,0.,0.};
  Float_t tracktoflen[6]={0.,0.,0.,0.,0.,0.};
  Float_t TimeResolution   = 0.90e-10; // 90 ps by default	
  Float_t timeresolutioninns=TimeResolution*(1.e+9); // convert in [ns]
  Float_t timezero[6]={0.,0.,0.,0.,0.,0.};
  Float_t weightedtimezero[6]={0.,0.,0.,0.,0.,0.};
  Float_t besttimezero[6]={0.,0.,0.,0.,0.,0.};
  Float_t bestchisquare[6]={0.,0.,0.,0.,0.,0.};
  Float_t bestweightedtimezero[6]={0.,0.,0.,0.,0.,0.};
  Float_t bestsqTrackError[6]={0.,0.,0.,0.,0.,0.};

  Int_t nelements = fESDsel->GetEntries();
  Int_t nset= (Int_t)(nelements/ntracksinset);
  for (Int_t i=0; i< nset; i++) {   

    AliTOFcalibESD **unc=new AliTOFcalibESD*[ntracksinset];
    for (Int_t itrk=0; itrk<ntracksinset; itrk++) {
      Int_t index = itrk+i*ntracksinset;
      AliTOFcalibESD *element=(AliTOFcalibESD*)fESDsel->At(index);
      unc[itrk]=element;
    }
    
    for (Int_t j=0; j<ntracksinset; j++) {
      AliTOFcalibESD *element=unc[j];
      Double_t mom=element->GetP();
      Double_t time=element->GetTOFsignal()*1.E-3; // in ns	
      Double_t exptime[10]; 
      element->GetIntegratedTimes(exptime);
      Double_t toflen=element->GetIntegratedLength()/100.;  // in m
      timeofflight[j]=time+t0offset;
      tracktoflen[j]=toflen+loffset;
      exptof[j][0]=exptime[2]*1.E-3+0.005;
      exptof[j][1]=exptime[3]*1.E-3+0.005;
      exptof[j][2]=exptime[4]*1.E-3+0.005;
      momentum[j]=mom;
    }
    Float_t t0best=999.;
    Float_t Et0best=999.;
    Float_t chisquarebest=999.;
    for (Int_t i1=0; i1<3;i1++) {
      beta[0]=momentum[0]/sqrt(massarray[i1]*massarray[i1]+momentum[0]*momentum[0]);
      texp[0]=exptof[0][i1];
      for (Int_t i2=0; i2<3;i2++) { 
	beta[1]=momentum[1]/sqrt(massarray[i2]*massarray[i2]+momentum[1]*momentum[1]);
	texp[1]=exptof[1][i2];
	for (Int_t i3=0; i3<3;i3++) {
	  beta[2]=momentum[2]/sqrt(massarray[i3]*massarray[i3]+momentum[2]*momentum[2]);
	  texp[2]=exptof[2][i3];
	  for (Int_t i4=0; i4<3;i4++) {
	    beta[3]=momentum[3]/sqrt(massarray[i4]*massarray[i4]+momentum[3]*momentum[3]);
	    texp[3]=exptof[3][i4];
	    
	    for (Int_t i5=0; i5<3;i5++) {
	      beta[4]=momentum[4]/sqrt(massarray[i5]*massarray[i5]+momentum[4]*momentum[4]);
	      texp[4]=exptof[4][i5];
	      for (Int_t i6=0; i6<3;i6++) {
		beta[5]=momentum[5]/sqrt(massarray[i6]*massarray[i6]+momentum[5]*momentum[5]);
		texp[5]=exptof[5][i6];
	
		Float_t sumAllweights=0.;
		Float_t meantzero=0.;
		Float_t Emeantzero=0.;
		
		for (Int_t itz=0; itz<ntracksinset;itz++) {
		  sqMomError[itz]=
		    ((1.-beta[itz]*beta[itz])*0.025)*
		    ((1.-beta[itz]*beta[itz])*0.025)*
		    (tracktoflen[itz]/
		     (0.299792*beta[itz]))*
		    (tracktoflen[itz]/
		     (0.299792*beta[itz])); 
		  sqTrackError[itz]=
		    (timeresolutioninns*
		     timeresolutioninns
		     +sqMomError[itz]); 
		  
		  timezero[itz]=texp[itz]-timeofflight[itz];		    
		  weightedtimezero[itz]=timezero[itz]/sqTrackError[itz];
		  sumAllweights+=1./sqTrackError[itz];
		  meantzero+=weightedtimezero[itz];
		  
		} // end loop for (Int_t itz=0; itz<15;itz++)
		
		meantzero=meantzero/sumAllweights; // it is given in [ns]
		Emeantzero=sqrt(1./sumAllweights); // it is given in [ns]
		
		// calculate chisquare
		
		Float_t chisquare=0.;		
		for (Int_t icsq=0; icsq<ntracksinset;icsq++) {
		  chisquare+=(timezero[icsq]-meantzero)*(timezero[icsq]-meantzero)/sqTrackError[icsq];
		} // end loop for (Int_t icsq=0; icsq<15;icsq++) 
		//		cout << " chisquare " << chisquare << endl;
		
		Int_t npion=0;
		if(i1==0)npion++;
		if(i2==0)npion++;
		if(i3==0)npion++;
		if(i4==0)npion++;
		if(i5==0)npion++;
		if(i6==0)npion++;
		
	     	if(chisquare<=chisquarebest  && ((Float_t) npion/ ((Float_t) ntracksinset)>0.3)){
		  //  if(chisquare<=chisquarebest){
		  
		  for(Int_t iqsq = 0; iqsq<ntracksinset; iqsq++) {
		    bestsqTrackError[iqsq]=sqTrackError[iqsq]; 
		    besttimezero[iqsq]=timezero[iqsq]; 
		    bestweightedtimezero[iqsq]=weightedtimezero[iqsq]; 
		    bestchisquare[iqsq]=(timezero[iqsq]-meantzero)*(timezero[iqsq]-meantzero)/sqTrackError[iqsq]; 
		  }
		  
		  assparticle[0]=i1;
		  assparticle[1]=i2;
		  assparticle[2]=i3;
		  assparticle[3]=i4;
		  assparticle[4]=i5;
		  assparticle[5]=i6;
		  
		  chisquarebest=chisquare;
     		  t0best=meantzero;
		  Et0best=Emeantzero;
		} // close if(dummychisquare<=chisquare)
	      } // end loop on i6
	    } // end loop on i5
	  } // end loop on i4
	} // end loop on i3
      } // end loop on i2
    } // end loop on i1


    Float_t confLevel=999;
    if(chisquarebest<999.){
      Double_t dblechisquare=(Double_t)chisquarebest;
      confLevel=(Float_t)TMath::Prob(dblechisquare,ntracksinset-1); 
    }
    // assume they are all pions for fake sets
    if(confLevel<0.01 || confLevel==999. ){
      for (Int_t itrk=0; itrk<ntracksinset; itrk++)assparticle[itrk]=0;
    }
    for (Int_t itrk=0; itrk<ntracksinset; itrk++) {
      Int_t index = itrk+i*ntracksinset;
      AliTOFcalibESD *element=(AliTOFcalibESD*)fESDsel->At(index);
      element->SetCombID(assparticle[itrk]);
    }
  }
}

//_____________________________________________________________________________

void AliTOFcalib::CalibrateESD(){
  Int_t nelements = fESDsel->GetEntries();
  Int_t *number=new Int_t[fsize];
  fArrayToT = new AliTOFArray(fsize);
  fArrayTime = new AliTOFArray(fsize);
  for (Int_t i=0; i<fsize; i++){
    number[i]=0;
    fArrayToT->AddArray(i, new TArrayF(fgkchannel));
    TArrayF * parrToT = fArrayToT->GetArray(i);
    TArrayF & refaToT = * parrToT;
    fArrayTime->AddArray(i, new TArrayF(fgkchannel));
    TArrayF * parrTime = fArrayToT->GetArray(i);
    TArrayF & refaTime = * parrTime;
    for (Int_t j = 0;j<AliTOFcalib::fgkchannel;j++){
      refaToT[j]=0.;      //ToT[i][j]=j;
      refaTime[j]=0.;      //Time[i][j]=j;
    }
  }
  
  for (Int_t i=0; i< nelements; i++) {
    AliTOFcalibESD *element=(AliTOFcalibESD*)fESDsel->At(i);
    Int_t ipid = element->GetCombID();
    Double_t etime = 0;   //expected time
    Double_t expTime[10]; 
    element->GetIntegratedTimes(expTime);
    if (ipid == 0) etime = expTime[2]*1E-3; //ns
    else if (ipid == 1) etime = expTime[3]*1E-3; //ns
    else if (ipid == 2) etime = expTime[4]*1E-3; //ns
    else AliError("No pid from combinatorial algo for this track");
    Double_t mtime = (Double_t)element->GetTOFsignal()*1E-3;  //measured time
    Double_t mToT = (Double_t) element->GetToT();  //measured ToT, ns
    /*
    Int_t *detId[5];
    detId[0] = element->GetSector();
    detId[1] = element->GetPlate();
    detId[2] = element->GetStrip();
    detId[3] = element->GetPadz();
    detId[4] = element->GetPadx();
    Int_t index = GetIndex(detId);
    */
    //select the correspondent channel with its simulated ToT spectrum
    //summing up everything, index = 0 for all channels:
    Int_t index = 0;
    Int_t index2 = number[index];
    TArrayF * parrToT = fArrayToT->GetArray(index);
    TArrayF & refaToT = * parrToT;
    refaToT[index2] = (Float_t)mToT;
    TArrayF * parrTime = fArrayTime->GetArray(index);
    TArrayF & refaTime = * parrTime;
    refaTime[index2] = (Float_t)(mtime-etime);
    number[index]++;
  }

  for (Int_t i=0;i<1;i++){
    TH1F * hProf = Profile(i);
    TF1* fGold = SetFitFunctions(hProf);
    Int_t nfpar = fGold->GetNpar();
    Float_t par[6];    
    for(Int_t kk=0;kk<6;kk++){
      par[kk]=0;
    }
    for (Int_t kk = 0; kk< nfpar; kk++){
      par[kk]=fGold->GetParameter(kk);
    }
    AliTOFChannel * CalChannel = fTOFCal->GetChannel(i);
    CalChannel->SetSlewPar(par);
  }
  delete[] number;
}

//___________________________________________________________________________

TH1F* AliTOFcalib::Profile(Int_t ich){
  const Int_t nbinToT = 650;
  Int_t nbinTime = 400;
  Float_t minTime = -10.5; //ns
  Float_t maxTime = 10.5; //ns
  Float_t minToT = 7.5; //ns
  Float_t maxToT = 40.; //ns
  Float_t DeltaToT = (maxToT-minToT)/nbinToT;
  Double_t mTime[nbinToT+1],mToT[nbinToT+1],meanTime[nbinToT+1], meanTime2[nbinToT+1],ToT[nbinToT+1], ToT2[nbinToT+1],meanToT[nbinToT+1],meanToT2[nbinToT+1],Time[nbinToT+1],Time2[nbinToT+1],xlow[nbinToT+1],sigmaTime[nbinToT+1];
  Int_t n[nbinToT+1], nentrx[nbinToT+1];
  Double_t sigmaToT[nbinToT+1];
  for (Int_t i = 0; i < nbinToT+1 ; i++){
    mTime[i]=0;
    mToT[i]=0;
    n[i]=0;
    meanTime[i]=0;
    meanTime2[i]=0;
    ToT[i]=0;
    ToT2[i]=0;
    meanToT[i]=0;
    meanToT2[i]=0;
    Time[i]=0;
    Time2[i]=0;
    xlow[i]=0;
    sigmaTime[i]=0;
    sigmaToT[i]=0;
    n[i]=0;
    nentrx[i]=0;
  }
  TH2F* hSlewing = new TH2F("hSlewing", "hSlewing", nbinToT, minToT, maxToT, nbinTime, minTime, maxTime);
  TArrayF * parrToT = fArrayToT->GetArray(ich);
  TArrayF & refaToT = * parrToT;
  TArrayF * parrTime = fArrayTime->GetArray(ich);
  TArrayF & refaTime = * parrTime;
  for (Int_t j = 0; j < AliTOFcalib::fgkchannel; j++){
    if (refaToT[j] == 0) continue; 
    Int_t nx = (Int_t)((refaToT[j]-minToT)/DeltaToT)+1;
    if ((refaToT[j] != 0) && (refaTime[j] != 0)){
      Time[nx]+=refaTime[j];
      Time2[nx]+=(refaTime[j])*(refaTime[j]);
      ToT[nx]+=refaToT[j];
      ToT2[nx]+=refaToT[j]*refaToT[j];
      nentrx[nx]++;
      hSlewing->Fill(refaToT[j],refaTime[j]);
    }
  }
  Int_t nbinsToT=hSlewing->GetNbinsX();
  if (nbinsToT != nbinToT) {
    AliError("Profile :: incompatible numbers of bins");
    return 0x0;
  }

  Int_t usefulBins=0;
  TH1F *histo = new TH1F("histo", "1D Time vs ToT", nbinsToT, minToT, maxToT);
  for (Int_t i=1;i<=nbinsToT;i++){
    if (nentrx[i]!=0){
    n[usefulBins]+=nentrx[i];
    if (n[usefulBins]==0 && i == nbinsToT) {
      break;
    }
    meanTime[usefulBins]+=Time[i];
    meanTime2[usefulBins]+=Time2[i];
    meanToT[usefulBins]+=ToT[i];
    meanToT2[usefulBins]+=ToT2[i];
    if (n[usefulBins]<20 && i!=nbinsToT) continue; 
    mTime[usefulBins]=meanTime[usefulBins]/n[usefulBins];
    mToT[usefulBins]=meanToT[usefulBins]/n[usefulBins];
    sigmaTime[usefulBins]=TMath::Sqrt(1./n[usefulBins]/n[usefulBins]
				   *(meanTime2[usefulBins]-meanTime[usefulBins]
				     *meanTime[usefulBins]/n[usefulBins]));
    if ((1./n[usefulBins]/n[usefulBins]
	 *(meanToT2[usefulBins]-meanToT[usefulBins]
	   *meanToT[usefulBins]/n[usefulBins]))< 0) {
      AliError(" too small radical" );
      sigmaToT[usefulBins]=0;
    }
    else{       
      sigmaToT[usefulBins]=TMath::Sqrt(1./n[usefulBins]/n[usefulBins]
				     *(meanToT2[usefulBins]-meanToT[usefulBins]
				       *meanToT[usefulBins]/n[usefulBins]));
    }
    usefulBins++;
    }
  }
  for (Int_t i=0;i<usefulBins;i++){
    Int_t binN = (Int_t)((mToT[i]-minToT)/DeltaToT)+1;
    histo->Fill(mToT[i],mTime[i]);
    histo->SetBinError(binN,sigmaTime[i]);
  } 
  return histo;
}
//_____________________________________________________________________________

void AliTOFcalib::CorrectESDTime(){
  Int_t nelements = fESDsel->GetEntries();
  for (Int_t i=0; i< nelements; i++) {
    AliTOFcalibESD *element=(AliTOFcalibESD*)fESDsel->At(i);
    /*
    Int_t *detId[5];
    detId[0] = element->GetSector();
    detId[1] = element->GetPlate();
    detId[2] = element->GetStrip();
    detId[3] = element->GetPadz();
    detId[4] = element->GetPadx();
    Int_t index = GetIndex(detId);
    */
    //select the correspondent channel with its simulated ToT spectrum
    //summing up everything, index = 0 for all channels:
    Int_t index = 0;
    Int_t ipid = element->GetCombID();
    Double_t etime = 0;   //expected time
    Double_t expTime[10]; 
    element->GetIntegratedTimes(expTime);
    if (ipid == 0) etime = expTime[2]*1E-3; //ns
    else if (ipid == 1) etime = expTime[3]*1E-3; //ns
    else if (ipid == 2) etime = expTime[4]*1E-3; //ns
    Float_t par[6];
    AliTOFChannel * CalChannel = fTOFCal->GetChannel(index);
    for (Int_t j = 0; j<6; j++){
      par[j]=CalChannel->GetSlewPar(j);
    }
    //Float_t TimeCorr=par[0]+par[1]*ToT+par[2]*ToT*ToT+par[3]*ToT*ToT*ToT+par[4]*ToT*ToT*ToT*ToT+par[5]*ToT*ToT*ToT*ToT*ToT;
  }
}
//_____________________________________________________________________________

void AliTOFcalib::CorrectESDTime(AliESD *event, AliRunLoader *rl ){
  AliLoader *tofl = rl->GetLoader("TOFLoader");
  if (tofl == 0x0) {
    AliError("Can not get the TOF Loader");
    delete rl;
  }
  tofl->LoadRecPoints("read");
  TClonesArray *fClusters =new TClonesArray("AliTOFcluster",  1000);
  TTree *rTree=tofl->TreeR();
  if (rTree == 0) {
    cerr<<"Can't get the Rec Points tree !\n";
    delete rl;
  }
  TBranch *branch=rTree->GetBranch("TOF");
  if (!branch) {
    cerr<<"Cant' get the branch !\n";
    delete rl;
  }
  branch->SetAddress(&fClusters);
  rTree->GetEvent(0);
  Int_t ntrk =0;
  ntrk=event->GetNumberOfTracks();
  for (Int_t itrk=0; itrk<ntrk; itrk++) {
    AliESDtrack *t=event->GetTrack(itrk);
    if ((t->GetStatus()&AliESDtrack::kTOFout)==0) {
      continue;
    }
    //IsStartedTimeIntegral
    if ((t->GetStatus()&AliESDtrack::kTIME)==0) {
      continue;
    }
    UInt_t AssignedTOFcluster=t->GetTOFcluster();//index of the assigned TOF cluster, >0 ?
    if(AssignedTOFcluster==0){ // not matched
      continue;
    }
    AliTOFcluster * tofcl = (AliTOFcluster *)fClusters->UncheckedAt(AssignedTOFcluster);	
    /*
    Int_t *detId[5];
    detId[0] = tofcl->GetSector();
    detId[1] = tofcl->GetPlate();
    detId[2] = tofcl->GetStrip();
    detId[3] = tofcl->GetPadz();
    detId[4] = tofcl->GetPadx();
    Int_t index = GetIndex(detId);
    */
    //select the correspondent channel with its simulated ToT spectrum
    //summing up everything, index = 0 for all channels:
    Int_t index = 0;
    AliTOFChannel * CalChannel = fTOFCal->GetChannel(index);
    Float_t par[6];
    for (Int_t j = 0; j<6; j++){
      par[j]=CalChannel->GetSlewPar(j);
    }
    Float_t ToT = tofcl->GetToT();
    Float_t TimeCorr =0; 
    TimeCorr=par[0]+par[1]*ToT+par[2]*ToT*ToT+par[3]*ToT*ToT*ToT+par[4]*ToT*ToT*ToT*ToT+par[5]*ToT*ToT*ToT*ToT*ToT;
  }
}
//_____________________________________________________________________________

void AliTOFcalib::WriteOnCDB(){
  AliCDBManager *man = AliCDBManager::Instance();
  AliCDBId id("TOF/Calib/Constants",1,100);
  AliCDBMetaData *md = new AliCDBMetaData();
  md->SetResponsible("Shimmize");
  man->Put(fTOFCal,id,md);
}
//_____________________________________________________________________________

void AliTOFcalib::ReadFromCDB(Char_t *sel, Int_t nrun){
  AliCDBManager *man = AliCDBManager::Instance();
  AliCDBEntry *entry = man->Get(sel,nrun);
  if (!entry){
    AliError("Retrivial failed");
    AliCDBStorage *origSto =man->GetDefaultStorage();
    man->SetDefaultStorage("local://$ALICE_ROOT");
    entry = man->Get("TOF/Calib/Constants",nrun);
    man->SetDefaultStorage(origSto);
  }
  AliTOFCal *cal =(AliTOFCal*)entry->GetObject();
  fTOFCal = cal;
}
//_____________________________________________________________________________

Int_t AliTOFcalib::GetIndex(Int_t *detId){
  Int_t isector = detId[0];
  if (isector >= fNSector)
    AliError(Form("Wrong sector number in TOF (%d) !",isector));
  Int_t iplate = detId[1];
  if (iplate >= fNPlate)
    AliError(Form("Wrong plate number in TOF (%d) !",iplate));
  Int_t istrip = detId[2];
  Int_t ipadz = detId[3];
  Int_t ipadx = detId[4];
  Int_t stripOffset = 0;
  switch (iplate) {
  case 0:
    stripOffset = 0;
    break;
  case 1:
    stripOffset = fNStripC;
    break;
  case 2:
    stripOffset = fNStripC+fNStripB;
    break;
  case 3:
    stripOffset = fNStripC+fNStripB+fNStripA;
    break;
  case 4:
    stripOffset = fNStripC+fNStripB+fNStripA+fNStripB;
    break;
  default:
    AliError(Form("Wrong plate number in TOF (%d) !",iplate));
    break;
  };

  Int_t idet = ((2*(fNStripC+fNStripB)+fNStripA)*fNpadZ*fNpadX)*isector +
               (stripOffset*fNpadZ*fNpadX)+
               (fNpadZ*fNpadX)*istrip+
	       (fNpadX)*ipadz+
	        ipadx;
  return idet;
}

