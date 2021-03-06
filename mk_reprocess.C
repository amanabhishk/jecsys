{
  // For JEC residual (and pile-up)
  gROOT->ProcessLine(".L CondFormats/JetMETObjects/src/Utilities.cc+");
  gROOT->ProcessLine(".L CondFormats//JetMETObjects/src/JetCorrectorParameters.cc+");
  gROOT->ProcessLine(".L CondFormats/JetMETObjects/src/SimpleJetCorrector.cc+");
  gROOT->ProcessLine(".L CondFormats/JetMETObjects/src/FactorizedJetCorrector.cc+");
  //
  gROOT->ProcessLine(".L CondFormats/JetMETObjects/src/SimpleJetCorrectionUncertainty.cc+");
  gROOT->ProcessLine(".L CondFormats/JetMETObjects/src/JetCorrectionUncertainty.cc+");
  gROOT->ProcessLine(".L tools.C+");
  gROOT->ProcessLine(".L reprocess.C+");
  gROOT->ProcessLine(".L softrad.C+");
  gROOT->ProcessLine(".L multijet.C+");
  gROOT->ProcessLine(".L globalFitL3res.C+");

  // Merge inputs from separate groups
  // NB: this does not need to be run, if the merged inputs
  //     are already available in 'rootfiles/jecdata.root'
  reprocess();

  // Calculate soft radiation (ISR+FSR) corrections
  // and uncertainty eigenvectors for global fit
  softrad(0.0,1.3,true);
  softrad(1.3,1.9,true);
  softrad(1.9,2.5,true);
  softrad(2.5,3.0,true);
  softrad(3.0,3.2,true);
  softrad(3.2,5.2,true);
  softrad(0.0,1.3,true); // redo for plots

  // Run multijet analysis to store information for later global fit
  multijet(false);    
  multijet(true);

  // Perform final global fit (goes into GT)
  globalFitL3Res(0.0,1.3); // L3Res
  // These are just checks for now:
  globalFitL3Res(1.3,1.9); // coarse L2Res
  globalFitL3Res(1.9,2.5); // coarse L2Res
  globalFitL3Res(2.5,3.0); // coarse L2Res
  globalFitL3Res(3.0,3.2); // coarse L2Res
  globalFitL3Res(3.2,5.2); // coarse L2Res
  // Redo for plots and fit results for L3Res
  // (above eta bins may overwrite some plots)
  globalFitL3Res(0.0,1.3);
}
