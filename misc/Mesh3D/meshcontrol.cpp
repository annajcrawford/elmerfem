#include <QtGui>
#include <iostream>
#include "meshcontrol.h"

using namespace std;

MeshControl::MeshControl(QWidget *parent)
  : QDialog(parent)
{
  tetlibPresent = true;
  nglibPresent = true;

  ui.setupUi(this);

  connect(ui.tetlibRadioButton, SIGNAL(clicked()), this, SLOT(tetlibClicked()));
  connect(ui.nglibRadioButton, SIGNAL(clicked()), this, SLOT(nglibClicked()));
  connect(ui.elmerGridRadioButton, SIGNAL(clicked()), this, SLOT(elmerGridClicked()));

  connect(ui.tetlibStringEdit, SIGNAL(textChanged(const QString&)), this, SLOT(defineTetlibControlString(const QString&)));

  connect(ui.nglibMaxHEdit, SIGNAL(textChanged(const QString&)), this, SLOT(defineNglibMaxH(const QString&)));
  connect(ui.nglibFinenessEdit, SIGNAL(textChanged(const QString&)), this, SLOT(defineNglibFineness(const QString&)));
  connect(ui.nglibBgmeshEdit, SIGNAL(textChanged(const QString&)), this, SLOT(defineNglibBackgroundmesh(const QString&)));

  connect(ui.defaultsButton, SIGNAL(clicked()), this, SLOT(defaultControls()));
  connect(ui.closeButton, SIGNAL(clicked()), this, SLOT(close()));

  connect(ui.elmerGridStringEdit, SIGNAL(textChanged(const QString&)), this, SLOT(defineElmerGridControlString(const QString&)));

  defaultControls();
}

MeshControl::~MeshControl()
{
}

void MeshControl::tetlibClicked()
{
  generatorType = GEN_TETLIB;
}

void MeshControl::nglibClicked()
{
  generatorType = GEN_NGLIB;
}

void MeshControl::elmerGridClicked()
{
  generatorType = GEN_ELMERGRID;
}

void MeshControl::defineTetlibControlString(const QString &qs)
{
  tetlibControlString = qs;
}

void MeshControl::defineNglibMaxH(const QString &qs)
{
  nglibMaxH = qs;
}

void MeshControl::defineNglibFineness(const QString &qs)
{
  nglibFineness = qs;
}

void MeshControl::defineNglibBackgroundmesh(const QString &qs)
{
  nglibBackgroundmesh = qs;
}

void MeshControl::defineElmerGridControlString(const QString &qs)
{
  elmerGridControlString = qs;
  // cout << string(elmerGridControlString.toAscii()) << endl;
  // cout.flush();
}

void MeshControl::defaultControls()
{
  generatorType = GEN_TETLIB;
  ui.tetlibRadioButton->setChecked(true);

  if(!tetlibPresent) {
    ui.nglibRadioButton->setChecked(true);
    generatorType = GEN_NGLIB;
  }

  ui.tetlibStringEdit->setText("nnJApq1.414V");
  ui.nglibMaxHEdit->setText("1000000");
  ui.nglibFinenessEdit->setText("0.5");
  ui.nglibBgmeshEdit->setText("");
  ui.elmerGridStringEdit->setText("default");
}
