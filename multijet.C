// File: multijet.C
// Created by Mikko Voutilainen, on June 9th, 2014
// Updated on Oct 25, 2014 (cleanup for 8 TeV JEC paper)
// Purpose: Prepare results from multijet analysis in a form suitable
//          for later global fit analysis
#include "TFile.h"
#include "TH1D.h"
#include "TGraphErrors.h"
#include "TRandom.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TLine.h"
#include "TMatrixD.h"
#include "TMinuit.h"
#include "TLatex.h"
#include "TFitter.h"
#include "TMatrixD.h"
#include "TVectorD.h"
#include "TMath.h"

#include "tdrstyle_mod14.C"
#include "tools.h"

#include <iostream>
#include <string>

using namespace std;

// Remove empty bins
void cleanGraph(TGraphErrors *g) {
  for (int i = g->GetN(); i != -1; --i)
    if (g->GetY()[i]==0) g->RemovePoint(i);
}

TF1 *_fitError_func(0);
TMatrixD *_fitError_emat(0);
Double_t fitError(Double_t *xx, Double_t *p);

const int np = 3; // Need 2 pars for now for globalFitL3Res.C to work
//const int np = 2;
TF1 *fhb(0), *fl1(0);
Double_t multiFit(Double_t *x, Double_t *p) {

  // Use log3 to match FSR uncertainty band shapes in global fit
  //return (p[0] + p[1]*log(0.01*(*x)) + p[2]*pow(log(0.01*(*x)),2)); // log3

  // same 2-parameter fit as for L3Res
  // Initialize SinglePionHCAL and PileUpPt shapes
  if (!fhb) fhb = new TF1("fhb","max(0,[0]+[1]*pow(x,[2]))",10,3500);
  fhb->SetParameters(1.03091e+00, -5.11540e-02, -1.54227e-01); // SPRH
  if (!fl1) fl1 = new TF1("fl1","1+([0]+[1]*log(x))/x",30,2000);
  fl1->SetParameters(-2.36997, 0.413917);

  double pt = x[0];
  double ptref = 208; // pT that minimizes correlation in p[0] and p[1]
  return (p[0] + p[1]/3.*100*(fhb->Eval(pt)-fhb->Eval(ptref))
	  + (0*p[2])-0.25 * (fl1->Eval(pt)-fl1->Eval(ptref)));
	  //+ -0.090 * (fl1->Eval(pt)-fl1->Eval(ptref)));
} // multiFit

int cnt(0), Nk(0);
TF1 *_multiFit(0);
TH1D *_herr(0);
vector<TGraphErrors*> *_vdt(0), *_vpt(0), *_vdt2(0), *_vpt2(0);
vector<TH1D*> *_vsrc(0);
void multiFitter(Int_t &npar, Double_t *grad, Double_t &chi2, Double_t *par,
		 Int_t flag);

