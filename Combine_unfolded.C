#include "TFile.h"
#include "TTree.h"
#include "TChain.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TMatrixD.h"
#include "TVectorD.h"
#include "TLorentzVector.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <set>

#include "UnfoldingHistogramFactory.h"

#define NR_BINS     10
#define NR_CHANNELS 4



// Gives index as a function of bin and channel
// All indices assume to start from zero



int index_from_bin_and_channel(int ibin, int ichan) {

  if ( ibin<0 || ibin>=NR_BINS 
       || ichan < 0 || ichan > NR_CHANNELS) {

    std::cout << "index_vs_bin_and_channel : INVALID INPUT " << ibin << "\t" << ichan << std::endl;
    return -1;
  }
  return ibin*NR_CHANNELS + ichan;
}

void bin_and_channel_from_index(int index, int & bin, int & channel) {

  if (index<0 || index > NR_BINS * NR_CHANNELS) {
    std::cout << "bin_and_channel_from_index: INVALID INPUT : " << index << std::endl;
    bin     = -1;
    channel = -1;
    return;
  }
  bin      = index/NR_CHANNELS;
  channel  = index % NR_CHANNELS;

}

using namespace std;


TMatrixD *  GetFinalCovariance(int nbins, int nchannels, 
			       TMatrixD M ) {
  // M is the inverse of the full Covariance Matrix


  //  TMatrix M=fullCovariance.Invert();

  TMatrixD D(nbins,nbins);

  for (int ibin1=0; ibin1<nbins; ibin1++) {
    for (int ibin2=0; ibin2<nbins; ibin2++) {

      double val=0;
      for (int ichan1=0; ichan1<nchannels; ichan1++) {
	for (int ichan2=0; ichan2<nchannels; ichan2++) {
	  int index1 = index_from_bin_and_channel(ibin1,ichan1);
	  int index2 = index_from_bin_and_channel(ibin2,ichan2);
	  val += M(index1,index2);
	}
      }
      D(ibin1,ibin2) = val;
    }
  }

  TMatrixD Dcopy(D);

  TMatrixD * finalCovariance  = new TMatrixD(D.Invert());

  TMatrixD testUnity = Dcopy*(*finalCovariance);

  std::cout << "======= SANITY CHECK =============== \n";
  testUnity.Print();
  std::cout << "======= END SANITY CHECK =============== \n";

  return finalCovariance;



}

void DumpMatrixToFile(TMatrixD matrix,std::string name="matrix.dat") {

  std::ofstream outf(name.c_str(),std::ofstream::out);

  outf << matrix.GetNrows() << "\t" << matrix.GetNcols() << std::endl;
  for (int irow=0; irow<matrix.GetNrows(); irow++) {
    for (int icol=0; icol<matrix.GetNcols(); icol++) {
      outf << "\t" << matrix(irow,icol);
    }
    outf << endl;
  }
  outf.close();
}



