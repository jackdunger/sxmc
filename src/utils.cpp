#include <iostream>
#include <vector>
#include <string>
#include <assert.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TH1.h>
#include <TDirectory.h>
#include <TNtuple.h>
#include <TMath.h>
#include "utils.h"

float get_ntuple_entry(TNtuple* nt, int i, std::string field) {
  float v;
  nt->SetBranchAddress(field.c_str(), &v);
  assert(i < nt->GetEntries());
  nt->GetEvent(i);
  nt->ResetBranchAddresses();
  return v;
}


std::vector<float> get_correlation_matrix(TNtuple* nt) {
  int nentries = nt->GetEntries();

  // get list of branch names
  std::vector<std::string> names;
  for (int i=0; i<nt->GetListOfBranches()->GetEntries(); i++) {
    std::string name = nt->GetListOfBranches()->At(i)->GetName();
    if (name == "likelihood") {
      continue;
    }
    names.push_back(name);
  }

  std::vector<float> matrix(names.size() * names.size());

  // convert the ntuple to a vector, calculating means as we go
  std::vector<float> table(names.size() * nentries);
  std::vector<float> means(names.size(), 0);
  for (int i=0; i<nentries; i++) {
    for (size_t j=0; j<names.size(); j++) {
      float v = get_ntuple_entry(nt, i, names.at(j));
      table.at(j + i * names.size()) = v;
      means.at(j) += v;
    }
  }

  // sums to means
  for (size_t i=0; i<names.size(); i++) {
    means.at(i) /= nentries;
  }

  // compute correlations
  for (size_t i=0; i<names.size(); i++) {
    for (size_t j=i; j<names.size(); j++) {
      float t = 0;
      float dx2 = 0;
      float dy2 = 0;
      for (int k=0; k<nentries; k++) {
        float x1 = table.at(i + k * names.size()) - means.at(i);
        float x2 = table.at(j + k * names.size()) - means.at(j);
        t += x1 * x2;
        dx2 += x1 * x1;
        dy2 += x2 * x2;
      }
      matrix.at(i * names.size() + j) = t / TMath::Sqrt(dx2 * dy2);
    }
  }

  return matrix;
}


SpectralPlot::SpectralPlot(int _line_width, float _xmin, float _xmax,
                           float _ymin, float _ymax, bool _logy,
                           std::string _title,
                           std::string _xtitle,
                           std::string _ytitle)
    : logy(_logy), line_width(_line_width),
      xmin(_xmin), xmax(_xmax), ymin(_ymin), ymax(_ymax),
      title(_title), xtitle(_xtitle), ytitle(_ytitle) {
  this->c = new TCanvas();

  if (this->logy) {
    this->c->SetLogy();
  }

  this->legend = new TLegend(0.85, 0.15, 0.985, 0.95);
  this->legend->SetFillColor(kWhite);
}


SpectralPlot::~SpectralPlot() {
  for (size_t i=0; i<this->histograms.size(); i++) {
    delete this->histograms[i];
  }
  delete this->legend;
}


void SpectralPlot::add(TH1* _h, std::string title, std::string options) {
  std::string name = "__" + std::string(title);
  TH1* h = dynamic_cast<TH1*>(_h->Clone(name.c_str()));
  h->SetDirectory(NULL);

  h->SetLineWidth(this->line_width);
  h->SetTitle(this->title.c_str());
  h->SetXTitle(xtitle.c_str());
  h->SetYTitle(ytitle.c_str());

  this->legend->AddEntry(h, title.c_str());

  if (!h || h->Integral() == 0) {
    return;
  }

  this->histograms.push_back(h);

  if (this->histograms.size() == 1) {
    h->SetAxisRange(this->ymin, this->ymax, "Y");
    h->SetAxisRange(this->xmin, this->xmax, "X");
    h->GetXaxis()->SetLabelFont(132);
    h->GetXaxis()->SetTitleFont(132);
    h->GetYaxis()->SetLabelFont(132);
    h->GetYaxis()->SetTitleFont(132);
    if (this->logy) {
      this->c->SetLogy();
    }
    this->c->cd();
    h->DrawClone(options.c_str());
  }
  else {
    this->c->cd();
    h->DrawClone(("same " + options).c_str());
  }
  this->c->Update();
}


void SpectralPlot::save(std::string filename) {
  this->c->cd();
  this->legend->SetTextFont(132);
  this->legend->Draw();
  this->c->Update();
  this->c->SaveAs(filename.c_str());
}


TH1* SpectralPlot::make_like(TH1* h, std::string name) {
  TH1* hnew = dynamic_cast<TH1*>(h->Clone(name.c_str()));
  hnew->Reset();
  return hnew;
}