void multijet(bool usemjb = true) {

  TDirectory *curdir = gDirectory;
  setTDRStyle();

  double ptbins[] = {200, 250, 300, 360, 400, 450, 500, 550, 600,
		     700, 800, 1000, 1200, 1500};
  const int npt = sizeof(ptbins)/sizeof(ptbins[0])-1;
  
  // On 21 Jun 2014, at 18:47, from Anne-Laure Pequegnot
  // Re: Need graphs
  //TFile *fmd = new TFile("rootfiles/MULTIJET_Run2012ABCD-22Jan2013_analysis_woPU_pt30_eta50_puJetIdT_HLTsel_woPtRecoilCut_recoilPtHLTBin_type1fix_beforePrescaleReweighting.root","READ"); // data, GT
  // On 18 Dec 2014, at 22:48, from Anne-Laure Pequegnot 
  // Re: Multijet data with the corrected L1Residuals
  //TFile *fmd = new TFile("rootfiles/MULTIJET_Run2012ABCD-22Jan2013_analysis_woPU_pt30_eta50_puJetIdT_recoilPtHLTBin_type1fix_afterPrescaleReweighting.root","READ"); // data, newL1
  // On 12 Jan 2015, at 11:21, from Anne-Laure Pequegnot
  // Re: Updated L2Res for new L1 - please rerun
  TFile *fmd = new TFile("rootfiles/MULTIJET_Run2012ABCD-22Jan2013_analysis_woPU_pt30_eta50_puJetIdT_recoilPtHLTBin_type1fix_afterPrescaleReweighting-v6.root","READ"); // data, newL1V6

  assert(fmd && !fmd->IsZombie());
  assert(fmd->cd("MJB"));
  assert(gDirectory->cd("PtBin"));
  TGraphErrors *gmd1 = (TGraphErrors*)gDirectory->Get("gMJB_RefObjPt");
  assert(gmd1); gmd1->SetName("MJB");
  //
  assert(fmd->cd("MPF"));
  assert(gDirectory->cd("PtBin"));
  TGraphErrors *gmd2 = (TGraphErrors*)gDirectory->Get("gMPF_RefObjPt");
  assert(gmd2); gmd2->SetName("MPF");

  // On 21 Jun 2014, at 18:47, from Anne-Laure Pequegnot
  // Re: Need graphs
  //TFile *fmc = new TFile("rootfiles/MULTIJET_MC_QCD-HT-100ToInf_analysis_woPU_pt30_eta50_puJetIdT_HLTsel_woPtRecoilCut_recoilPtHLTBin_type1fix.root","READ"); // MC
  // On 12 Jan 2015, at 11:21, from Anne-Laure Pequegnot
  // Re: Updated L2Res for new L1 - please rerun
  TFile *fmc = new TFile("rootfiles/MULTIJET_MC_QCD-HT-100ToInf_analysis_woPU_pt30_eta50_puJetIdT_recoilPtHLTBin_type1fix-v6.root","READ"); // MC, newL1V6

  assert(fmc && !fmc->IsZombie());
  assert(fmc->cd("MJB"));
  assert(gDirectory->cd("PtBin"));
  TGraphErrors *gmc1 = (TGraphErrors*)gDirectory->Get("gMJB_RefObjPt");
  assert(gmc1); gmc1->SetName("MJB");

  assert(fmc->cd("MPF"));
  assert(gDirectory->cd("PtBin"));
  TGraphErrors *gmc2 = (TGraphErrors*)gDirectory->Get("gMPF_RefObjPt");
  assert(gmc2); gmc2->SetName("MPF");

  // Remove "empty" points (from TH1D to TGraph conversion?)
  cleanGraph(gmd1); cleanGraph(gmc1);
  cleanGraph(gmd2); cleanGraph(gmc2);
  TGraphErrors *gm1 = tools::ratioGraphs(gmd1, gmc1);
  TGraphErrors *gm2 = tools::ratioGraphs(gmd2, gmc2);
  cleanGraph(gm1); cleanGraph(gm2);

  // Load Anne-Laure's MJB systematics
  TFile *fs = new TFile("rootfiles/compareMC_signedValues_recoilPtHLTBin.root",
			"READ");
  assert(fs && !fs->IsZombie());

  vector<TH1D*> vsrc;
  vector<string> src;
  src.push_back("JEC");
  src.push_back("JER");
  src.push_back("PU");

  const int nmethods = 2;
  for (int im = 0; im != nmethods; ++im) {

    const char *cm = (im==0 ? "MJB" : "MPF");
    for (unsigned int isrc = 0; isrc != src.size(); ++isrc) {
   
      assert(fs->cd(cm));
      assert(gDirectory->cd("PtBin"));
      const char *cs = src[isrc].c_str();
      TGraphErrors *gjec = (TGraphErrors*)gDirectory->Get(Form("g%s_%s",cm,cs));
      assert(gjec);
      string s = Form("%s_multijet_src%d", im==0 ? "ptchs" : "mpfchs1", isrc);
      TH1D *hs = new TH1D(Form("bm%d_%s",1<<im, s.c_str()),
			  Form("%s;p_{T}^{recoil};%s unc.",s.c_str(),cs),
			  npt, ptbins);

      for (int i = 0; i != gjec->GetN(); ++i) {

	double pt = gjec->GetX()[i];
	int ipt = hs->FindBin(pt);
	hs->SetBinContent(ipt, gjec->GetY()[i]);
	hs->SetBinError(ipt, gjec->GetY()[i]);
      } // for i

      vsrc.push_back(hs);
    } // for isrc
  } // for im
  
  // Get cExp for recoil (available for data and MC, but can only use one)
  // This one is pT>30 GeV for MJB
  // (When was this file obtained? In which e-mail?)
  // (Should I use instead the 08Jul14 pt30 file from mail below?)
  //TFile *fe = new TFile("rootfiles/cExp_sum_Fi_log_fi_RecoilPt_mc_woPUJets_pt30_eta50_puJetIdT_HLTsel_woPtRecoilCut_recoilPtHLTBin_type1fix_QCD-HT.root","READ"); // MC
  TFile *fe = new TFile("rootfiles/MULTIJET_MC_QCD-HT-100ToInf_analysis_woPU_pt30_eta50_puJetIdT_recoilPtHLTBin_type1fix.root","READ"); // newL1V6
  //TFile *fe = new TFile("rootfiles/cExp_sum_Fi_log_fi_RecoilPt_RefObjPt_resize_woPUJets_pt30_eta50_puJetIdT_HLTsel_woPtRecoilCut_recoilPtHLTBin_type1fix_afterPrescaleReweighting_withSystErrors_QCD-HT-v6.root","READ"); // MC, newL1V6
  assert(fe && !fe->IsZombie());

  // On 08 Jul 2014, at 17:02, from Anne-Laure Pequegnot
  // Re: Need graphs
  // This one is pT>10 GeV for MPF
  //TFile *fe2 = new TFile("rootfiles/cExp_sum_Fi_log_fi_RecoilPt_mc_woPUJets_pt10_eta50_puJetIdT_recoilPtHLTBin_type1fix_QCD-HT.root","READ");
  TFile *fe2 = new TFile("rootfiles/MULTIJET_MC_QCD-HT-100ToInf_analysis_woPU_pt10_eta50_puJetIdT_recoilPtHLTBin_type1fix-v6.root","READ"); // newL1V6
  //TFile *fe2 = new TFile("rootfiles/cExp_sum_Fi_log_fi_RecoilPt_RefObjPt_resize_woPUJets_pt10_eta50_puJetIdT_HLTsel_woPtRecoilCut_recoilPtHLTBin_type1fix_afterPrescaleReweighting_withSystErrors_QCD-HT-v6.root","READ"); // newL1V6
  assert(fe2 && !fe2->IsZombie());

  TFile *fjes = new TFile("rootfiles/jecdata.root","READ");
  assert(fjes && !fjes->IsZombie());

  TH1D *herr = (TH1D*)fjes->Get("ratio/eta00-13/herr"); assert(herr);
  TH1D *herr_ref = (TH1D*)fjes->Get("ratio/eta00-13/herr_ref");
  assert(herr_ref);
  _herr = herr_ref;

  TGraphErrors *gm = (usemjb ? gm1 : gm2);
  string s1 = (usemjb ? "mjb" : "mpf");
  const char *cm = s1.c_str();
  string s2 = (usemjb ? "ptchs" : "mpfchs1");
  const char *cm2 = s2.c_str();

  // cExp stored in a TCanvas so need to do some digging to get it out
  //TCanvas *c1e = (TCanvas*)fe->Get("cExp_sum_Fi_log_fi_RecoilPt");
  //assert(c1e);
  //TGraphErrors *ge1 = (TGraphErrors*)c1e->FindObject("Exp_sum_Fi_log_fi");
  // newL1V6:
  TGraphErrors *ge1 = (TGraphErrors*)fe->Get("recoilJets/gExp_sum_Fi_log_fi_RecoilPt");
  assert(ge1);

  //TCanvas *c2e = (TCanvas*)fe2->Get("cExp_sum_Fi_log_fi_RecoilPt");
  //assert(c1e);
  //TGraphErrors *ge2 = (TGraphErrors*)c2e->FindObject("Exp_sum_Fi_log_fi");
  // newL1V6:
  TGraphErrors *ge2 = (TGraphErrors*)fe2->Get("recoilJets/gExp_sum_Fi_log_fi_RecoilPt");
  assert(ge2);

  TGraphErrors *ge = (usemjb ? ge1 : ge2);

  curdir->cd();
  // Make graphs "stick" by cloning them
  gm = (TGraphErrors*)gm->Clone();
  gm1 = (TGraphErrors*)gm1->Clone();
  gm2 = (TGraphErrors*)gm2->Clone();
  ge = (TGraphErrors*)ge->Clone();
  ge1 = (TGraphErrors*)ge1->Clone();
  ge2 = (TGraphErrors*)ge2->Clone();
  //delete c1e;
  //delete c2e;
  fe->Close();
  fe2->Close();

  //const double ptx = 325; // something wrong with MC below this (v2)
  const double ptx(250), pty(1350); // data 230-1350, but <300 doesn't work well
  for (int i = ge->GetN()-1; i != -1; --i) {
    if (ge->GetY()[i]==0 || ge->GetX()[i]<ptx || ge->GetX()[i]>pty)
      ge->RemovePoint(i);
  }
  for (int i = ge1->GetN()-1; i != -1; --i) {
    if (ge1->GetY()[i]==0 || ge1->GetX()[i]<ptx || ge1->GetX()[i]>pty)
      ge1->RemovePoint(i);
  }
  for (int i = ge2->GetN()-1; i != -1; --i) {
    if (ge2->GetY()[i]==0 || ge2->GetX()[i]<ptx || ge2->GetX()[i]>pty)
      ge2->RemovePoint(i);
  }
  for (int i = gm->GetN()-1; i != -1; --i) {
    if (gm->GetY()[i]==0 || gm->GetX()[i]<ptx) gm->RemovePoint(i);
  }
  for (int i = gm1->GetN()-1; i != -1; --i) {
    if (gm1->GetY()[i]==0 || gm1->GetX()[i]<ptx) gm1->RemovePoint(i);
  }
  for (int i = gm2->GetN()-1; i != -1; --i) {
    if (gm2->GetY()[i]==0 || gm2->GetX()[i]<ptx) gm2->RemovePoint(i);
  }

  cout << "Ngm1 = " << gm1->GetN() << " Ngm2 = " << gm2->GetN()
       << " Nge1 = " << ge1->GetN() << " Nge2 = " << ge2->GetN()
       << endl << flush;

  cout << "xgm1[0]=" << gm1->GetX()[0] << " xgm2[0]=" << gm2->GetX()[0] 
       << " xge[0]=" << ge->GetX()[0] << endl << flush;
  cout << "xgm1[n-1]=" << gm1->GetX()[gm1->GetN()-1]
       << " xgm2[n-1]=" << gm2->GetX()[gm2->GetN()-1]
       << " xge[n-1]=" << ge->GetX()[ge->GetN()-1]
       << endl << flush;
  assert(gm1->GetN()==ge1->GetN());
  assert(gm2->GetN()==ge2->GetN());

  // Note recoilJets/gNjetsRecoil_RecoilPt => some jets missed at pT<600 GeV
  // due to pT>30 GeV threshold

  // Systematic differences between MPF and pT balance result from
  // MPF effectively including jets below 30 GeV. Estimate a systematic
  // from pT threshold using pT>10, 20, 30 GeV samples
  // Binning has changed so fit with power law function
  if (false) { // study pT threshold systematics
  
    // On 19 Jun 2014, at 08:59, from Anne-Laure Pequegnot
    // Re: Need graphs
    TFile *fd10 = new TFile("files/MULTIJET_Run2012ABCD-22Jan2013_analysis_woPU_pt10_eta50_puJetIdT_HLTsel_woPtRecoilCut_recoilPtHLTBin_beforePrescaleReweighting.root","READ");
    TFile *fd20 = new TFile("files/MULTIJET_Run2012ABCD-22Jan2013_analysis_woPU_pt20_eta50_puJetIdT_HLTsel_woPtRecoilCut_recoilPtHLTBin_beforePrescaleReweighting.root","READ");
    TFile *fd30 = new TFile("files/MULTIJET_Run2012ABCD-22Jan2013_analysis_woPU_pt30_eta50_puJetIdT_HLTsel_woPtRecoilCut_recoilPtHLTBin_beforePrescaleReweighting.root","READ");
    TFile *fm10 = new TFile("files/MULTIJET_MC_QCD-HT-100ToInf_analysis_woPU_pt10_eta50_puJetIdT_HLTsel_woPtRecoilCut_recoilPtHLTBin.root","READ");
    TFile *fm20 = new TFile("files/MULTIJET_MC_QCD-HT-100ToInf_analysis_woPU_pt20_eta50_puJetIdT_HLTsel_woPtRecoilCut_recoilPtHLTBin.root","READ");
    TFile *fm30 = new TFile("files/MULTIJET_MC_QCD-HT-100ToInf_analysis_woPU_pt30_eta50_puJetIdT_HLTsel_woPtRecoilCut_recoilPtHLTBin.root","READ");
    
    TGraphErrors *gd10 = (TGraphErrors*)fd10->Get("MJB/PtBin/gMJB_RefObjPt");
    assert(gd10);
    TGraphErrors *gd20 = (TGraphErrors*)fd20->Get("MJB/PtBin/gMJB_RefObjPt");
    assert(gd20);
    TGraphErrors *gd30 = (TGraphErrors*)fd30->Get("MJB/PtBin/gMJB_RefObjPt");
    assert(gd30);

    TGraphErrors *gdm10 = (TGraphErrors*)fd10->Get("MPF/PtBin/gMPF_RefObjPt");
    assert(gdm10);
    TGraphErrors *gdm20 = (TGraphErrors*)fd20->Get("MPF/PtBin/gMPF_RefObjPt");
    assert(gdm20);
    TGraphErrors *gdm30 = (TGraphErrors*)fd30->Get("MPF/PtBin/gMPF_RefObjPt");
    assert(gdm30);

    TGraphErrors *gm10 = (TGraphErrors*)fm10->Get("MJB/PtBin/gMJB_RefObjPt");
    assert(gm10);
    TGraphErrors *gm20 = (TGraphErrors*)fm20->Get("MJB/PtBin/gMJB_RefObjPt");
    assert(gm20);
    TGraphErrors *gm30 = (TGraphErrors*)fm30->Get("MJB/PtBin/gMJB_RefObjPt");
    assert(gm30);

    TGraphErrors *gmm10 = (TGraphErrors*)fm10->Get("MPF/PtBin/gMPF_RefObjPt");
    assert(gmm10);
    TGraphErrors *gmm20 = (TGraphErrors*)fm20->Get("MPF/PtBin/gMPF_RefObjPt");
    assert(gmm20);
    TGraphErrors *gmm30 = (TGraphErrors*)fm30->Get("MPF/PtBin/gMPF_RefObjPt");
    assert(gmm30);

    cleanGraph(gd10);    cleanGraph(gd20);    cleanGraph(gd30);
    cleanGraph(gdm10);   cleanGraph(gdm20);   cleanGraph(gdm30);
    cleanGraph(gm10);    cleanGraph(gm20);    cleanGraph(gm30);
    cleanGraph(gmm10);   cleanGraph(gmm20);   cleanGraph(gmm30);

    TGraphErrors *g10 = tools::ratioGraphs(gd10, gm10);
    TGraphErrors *g20 = tools::ratioGraphs(gd20, gm20);
    TGraphErrors *g30 = tools::ratioGraphs(gd30, gm30);

    TGraphErrors *gr10 = tools::ratioGraphs(gdm10, gmm10);
    TGraphErrors *gr20 = tools::ratioGraphs(gdm20, gmm20);
    TGraphErrors *gr30 = tools::ratioGraphs(gdm30, gmm30);

    TCanvas *c0 = new TCanvas("c0","c0",600,600);
    gPad->SetLogx();

    TH1D *h0 = new TH1D("h0",";p_{T} (GeV);MJB",1600,200,1800);
    h0->SetMaximum(1.05);
    h0->SetMinimum(0.90);
    h0->GetXaxis()->SetMoreLogLabels();
    h0->GetXaxis()->SetNoExponent();
    h0->Draw("AXIS");

    tdrDraw(gd10,"P",kFullCircle,kRed);
    tdrDraw(gd20,"P",kFullCircle,kGreen+2);
    tdrDraw(gd30,"P",kFullCircle,kBlue);
    //
    tdrDraw(gm10,"P",kOpenCircle,kRed);
    tdrDraw(gm20,"P",kOpenCircle,kGreen+2);
    tdrDraw(gm30,"P",kOpenCircle,kBlue);

    tdrDraw(gdm10,"P",kFullSquare,kRed);
    tdrDraw(gdm20,"P",kFullSquare,kGreen+2);
    tdrDraw(gdm30,"P",kFullSquare,kBlue);
    //
    tdrDraw(gmm10,"P",kOpenSquare,kRed);
    tdrDraw(gmm20,"P",kOpenSquare,kGreen+2);
    tdrDraw(gmm30,"P",kOpenSquare,kBlue);

    TF1 *f0 = new TF1("f0","[0]+[1]*pow(x,[2])",200,1200);
    f0->SetParameters(1,-0.5,-0.5);

    f0->SetLineStyle(kDashed);
    gd10->Fit(f0,"QRN");
    f0->SetLineColor(gd10->GetLineColor()); f0->DrawClone("SAME");
    gd20->Fit(f0,"QRN");
    f0->SetLineColor(gd20->GetLineColor()); f0->DrawClone("SAME");
    gd30->Fit(f0,"QRN");
    f0->SetLineColor(gd30->GetLineColor()); f0->DrawClone("SAME");
    //
    f0->SetRange(350,1200);
    gm10->Fit(f0,"QRN");
    f0->SetLineColor(gm10->GetLineColor()); f0->DrawClone("SAME");
    gm20->Fit(f0,"QRN");
    f0->SetLineColor(gm20->GetLineColor()); f0->DrawClone("SAME");
    gm30->Fit(f0,"QRN");
    f0->SetLineColor(gm30->GetLineColor()); f0->DrawClone("SAME");

    TCanvas *c0b = new TCanvas("c0b","c0b",600,600);
    gPad->SetLogx();
    h0->DrawClone("AXIS");

    tdrDraw(g10,"P",kFullSquare,kRed);
    tdrDraw(g20,"P",kFullSquare,kGreen+2);
    tdrDraw(g30,"P",kFullSquare,kBlue);

    tdrDraw(gr10,"P",kOpenSquare,kRed);
    tdrDraw(gr20,"P",kOpenSquare,kGreen+2);
    tdrDraw(gr30,"P",kOpenSquare,kBlue);

    f0->SetLineStyle(kSolid);
    f0->SetRange(350,1200);
    g10->Fit(f0,"QRN");
    f0->SetLineColor(g10->GetLineColor()); f0->DrawClone("SAME");
    g20->Fit(f0,"QRN");
    f0->SetLineColor(g20->GetLineColor()); f0->DrawClone("SAME");
    g30->Fit(f0,"QRN");
    f0->SetLineColor(g30->GetLineColor()); f0->DrawClone("SAME");
  } // pT threshold systematics


  if (true) { // cExp plots

    TH1D *h = new TH1D("h2",";p_{T}^{recoil} (GeV);MJB",1570,30,1600);
    h->SetMinimum(0);
    h->SetMaximum(1.5);
    h->GetXaxis()->SetMoreLogLabels();
    h->GetXaxis()->SetNoExponent();
    
    TCanvas *c2 = new TCanvas("c2","c2",600,600);
    
    c2->cd(); gPad->SetLogx();
    h->SetYTitle("Geometric mean p_{T} fraction");
    h->SetMaximum(0.50);
    h->SetMinimum(0.35);
    h->DrawClone("AXIS");
    //ge->DrawClone("SAMEP");
    ge2->SetMarkerStyle(kFullCircle);
    ge2->Draw("SAMEP");
    ge1->SetMarkerStyle(kOpenCircle);
    ge1->Draw("SAMEP");
    
    c2->SaveAs("pdf/multijet_cExp.pdf");
  }


  if (true) { // ATLAS-style iterative extrapolation of JES

    TCanvas *c1 = new TCanvas("c1","c1",600,600);

    TH1D *h = new TH1D("h",";p_{T}^{recoil} (GeV);MJB",1570,30,1600);
    h->SetMinimum(0);
    h->SetMaximum(1.5);
    h->GetXaxis()->SetMoreLogLabels();
    h->GetXaxis()->SetNoExponent();
    
    c1->cd(); gPad->SetLogx();
    h->SetYTitle("Response (Data/MC)");
    h->SetMaximum(1.08);
    h->SetMinimum(0.93);
    h->DrawClone("AXIS");
    
    herr_ref->SetLineWidth(2);
    herr_ref->SetLineColor(kYellow+3);
    herr_ref->SetLineStyle(kDotted);
    herr_ref->DrawClone("SAME E3");
    (new TGraph(herr))->DrawClone("SAMEL");

    TGraphErrors *gfit = new TGraphErrors(0);
    TGraphErrors *g = new TGraphErrors(0);
    g->SetMarkerStyle(kOpenCircle);
    g->SetMarkerColor(kRed);
    g->SetLineColor(kRed);
    TGraphErrors *gi = new TGraphErrors(0);
    gi->SetMarkerStyle(kOpenCircle);
    gi->SetMarkerColor(kRed);
    gi->SetLineColor(kRed);
    TGraphErrors *gn = new TGraphErrors(0);
    gn->SetMarkerStyle(kFullCircle);
    gn->SetMarkerColor(kBlue);
    gn->SetLineColor(kBlue);
    TGraphErrors *g0 = new TGraphErrors(0);
    g0->SetMarkerStyle(kFullCircle);
    g0->SetMarkerColor(kRed);
    g0->SetLineColor(kRed);
    
    TF1 *f1 = new TF1("f1","[0]+[1]*log(x/[3])+[2]*pow(log(x/[3]),2)",80,1600);
    f1->SetParameters(1,0);
    TLine *l = new TLine();
    l->SetLineColor(kBlue-10);
    
    const int i0 = 0;
    const double jes0 = 0.982; // JES at 100 GeV
    //const double jes0 = 1; // JES at ~200 GeV
    cout << "Starting point pTrecoil=" << gm->GetX()[i0] << endl;
    cout << "This maps to pT'=" << gm->GetX()[i0]*ge->GetY()[i0] << endl;
    for (int i = i0; i != gm->GetN(); ++i) {
      if (gm->GetY()[i]>0) {
	
	double ptrecoil = gm->GetX()[i];
	double mjb = gm->GetY()[i];
	double mjbe = gm->GetEY()[i];
	double geomf = ge->GetY()[i];
	double ptref = ptrecoil * geomf;
	
	// Find the reference JES
	double jesref(0), jesrefe(0);
	if (g->GetN()==0) {
	  // No points yet - set starting point at JES0
	  jesref = jes0;
	  jesrefe = 0.004;//0.002;//mjbe;
	  g->SetPoint(0, ptref, jesref);
	  g->SetPointError(0, 0, jesrefe);
	  g0->SetPoint(0, ptref, jesref);
	  g0->SetPointError(0, 0, jesrefe);
	  
	  gfit->SetPoint(0, ptref, jesref);
	  gfit->SetPointError(0, 0, jesrefe);
	  
	  l->DrawLine(ptref, jesref, ptrecoil, jesref * mjb);
	}
	else {
	  // Interpolate JES at ptref from existing points
	  f1->SetParameters(1, 0, 0, ptref);
	  f1->FixParameter(3, ptref);
	  // Ideally should use quadratic model for interpolating JES,
	  // but this model becomes very inaccurate from the third point
	  // onwards until enough points of inferred JES available
	  // Linear model is more stable in the region between starting JES
	  // and the first points of inferred JES
	  // One solution could be to iterate, starting with a linear model
	  // then using a quadratic model with points from the previous iteration
	  // However, optimally would directly fit the quadratic model
	  // to MJB data
	  f1->FixParameter(2,0);
	  // Have to fit gfit, not g, because otherwise result strongly
	  // biased by the first MJB point that sets the trend
	  gfit->Fit(f1, "QRN");
	  jesref = f1->GetParameter(0);
	  jesrefe = (f1->GetNDF()>=0 ? f1->GetParError(0) : mjbe);
	  
	  l->DrawLine(ptref, jesref, ptrecoil, jesref * mjb);
	  
	  int n = g->GetN();
	  g->SetPoint(n, ptref, jesref);
	  g->SetPointError(n, 0, jesrefe);
	  n = gi->GetN();
	  gi->SetPoint(n, ptref, jesref);
	  gi->SetPointError(n, 0, jesrefe);
	} // GetN!=0
      
	// Calculate the new JES from reference JES and MJB
	double jes = jesref * mjb;
	double jese = sqrt(pow(mjb*jesrefe,2) + pow(jesref*mjbe,2));
	// Add the new point at pTrecoil
	int n = g->GetN();
	g->SetPoint(n, ptrecoil, jes);
	g->SetPointError(n, 0, jese);
	n = gn->GetN();
	gn->SetPoint(n, ptrecoil, jes);
	gn->SetPointError(n, 0, jese);
	n = gfit->GetN();
	gfit->SetPoint(n, ptrecoil, jes);
	gfit->SetPointError(n, 0, jese);
      } // valid point
    } // for i

    //c1->cd();

    gm->DrawClone("SAMEPz");
    gi->Draw("SAME Pz");
    gn->Draw("SAME Pz");
    g0->Draw("SAME Pz");
    
    TF1 *f2 = new TF1("f2","[0]+[1]*log(0.01*x)+[2]*pow(log(0.01*x),2)",
		      30,1600);
    f2->SetParameters(1,0,0);
    f2->SetLineColor(kBlack);
    gfit->Fit(f2,"QRN");
    f2->Draw("SAME");

    const int n = f2->GetNpar();
    TMatrixD emat(n, n);
    gMinuit->mnemat(&emat[0][0], n);
    _fitError_emat = &emat;
    _fitError_func = f2;
    TF1 *f2e = new TF1("f2e",fitError,30,1600,1);
    f2e->SetLineColor(kBlack);
    f2e->SetLineStyle(kDotted);
    f2e->SetParameter(0,+1);
    f2e->DrawClone("SAME");
    f2e->SetParameter(0,-1);
    f2e->DrawClone("SAME");

    //cmsPrel(19700.);
    CMS_lumi(c1, 2, 0);
    
    TLegend *leg0 = tdrLeg(0.20,0.75,0.50,0.90);
    leg0->AddEntry(gm, "MJB", "PL");
    leg0->AddEntry(herr, "JES unc.", "FL");
    
    TLegend *leg = tdrLeg(0.50,0.60,0.80,0.90);
    leg->AddEntry(g0, "Starting JES", "PL");
    leg->AddEntry(g, "(R_{0} = 0.982#pm0.002)", "");
    leg->AddEntry(gn, "Inferred JES", "PL");
    leg->AddEntry(gi, "Interpolated JES", "PL");
    
    TLatex *tex = new TLatex();
    tex->SetNDC();
    tex->SetTextSize(0.045);
    tex->DrawLatex(0.55,0.20,Form("#chi^{2} / NDF = %1.1f / %d",
				  f2->GetChisquare(), f2->GetNDF()));

    //delete c2;
    
    fe->Close();
    curdir->cd();
    
    c1->RedrawAxis();
    c1->SaveAs(Form("pdf/multijet_%s.pdf",cm));
  }  

  // Repeat the same thing, but this time with a proper global fit
  if (true) { // global fit
    
    vector<TGraphErrors*> vdt(0), vpt(0), vdt2(0), vpt2(0);
    vdt.push_back((TGraphErrors*)gm1->Clone());
    vdt.push_back((TGraphErrors*)gm2->Clone());
    vdt2.push_back((TGraphErrors*)gm1->Clone());
    vdt2.push_back((TGraphErrors*)gm2->Clone());
    vpt.push_back((TGraphErrors*)ge1->Clone());
    vpt.push_back((TGraphErrors*)ge2->Clone());
    vpt2.push_back((TGraphErrors*)ge1->Clone());
    vpt2.push_back((TGraphErrors*)ge2->Clone());
    _vdt = &vdt;
    _vdt2 = &vdt2;
    _vpt = &vpt;
    _vpt2 = &vpt2;
    _vsrc = &vsrc;
    
    TF1 *fm = new TF1("multiFit",multiFit,30,1800,np);
    fm->SetParameters(0.982,-0.005,0.);
    _multiFit = fm;
    
    // Add number of fit and nuisance parameters
    const int nsrc = _vsrc->size();
    Int_t Npar = np+nsrc;
    
    cout << "Global fit has " << np << " fit parameters and "
	 << nsrc << " nuisance parameters" << endl;
    
    // Set initial values for the fit function
    Double_t a[Npar];
    for (int i = 0; i != np; ++i) a[i] = fm->GetParameter(i);
    
    // Setup global chi2 fit (multiFitter is our chi2 function)
    TFitter *fitter = new TFitter(Npar);
    fitter->SetFCN(multiFitter);
    
    // Set initial parameters
    vector<string> parnames(Npar);
    for (int i = 0; i != Npar; ++i)
      fitter->SetParameter(i, parnames[i].c_str(), a[i], 0.01, -100, 100);
    
    // Run fitter (multiple times if needed)
    const int nFit = 1;
    cnt = 0;
    for (int i = 0; i != nFit; ++i)
      fitter->ExecuteCommand("MINI", 0, 0);
    TMatrixD ematg(Npar, Npar);
    gMinuit->mnemat(ematg.GetMatrixArray(), Npar);
    
    // Retrieve the chi2
    Double_t chi2_gbl(0), chi2_src(0), chi2_data(0);
    Double_t grad[Npar];
    Int_t flag = 1;
    TH1D *hsrc = new TH1D("hsrc",";Nuisance parameter;",30,-3,3);
    
    for (int i = 0; i != Npar; ++i) {
      a[i] = fitter->GetParameter(i);
      if (i<np) fm->SetParameter(i, a[i]);
    }
    multiFitter(Npar, grad, chi2_gbl, a, flag);
    for (int i = np; i != Npar; ++i) {
      hsrc->Fill(a[i]);
      chi2_src += pow(a[i],2);
    }
    
    TGraphErrors *gn2 = (*_vdt2)[0]; assert(gn2);
    for (int i = 0; i != gn2->GetN(); ++i) {
      double x = gn2->GetX()[i];
      double y = gn2->GetY()[i];
      double ey = gn2->GetEY()[i];
      chi2_data += pow((y - fm->Eval(x)) / ey, 2);
    }
    TGraphErrors *gn3 = (*_vdt2)[1]; assert(gn3);
    for (int i = 0; i != gn3->GetN(); ++i) {
      double x = gn3->GetX()[i];
      double y = gn3->GetY()[i];
      double ey = gn3->GetEY()[i];
      chi2_data += pow((y - fm->Eval(x)) / ey, 2);
    }

    TGraphErrors *gi2 = (*_vpt2)[0]; assert(gi2);
    TGraphErrors *gi3 = (*_vpt2)[1]; assert(gi3);

    // Setup calculation for uncertainty of interpolated JES
    TF1 *fme = new TF1("fme",fitError,30,1800,1);
    _fitError_func = fm;
    TMatrixD emat2(np, np);
    for (int i = 0; i != np; ++i) {
      for (int j = 0; j != np; ++j) {
	emat2[i][j] = ematg[i][j];
      } // for i
    } // for j
    _fitError_emat = &emat2;
    fme->SetParameter(0,+1);
    
    // Update uncertainty of interpolated and inferred JES
    TGraphErrors *gn2s = (TGraphErrors*)gn2->Clone(); // stat only
    for (int i = 0; i != gi2->GetN(); ++i) {
      double ptref = gi2->GetX()[i];
      double ey = fme->Eval(ptref) - fm->Eval(ptref);
      gi2->SetPointError(i, gi2->GetEX()[i], ey);
      double enew = sqrt(pow(gn2->GetEY()[i],2)+ey*ey);
      gn2->SetPointError(i, gn2->GetEX()[i], enew);
    }
    
    cout << endl;
    cout << "Global chi2/ndf = " << chi2_gbl
	 << " / " << Nk-np << " for " << Nk <<" data points, "
	 << np << " fit parameters and "
	 << nsrc << " uncertainty sources" << endl;
    cout << endl;
    cout << "For data chi2/ndf = " << chi2_data << " / " << Nk << endl;
    cout << "For sources chi2/ndf = " << chi2_src << " / " << nsrc << endl;
    
    cout << "Fit parameters:" << endl;
    for (int i = 0; i != np; ++i) {
      cout << Form("%2d: %12.4g,   ",i+1,a[i]);// << endl;
    }
    cout << endl;
    
    cout << "Uncertainty sources:" << endl;
    for (int i = np; i != Npar; ++i) {
      if ((i-np)%6==0) cout << (*_vsrc)[i-np]->GetName() << endl;
      cout << Form("%2d: %12.3f,   ",i-np+1,a[i]);// << endl;
      if ((i-np)%3==2) cout << endl;
    }
    cout << endl;

    TCanvas *c3 = new TCanvas("c3","c3",600,600);
    gPad->SetLogx();

    TH1D *h = new TH1D("h3",";p_{T}^{recoil} (GeV);Response (Data/MC)",
		       1570,30,1600);
    h->SetMaximum(1.08);
    h->SetMinimum(0.93);
    h->GetXaxis()->SetMoreLogLabels();
    h->GetXaxis()->SetNoExponent();    
    h->DrawClone("AXIS");

    TLatex *tex = new TLatex();
    tex->SetNDC();
    tex->SetTextSize(0.045);

    //TLegend *leg0 = tdrLeg(0.20,0.75,0.50,0.90);
    //leg0->AddEntry(gm, "MJB", "PL");
    //leg0->AddEntry(herr, "JES unc.", "FL");
    
    //TLegend *leg = tdrLeg(0.50,0.60,0.80,0.90);
    //leg->AddEntry(g0, "Starting JES", "PL");
    //leg->AddEntry(g, "(R_{0} = 0.982#pm0.002)", "");
    //leg->AddEntry(gn, "Inferred JES", "PL");
    //leg->AddEntry(gi, "Interpolated JES", "PL");

    //cmsPrel(19700.);
    CMS_lumi(c3, 2, 0);
    //leg->Draw();
    //leg0->Draw();
    
    herr_ref->DrawClone("SAME E3");
    (new TGraph(herr_ref))->Draw("SAME L");
    
    fm->SetLineColor(kBlack);
    fm->DrawClone("SAME");
    
    fme->SetLineStyle(kDotted);
    fme->SetLineColor(fm->GetLineColor());
    fme->SetParameter(0,-1);
    fme->DrawClone("SAME");
    fme->SetParameter(0,+1);
    fme->DrawClone("SAME");
    
    gm1->SetMarkerStyle(kOpenCircle);
    gm1->DrawClone("SAMEPz");
    gm2->SetMarkerStyle(kFullCircle);
    gm2->DrawClone("SAMEPz");

    gi2->SetMarkerStyle(kOpenCircle);
    gi2->SetMarkerColor(kRed);
    gi2->SetLineColor(kRed);
    //gi2->DrawClone("SAMEPz");

    //g0->DrawClone("SAMEPz");

    gn2->SetMarkerStyle(kOpenCircle);
    gn2->SetMarkerColor(kBlue);
    gn2->SetLineColor(kBlue);
    gn2->DrawClone("SAMEPz");

    gn3->SetMarkerStyle(kFullCircle);
    gn3->SetMarkerColor(kBlue);
    gn3->SetLineColor(kBlue);
    gn3->DrawClone("SAMEPz");

    gn2s->SetMarkerStyle(kOpenCircle);
    gn2s->SetMarkerColor(kBlue);
    gn2s->SetLineColor(kBlue);
    gn2s->DrawClone("SAMEP");

    tex->DrawLatex(0.55,0.20,Form("#chi^{2} / NDF = %1.1f / %d",
				  chi2_gbl, Nk-np));


    // Factorize error matrix into eigenvectors
    TVectorD eigvec(np);
    TMatrixD eigmat = emat2.EigenVectors(eigvec);

    vector<TH1D*> hmes(0);
    for (int ieig = 0; ieig != np; ++ieig) {
      
      // Need to recreate because otherwise parameters won't take effect
      // (custom functions become a discretized set of points in cloning)
      TF1 *fmeig = new TF1(Form("ptchs_multijet_feig%d",ieig),
			   multiFit,30,1800,np);
      fmeig->SetLineStyle(kDashed);
      
      // Eigenvector functions
      for (int i = 0; i != np; ++i) {
	fmeig->SetParameter(i, fm->GetParameter(i)
			    + eigmat[i][ieig] * sqrt(eigvec[ieig]));
      }

      // Eigenvector histograms evaluated at bin mean pT
      double ptbins[] = {200, 250, 300, 360, 400, 450, 500, 550, 600,
			 700, 800, 1000, 1200, 1500};
      const int npt = sizeof(ptbins)/sizeof(ptbins[0])-1;
      TH1D *hme = new TH1D(Form("ptchs_multijet_eig%d",ieig),
			   ";p_{T}^{recoil};MJB #times JESref",npt,ptbins);
      
      for (int i = 0; i != gn2s->GetN(); ++i) {

	double pt = gn2s->GetX()[i];
	int ipt = hme->FindBin(pt);
	// Need to store central value as well, because
	// uncertainty sources are signed
	hme->SetBinContent(ipt, fmeig->Eval(pt)-fm->Eval(pt));
	hme->SetBinError(ipt, fabs(fmeig->Eval(pt)-fm->Eval(pt)));
      }
      hmes.push_back(hme);
    } // for ieig
    
    c3->RedrawAxis();

    c3->SaveAs(Form("pdf/multijet_%s_global.pdf",cm));

    if (true) { // Save MJB results to JEC combination file for full global fit
      
      TFile *fout = new TFile("rootfiles/jecdata.root","UPDATE");
      assert(fout && !fout->IsZombie());
      
      assert(gDirectory->cd("ratio"));
      assert(gDirectory->cd("eta00-13"));
      
      if (gm1) {
	gm1->SetMarkerStyle(kOpenCircle);
	gm1->SetMarkerColor(kBlack);
	gm1->SetLineColor(kBlack);
	gm1->SetName(Form("%s_multijet_a30","ptchs"));
	gm1->Write(Form("%s_multijet_a30","ptchs"),TObject::kOverwrite);
      }
      if (gm2) {
	gm2->SetMarkerStyle(kFullCircle);
	gm2->SetMarkerColor(kBlack);
	gm2->SetLineColor(kBlack);
	gm2->SetName(Form("%s_multijet_a30","mpfchs1"));
	gm2->Write(Form("%s_multijet_a30","mpfchs1"),TObject::kOverwrite);
      }
      if (ge1) {
	ge1->SetMarkerStyle(kFullDiamond);
	ge1->SetMarkerColor(kBlack);
	ge1->SetLineColor(kBlack);
	ge1->SetName(Form("%s_multijet_a30","ptf30"));
	ge1->Write(Form("%s_multijet_a30","ptf30"),TObject::kOverwrite);
      }
      if (ge2) {
	ge2->SetMarkerStyle(kFullDiamond);
	ge2->SetMarkerColor(kBlack);
	ge2->SetLineColor(kBlack);
	ge2->SetName(Form("%s_multijet_a30","ptf10"));
	ge2->Write(Form("%s_multijet_a30","ptf10"),TObject::kOverwrite);
      }

      if (!gDirectory->FindObject("fsr")) gDirectory->mkdir("fsr");
      assert(gDirectory->cd("fsr"));

      if (hmes.size()!=0 && hmes[0]) {
	TH1D *hm = (TH1D*)hmes[0]->Clone(); hm->Reset();
	hm->SetName("hkfsr_ptchs_multijet");
	hm->Write(hm->GetName(), TObject::kOverwrite);
	hm->SetName("hkfsr_mpfchs1_multijet");
	hm->Write(hm->GetName(), TObject::kOverwrite);
      }

      for (unsigned int i = 0; i != hmes.size(); ++i) {

	hmes[i]->Reset(); // Don't use these in global fit, just to give binning
	hmes[i]->SetName(Form("hkfsr_ptchs_multijet_eig%d",i));
	hmes[i]->Write(hmes[i]->GetName(), TObject::kOverwrite);
	// Just to fill MPF as well (update with proper one later)
	hmes[i]->SetName(Form("hkfsr_mpfchs1_multijet_eig%d",i));
	hmes[i]->Write(hmes[i]->GetName(), TObject::kOverwrite);
      }
      for (unsigned int i = 0; i != vsrc.size(); ++i) {
	vsrc[i]->SetName(vsrc[i]->GetTitle());
	vsrc[i]->Write(vsrc[i]->GetName(), TObject::kOverwrite);
      }
      
      fout->Close();
    } // if true (write)
  } // if true (global fit

  curdir->cd();
}