void Combine_unfolded(
		      std::map<int, TH1D* >      & h_dsigma,
		      std::map<int, TH1D* >      & h_dsigmadx,
		      std::map<int, TMatrixD *>  & unfoldingCovariance,
		      std::map<int, TMatrixD *>  & binCovariance
		      )
{


  bool useFullCorrelations=false;

  int verbosityLevel=0;

  // Read measurements from input

  
  std::map<int, TH1D* >::iterator ithist, ithist2;

  std::map<int, TH1D* >   differential_xsections;

  for (ithist=h_dsigma.begin(); ithist!=h_dsigma.end(); ithist++) {

    int channel = ithist->first;
    if (verbosityLevel>0) {
    std::cout << "CHANNEL = " << channel << std::endl
	      << "============== \n";
    }
    TH1D * h = ithist->second;
    int nbins = h->GetNbinsX();

    // ithist2=h_dsigmadx.find(ithist->first);
    // TH1D * h2(0);
    // if (ithist2 != h_dsigmadx.end()) {
    //   int nbins2 = ithist2->second->GetNbinsX();
    //   if (nbins2 != nbins) {
    // 	std::cout << "MISMATCH IN # of bins!!! \n";
    //   } else {
    // 	h2 = ithist2->second;
    //   }
    // }
       
    std::ostringstream diffXsName;
    diffXsName << "h_diffXS_" << channel;
    TH1D * h_diffxs = (TH1D*) h->Clone(diffXsName.str().c_str());

    for (int ibin=0; ibin<=nbins+1; ibin++) {
      double val = h->GetBinContent(ibin);
      double wid = h->GetBinWidth(ibin);
      double err = h->GetBinError(ibin);

      h_diffxs->SetBinContent(ibin,val/wid);
      h_diffxs->SetBinError(ibin,err/wid);

      if (verbosityLevel>0) {
      std::cout << "Bin : " << ibin
		<< "  Val = " << val << "  Error = " << err 
		<< "  Val/dX = " << val/wid
		<< "  Rel. =  " << err/val ;
      // if (h2) {
      // 	double val2 = h2->GetBinContent(ibin);
      // 	double err2 = h2->GetBinError(ibin);
      // 	std::cout << "\t DSDX  Val = " << val2 << "  Error = " << err2 
      // 		  << "  Rel. =  " << err2/val2 
      // 		  << "\t R = " << (err/val) / (err2/val2);
      // }
      std::cout << std::endl;
      }

    }
    differential_xsections.insert(std::pair<int,TH1D *> (channel, h_diffxs));
  }

  // Read matrices from input

  int nr_bins     = binCovariance.size();
  int nr_channels = unfoldingCovariance.size();

  std::map<int, TMatrixD *>::iterator it;

  std::cout << "FULL BLUE UNFOLDING " 
	    << "\t # bins = " << nr_bins
	    << "\t # channels = " << nr_channels << endl;


  // BUILD CORRELATION AND UNFOLDING COVARINACE MATRIX
  ///   DOUBT: 
  /// we have nbins measured bins
  /// BUT: unfolding covariance matrix is (nbins+2)*(nbins+2)
  // Contains under- and overflow bins: for bin=0 and bin=NBins
  //   OUR CHOICE:

  // NOTE NOTE NOTE:
  // We DO *NOT* TAKE UNDER- AND OVERFLOW BINS

  std::map<int, TMatrixD *>  unfoldingCorrelation;
  std::map<int, TMatrixD *>  unfoldingErrorMatrix;

  for (it=unfoldingCovariance.begin(); it!=unfoldingCovariance.end(); it++) {
    std::cout << "Channel " << it->first << std::endl;
    int channel = it->first;
    //    it->second->Print();
    int dim=it->second->GetNrows();
    // remove under- and overflow bins
    dim -= 2;

    TMatrixD * correlationMatrix    = new TMatrixD(dim,dim);
    TMatrixD * errorMatrix          = new TMatrixD(dim,dim);
    for (int irow=0; irow<correlationMatrix->GetNrows(); irow++) {
      for (int icol=irow; icol<correlationMatrix->GetNcols(); icol++) {
	double cov   = 	(*it->second)(irow+1,icol+1);
	double sig_x = sqrt( (*it->second)(irow+1,irow+1)); 
	double sig_y = sqrt( (*it->second)(icol+1,icol+1));
	double corr_xy = cov/(sig_x*sig_y);

	//	std::cout << "row : " << irow << "  col : " << icol
	//		  << " cov = " << cov << "\t sx = " << sig_x << "\t xy = " << sig_y << std::endl;
	(*correlationMatrix)(irow,icol) = corr_xy;
	(*correlationMatrix)(icol,irow) = corr_xy;
	//  And now produce real unfolding covariance
	// Correctly normalized to cross section

	// double unf_error_x = differential_xsections[channel]->GetBinError(irow+1);
	// double unf_error_y = differential_xsections[channel]->GetBinError(icol+1);
	double unf_error_x = h_dsigma[channel]->GetBinError(irow+1);
	double unf_error_y = h_dsigma[channel]->GetBinError(icol+1);

	double cov_xy   = corr_xy*unf_error_x*unf_error_y;

	(*errorMatrix)(irow,icol) = cov_xy;
	(*errorMatrix)(icol,irow) = cov_xy;
      }
    }

    unfoldingCorrelation.insert(std::pair<int,TMatrixD*> ( it->first, correlationMatrix));
    unfoldingErrorMatrix.insert(std::pair<int,TMatrixD*> ( it->first, errorMatrix));

    if (verbosityLevel>1) {
      std::cout << "Correlation Matrix \n";
      correlationMatrix->Print();
      std::cout << "Error Matrix \n";
      errorMatrix->Print();
    }
  }

  //

  int dimension = nr_bins*nr_channels;

  //  vector<TMatrixD*>  unfoldingCovariance;   // [nr_channels]
  //  vector<TMatrixD*>  channelCovariance;     // [nr_bins]

  TMatrixD  fullCovariance(dimension,dimension);
  //  TMatrixD  lumiCovariance(dimension,dimension);
  TMatrixDSym  lumiCovariance(dimension);
  TMatrixD  statCovariance(dimension,dimension);

  // Build full covariance matrix

  for (int ibin1=0; ibin1<nr_bins; ibin1++) {
    for (int ibin2=0; ibin2<nr_bins; ibin2++) {

      // SHIFT TO JUMP OVER UNDERFLOW BIN
      int binIndex = ibin1+1;

      for (int ichan1=0; ichan1<nr_channels; ichan1++) {
	for (int ichan2=0; ichan2<nr_channels; ichan2++) {


	  double sig1 = h_dsigma[ichan1]->GetBinContent(ibin1+1);
	  double sig2 = h_dsigma[ichan2]->GetBinContent(ibin2+1);
	  double cov(0);
	  double lumiCov(0);
	  double statCov(0);

	  if (ibin1 == ibin2 && ichan1 == ichan2) {
	    cov     = (*binCovariance[binIndex])(ichan1,ichan2);
	    lumiCov = 0.026*0.026*sig1*sig2;
	    statCov = (*unfoldingErrorMatrix[ichan1])(ibin1,ibin2);
	  }
	  else if (ibin1 == ibin2) {
	    cov = (*binCovariance[binIndex])(ichan1,ichan2);
	    lumiCov = 0.026*0.026*sig1*sig2;

	  } else if (ichan1 == ichan2) {
	    if (useFullCorrelations) {
	      cov     = (*unfoldingErrorMatrix[ichan1])(ibin1,ibin2);
	      statCov = cov;
	      lumiCov = 0.026*0.026*sig1*sig2;
	    }
	  } else {
	    cov = 0.;
	    //	    lumiCov = 0.026*0.026*sig1*sig2;
	  }
	  int index1 = index_from_bin_and_channel(ibin1,ichan1);
	  int index2 = index_from_bin_and_channel(ibin2,ichan2);
	  fullCovariance(index1,index2) = cov;
	  //if (index1<=index2) 
	  lumiCovariance(index1,index2) = lumiCov;
	  statCovariance(index1,index2) = statCov;
	}  
      }    
    }
  }

  // std::cout << "FULL BLOW COVARIANCE : \n";
  // std::cout << "====================== \n \n";
  // fullCovariance.Print();
  std::cout << "FULL BLOWN LUMI COVARIANCE : \n";
  std::cout << "========================= \n \n";
  lumiCovariance.Print();
  std::cout << "Full Covariance Determinant = " << fullCovariance.Determinant() << std::endl;

  // Build full measurements vector

  //  int nall = nr_bins*nr_channels;

  TVectorD  a(dimension);
  for (int ibin=0; ibin<nr_bins; ibin++) {
    for (int ichan=0; ichan<nr_channels; ichan++) {
      double val = differential_xsections[ichan]->GetBinContent(ibin+1);
      int index = index_from_bin_and_channel(ibin,ichan);
      a(index) = val;
    }
  }

  if (verbosityLevel>1) {
    std::cout << "MEASUREMENT VECTOR: \n";
    std::cout << "====================== \n \n";
    a.Print();
  }

  // Print out measurements with errors
  for (int ibin=0; ibin<nr_bins && verbosityLevel>0; ibin++) {
    double binWidth = h_dsigma[0]->GetBinWidth(ibin+1);
    for (int ichan=0; ichan<nr_channels; ichan++) {
      int index = index_from_bin_and_channel(ibin,ichan);
      double val = a(index);
      double err = fullCovariance(index,index);
      double stat_err = statCovariance(index,index);
      double lumi_err = lumiCovariance(index,index);
      std::cout << "Measurement in bin " << ibin << "\t channel: " << ichan
		<< "\t:\t" << val << " +/- " << sqrt(err)/binWidth
		<< "\t " << sqrt(stat_err)/binWidth << " (stat.)"
		<< "\t " << sqrt(lumi_err)/binWidth << " (lumi.)" 
		<< std::endl;
    }
    std::cout << "========================================================== \n";
  }

  // Invert full covariance matrix and auxiliary vector
  // 
  // M = Cov-1
  // 
  // f = M*a

  std::cout << "LUMI COVARIANCE (AGAIN) \n";
  lumiCovariance.Print();
  std::cout << "inverting lumi Covariance \n";
  TMatrixDSym lumiCovCopy(lumiCovariance);
  DumpMatrixToFile(lumiCovariance, "lumicov.txt");
  TMatrixDSym Mlumi(lumiCovariance);
  Mlumi.Invert();
  Mlumi.Print();
  // Sanity check: multiply with the inverse: do we get unity???
  TMatrixD testUnity = Mlumi*lumiCovariance; // lumiCovCopy;
  std::cout << "Should be unity: M*Minv = \n";
  testUnity.Print();

  std::cout << "inverting full Covariance \n";
  TMatrixD M = fullCovariance.Invert();

  //  lumiCovariance.Print();
  std::cout << "inverting stat Covariance \n";
  TMatrixD Mstat = statCovariance.Invert();
  TVectorD f = M*a;

  // std::cout << "LUMI COV INVERSE : \n";
  // Mlumi.Print();
  // 

  TMatrixD * finalCov     = GetFinalCovariance(nr_bins,nr_channels,M);
  TMatrixD * finalCovLumi = GetFinalCovariance(nr_bins,nr_channels,Mlumi);
  TMatrixD * finalCovStat = GetFinalCovariance(nr_bins,nr_channels,Mstat);

  std::cout << "First inversion done \n";

  // Build vector of n_bins component, each summing all channels of that bin

  TVectorD g(nr_bins);
  for (int ibin1=0; ibin1<nr_bins; ibin1++) {
    double val =0;
    for (int ichan1=0; ichan1<nr_channels; ichan1++) {
      int index1 = index_from_bin_and_channel(ibin1,ichan1);      
      val += f(index1);
    }
    g(ibin1) = val;
  }


  // And now get result and final covariance matrix

  //  D.Print();

  TVectorD result(nr_bins);
  TMatrixD finalCovariance(nr_bins,nr_bins);

  /*
  TMatrixD Dinv=D.Invert();
  */

  std::cout << "Second inversion done \n";
  
  //  result = Dinv * g;
  result = (*finalCov) * g;

  //  finalCovariance = Dinv;
  finalCovariance = *finalCov;

  //  Dinv.Print();

  if (verbosityLevel>0) {
    finalCov->Print();
    std::cout << " Lumi. Covariance: \n";
    finalCovLumi->Print();
    std::cout << " Stat. Covariance: \n";
    finalCovStat->Print();
  }
  // Get bin widths to correct for it...
  double * binWidths(0);

  //  std::map<int, TH1D* >::iterator ithist;
  ithist = h_dsigma.find(0);
  if (ithist != h_dsigma.end() ) {
    int nbins = ithist->second->GetNbinsX();
    binWidths = new double[nbins];
    for (int ibin=0; ibin<nbins; ibin++) {
      binWidths[ibin] = ithist->second->GetBinWidth(ibin+1);
    }
  } else {
    std::cout << "DID NOT FIND HISTOGRAM TO GET WIDTHS... \n";
  }

  std::cout << "FINAL RESULTS: \n \n";
  for (int ibin=0; ibin<nr_bins ;ibin++) {
    double err      = sqrt(finalCovariance(ibin,ibin));
    double err_stat = sqrt( (*finalCovStat)(ibin,ibin));
    double err_lumi = sqrt( (*finalCovLumi)(ibin,ibin));
    if (binWidths) {
      err      *= (1./binWidths[ibin]);
      err_stat *= (1./binWidths[ibin]);
      err_lumi *= (1./binWidths[ibin]);
    }
    std::cout << "Bin " << ibin 
	      << "\t Value = " << result(ibin)
      //	      << " +/- " << sqrt(finalCovariance(ibin,ibin)) << std::endl;
	      << " +/- " << err
	      << "\t" << err_stat << "(stat)"
	      << "\t" << err_lumi << "(lumi)"
	      << std::endl;
  }
 
}
