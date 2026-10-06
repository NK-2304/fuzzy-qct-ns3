#include "fuzzy-engine.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

namespace fuzzyqct {

using namespace fl;

FuzzyEngine::FuzzyEngine(double davgRange, double sdavgRange)
  : m_engine(nullptr), m_davg(nullptr), m_sdavg(nullptr),
    m_deltaMidTh(nullptr), m_e(nullptr)
{
  BuildEngine(davgRange, sdavgRange);
}

FuzzyEngine::~FuzzyEngine()
{
  delete m_engine;
}

void
FuzzyEngine::BuildEngine(double davgRange, double sdavgRange)
{
  m_engine = new Engine;
  m_engine->setName("FuzzyQCT");

  // ---------------- Inputs: 5-term partition matching NH/NL/ZE/PL/PH ----------------
  m_davg = new InputVariable;
  m_davg->setName("davg");
  m_davg->setRange(-davgRange, davgRange);
  m_davg->addTerm(new Triangle("NH", -davgRange * 1.5, -davgRange,        -davgRange * 0.5));
  m_davg->addTerm(new Triangle("NL", -davgRange,        -davgRange * 0.5,  0.0));
  m_davg->addTerm(new Triangle("ZE", -davgRange * 0.5,   0.0,              davgRange * 0.5));
  m_davg->addTerm(new Triangle("PL",  0.0,               davgRange * 0.5,  davgRange));
  m_davg->addTerm(new Triangle("PH",  davgRange * 0.5,   davgRange,        davgRange * 1.5));
  m_engine->addInputVariable(m_davg);

  m_sdavg = new InputVariable;
  m_sdavg->setName("sdavg");
  m_sdavg->setRange(-sdavgRange, sdavgRange);
  m_sdavg->addTerm(new Triangle("NH", -sdavgRange * 1.5, -sdavgRange,       -sdavgRange * 0.5));
  m_sdavg->addTerm(new Triangle("NL", -sdavgRange,        -sdavgRange * 0.5, 0.0));
  m_sdavg->addTerm(new Triangle("ZE", -sdavgRange * 0.5,   0.0,              sdavgRange * 0.5));
  m_sdavg->addTerm(new Triangle("PL",  0.0,                sdavgRange * 0.5, sdavgRange));
  m_sdavg->addTerm(new Triangle("PH",  sdavgRange * 0.5,   sdavgRange,       sdavgRange * 1.5));
  m_engine->addInputVariable(m_sdavg);

  // ---------------- Output 1: delta_mid_th [-1, 1] ----------------
  m_deltaMidTh = new OutputVariable;
  m_deltaMidTh->setName("deltaMidTh");
  m_deltaMidTh->setRange(-1.0, 1.0);
  m_deltaMidTh->setDefaultValue(0.0);
  m_deltaMidTh->setAggregation(new Maximum);
  m_deltaMidTh->setDefuzzifier(new Centroid(100));
  m_deltaMidTh->addTerm(new Triangle("LowerMax",  -1.5,  -1.0,  -0.75));
  m_deltaMidTh->addTerm(new Triangle("LowerFast", -1.0,  -0.75, -0.5));
  m_deltaMidTh->addTerm(new Triangle("LowerMed",  -0.75, -0.5,  -0.25));
  m_deltaMidTh->addTerm(new Triangle("LowerSlow", -0.5,  -0.25,  0.0));
  m_deltaMidTh->addTerm(new Triangle("Keep",      -0.25,  0.0,   0.25));
  m_deltaMidTh->addTerm(new Triangle("RaiseSlow",  0.0,   0.25,  0.5));
  m_deltaMidTh->addTerm(new Triangle("RaiseMed",   0.25,  0.5,   0.75));
  m_deltaMidTh->addTerm(new Triangle("RaiseFast",  0.5,   0.75,  1.0));
  m_deltaMidTh->addTerm(new Triangle("RaiseMax",   0.75,  1.0,   1.5));
  m_engine->addOutputVariable(m_deltaMidTh);

  // ---------------- Output 2: drop exponent e [1.0, 3.0] ----------------
  m_e = new OutputVariable;
  m_e->setName("e");
  m_e->setRange(1.0, 3.0);
  m_e->setDefaultValue(2.0);
  m_e->setAggregation(new Maximum);
  m_e->setDefuzzifier(new Centroid(100));
  m_e->addTerm(new Triangle("Aggressive",   0.75, 1.0,  1.25));
  m_e->addTerm(new Triangle("VeryLow",      1.0,  1.25, 1.5));
  m_e->addTerm(new Triangle("Low",          1.25, 1.5,  1.75));
  m_e->addTerm(new Triangle("SlightLow",    1.5,  1.75, 2.0));
  m_e->addTerm(new Triangle("Balanced",     1.75, 2.0,  2.25));
  m_e->addTerm(new Triangle("SlightHigh",   2.0,  2.25, 2.5));
  m_e->addTerm(new Triangle("High",         2.25, 2.5,  2.75));
  m_e->addTerm(new Triangle("VeryHigh",     2.5,  2.75, 3.0));
  m_e->addTerm(new Triangle("Conservative", 2.75, 3.0,  3.25));
  m_engine->addOutputVariable(m_e);

  // ---------------- Rule block: Monotonic and Point-Antisymmetric ----------------
  RuleBlock* rb = new RuleBlock;
  rb->setConjunction(new Minimum);
  rb->setDisjunction(new Maximum);
  rb->setImplication(new Minimum);
  rb->setActivation(new General);

  auto addRule = [&](const std::string& davgTerm, const std::string& sdavgTerm,
                     const std::string& midThTerm, const std::string& eTerm)
  {
    std::string text = "if davg is " + davgTerm + " and sdavg is " + sdavgTerm +
                       " then deltaMidTh is " + midThTerm + " and e is " + eTerm;
    rb->addRule(Rule::parse(text, m_engine));
  };

  // davg = PH (queue growing fastest)
  addRule("PH", "NH", "LowerMed",  "Low");
  addRule("PH", "NL", "LowerFast", "VeryLow");
  addRule("PH", "ZE", "LowerFast", "VeryLow");
  addRule("PH", "PL", "LowerMax",  "Aggressive");
  addRule("PH", "PH", "LowerMax",  "Aggressive");

  // davg = PL
  addRule("PL", "NH", "Keep",      "Balanced");
  addRule("PL", "NL", "LowerSlow", "SlightLow");
  addRule("PL", "ZE", "LowerMed",  "Low");
  addRule("PL", "PL", "LowerMed",  "Low");
  addRule("PL", "PH", "LowerFast", "VeryLow");

  // davg = ZE
  addRule("ZE", "NH", "RaiseMed",  "High");
  addRule("ZE", "NL", "RaiseSlow", "SlightHigh");
  addRule("ZE", "ZE", "Keep",      "Balanced");
  addRule("ZE", "PL", "LowerSlow", "SlightLow");
  addRule("ZE", "PH", "LowerMed",  "Low");

  // davg = NL
  addRule("NL", "NH", "RaiseFast", "VeryHigh");
  addRule("NL", "NL", "RaiseMed",  "High");
  addRule("NL", "ZE", "RaiseMed",  "High");
  addRule("NL", "PL", "RaiseSlow", "SlightHigh");
  addRule("NL", "PH", "Keep",      "Balanced");

  // davg = NH (queue shrinking fastest)
  addRule("NH", "NH", "RaiseMax",  "Conservative");
  addRule("NH", "NL", "RaiseMax",  "Conservative");
  addRule("NH", "ZE", "RaiseFast", "VeryHigh");
  addRule("NH", "PL", "RaiseFast", "VeryHigh");
  addRule("NH", "PH", "RaiseMed",  "High");

  m_engine->addRuleBlock(rb);

  std::string status;
  if (!m_engine->isReady(&status))
    {
      throw std::runtime_error("FuzzyEngine is not ready: " + status);
    }
}

FuzzyOutput
FuzzyEngine::Evaluate(double davg, double sdavg)
{
  double clampedDavg = std::max(m_davg->getMinimum(), std::min(m_davg->getMaximum(), davg));
  double clampedSdavg = std::max(m_sdavg->getMinimum(), std::min(m_sdavg->getMaximum(), sdavg));

  m_davg->setValue(clampedDavg);
  m_sdavg->setValue(clampedSdavg);
  m_engine->process();

  FuzzyOutput out;
  double vMid = m_deltaMidTh->getValue();
  double vE = m_e->getValue();

  out.deltaMidTh = std::isnan(vMid) ? 0.0 : vMid;
  out.e = std::isnan(vE) ? 2.0 : vE;
  return out;
}

} // namespace fuzzyqct