Double_t fitError(Double_t *xx, Double_t *p) {

  assert(_fitError_func);
  assert(_fitError_emat);
  double x = *xx;
  double k = p[0];
  TF1 *f = _fitError_func;
  int n = f->GetNpar();
  TMatrixD &emat = (*_fitError_emat);
  assert(emat.GetNrows()==n);
  assert(emat.GetNcols()==n);
  
  vector<double> df(n);
  for (int i = 0; i != n; ++i) {

    double p = f->GetParameter(i);
    double dp = 0.1*sqrt(emat[i][i]);
    f->SetParameter(i, p+dp);
    double fup = f->Eval(x);
    f->SetParameter(i, p-dp);
    double fdw = f->Eval(x);
    f->SetParameter(i, p);
    df[i] = (dp ? (fup - fdw) / (2.*dp) : 0);
  }

  double sumerr2(0);
  for (int i = 0; i != n; ++i) {
    for (int j = 0; j != n; ++j) {
      sumerr2 += emat[i][j]*df[i]*df[j];
    }
  }

  double err = sqrt(sumerr2);

  return (f->Eval(x) + k*err);
}


void multiFitter(Int_t& npar, Double_t* grad, Double_t& chi2, Double_t* par,
	       Int_t flag) {

  // Basic checks
  assert(_vdt);
  assert(_vpt);
  assert(_vdt->size()==_vpt->size());
  assert(_vsrc);
  assert(int(_vsrc->size())==npar-_multiFit->GetNpar()); // nuisance per source
  assert(_vdt->size()<=sizeof(int)*8); // bitmap large enough

  Double_t *ps = &par[_multiFit->GetNpar()];
  int ns = npar - _multiFit->GetNpar();

  // Starting JES, i.e. first fixed reference point
  //double jes0 = 0.982;
  //double ejes0 = 0.002;
  double pt0 = (*_vpt)[0]->GetY()[0] * (*_vdt)[0]->GetX()[0];
  int ipt0 = _herr->FindBin(pt0);
  double jes0 = _herr->GetBinContent(ipt0);
  double ejes0 = _herr->GetBinError(ipt0);

  if (flag) {
    
    // do the following calculation:
    // chi2 = sum_i( (x_i+sum_s(a_s y_si) -fit)^2/sigma_i^2) + sum_s(a_s^2)
    chi2 = 0;
    Nk = 0;

    // Setup parameters for the fit function
    for (int ipar = 0; ipar != _multiFit->GetNpar(); ++ipar) {
      _multiFit->SetParameter(ipar, par[ipar]);
    }

    // Loop over input data (graphs x bins)
    // - for each point, add up source eigenvectors x nuisance parameters
    // - then calculate chi2 adding up residuals + nuisance parameters + jes0
    for (unsigned int ig = 0; ig != _vdt->size(); ++ig) {

      TGraphErrors *g = (*_vdt)[ig]; assert(g);
      TGraphErrors *gp = (*_vpt)[ig]; assert(gp);
      assert(g->GetN()==gp->GetN());

      for (int i = 0; i != g->GetN(); ++i) {

	// Retrieve central value and uncertainty for this point
	double ptrecoil = g->GetX()[i];
	double ptref = gp->GetY()[i] * ptrecoil;
	double data = g->GetY()[i];
	double sigma = g->GetEY()[i];
	
	// Calculate the fit value at this point
	double jes = _multiFit->EvalPar(&ptrecoil,par);
	double jesref = _multiFit->EvalPar(&ptref,par);
	double fit = jes / jesref;

	// Calculate total shift caused by all nuisance parameters
	double shifts = 0;
	for (unsigned int is = 0; is != _vsrc->size(); ++is) {

	  TH1D *hsrc = (*_vsrc)[is]; assert(hsrc);
	  // Bitmap controls which datasets this eigenvector applies to
	  int bitmap; char foo[256];
	  sscanf(hsrc->GetName(),"bm%d_%s", &bitmap,foo);
	  if (!(bitmap>0)) {
	    cout << "Source "<<hsrc->GetName()<<" bitmap "<<bitmap<<endl<<flush;
	  }
	  assert(bitmap>0);
	  bool ison = (bitmap & (1<<ig));
	  int ipt = hsrc->FindBin(ptrecoil);

	  if (ison) shifts += ps[is] * hsrc->GetBinContent(ipt);
	}

	// Add chi2 from residual
	double chi = (data + shifts - fit) / sigma;
	chi2 += chi * chi;
	++Nk;

	// if _vdt2 is provided, store shifted data there
	if (_vdt2 && _vdt2->size()==_vdt->size()) {
	  TGraphErrors *g2 = (*_vdt2)[ig];
	  if (g2 && g2->GetN()==g->GetN() && g2->GetX()[i]==ptrecoil) {
	    g2->SetPoint(i, ptrecoil, (data + shifts) * jesref);
	  }
	}
	// if _vpt2 is provided, store shifted reference points there
	if (_vpt2 && _vpt2->size()==_vpt->size()) {
	  TGraphErrors *gp2 = (*_vpt2)[ig];
	  if (gp2 && gp2->GetN()==gp->GetN()) {// &&
	    gp2->SetPoint(i, ptref, jesref);
	    // Add uncertainties after the fit when they're available
	    gp2->SetPointError(i, 0, 0.004);//0.002);
	  }
	}
      } // for ipt
    } // for ig

    // Add chi2 from nuisance parameters
    for (int is = 0; is != ns; ++is) {
      chi2 += ps[is]*ps[is];
    } // for ipar

    // Add chi2 from starting JES
    double chi = (jes0 - _multiFit->EvalPar(&pt0, par)) / ejes0;
    chi2 += chi * chi;
    
    // Give some feedback on progress in case loop gets stuck
    if ((++cnt)%1000==0) cout << "." << flush;
  } // if flag
  else {
    if (grad) {}; // suppress warning;
    return;
  }

} // multiFitter
