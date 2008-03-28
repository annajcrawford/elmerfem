#include <QtGui>
#include <iostream>
#include "edfeditor.h"

using namespace std;

EdfEditor::EdfEditor(QWidget *parent)
  : QWidget(parent)
{
  addIcon = QIcon(":/icons/list-add.png");
  removeIcon = QIcon(":/icons/list-remove.png");

  setWindowFlags(Qt::Window);

  // Tree widget:
  //-------------
  edfTree = new QTreeWidget;

  connect(edfTree, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
	  this, SLOT(treeItemClicked(QTreeWidgetItem*,int)));

  edfTree->setColumnCount(3);
  edfTree->setColumnWidth(0,200);
  edfTree->setColumnWidth(1,200);
  edfTree->setColumnWidth(2,200);

  //edfTree->header()->setResizeMode(QHeaderView::Stretch);

  QStringList qsl;
  qsl << "Tag" << "Attributes" << "Value";
  edfTree->setHeaderLabels(qsl);
  edfTree->setAlternatingRowColors(true);

  // Buttons:
  //---------
  addButton = new QPushButton(tr("&Add child"));
  addButton->setIcon(addIcon);
  connect(addButton, SIGNAL(clicked()), this, SLOT(addButtonClicked()));
  
  removeButton = new QPushButton(tr("&Remove"));
  removeButton->setIcon(removeIcon);
  connect(removeButton, SIGNAL(clicked()), this, SLOT(removeButtonClicked()));

  QHBoxLayout *buttonLayout = new QHBoxLayout;  
  buttonLayout->addWidget(addButton);
  buttonLayout->addWidget(removeButton);

  // Main layout:
  //-------------
  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(edfTree);
  mainLayout->addLayout(buttonLayout);
  setLayout(mainLayout);  
}

//----------------------------------------------------------------------------
EdfEditor::~EdfEditor()
{
}

//----------------------------------------------------------------------------
void EdfEditor::insertEntry(QDomElement element,
			    QTreeWidgetItem *parentItem)
{
  if(element.isNull())
    return;

  if(parentItem != NULL)
    parentItem->setExpanded(true);
  
  QTreeWidgetItem *newItem = new QTreeWidgetItem(parentItem);
  
  newItem->setText(0, element.tagName().trimmed());
  newItem->setFlags(newItem->flags() | Qt::ItemIsEditable);

  if(element.firstChildElement().isNull()) {

    // display attributes
    QStringList list;
    QDomNamedNodeMap attributeMap = element.attributes();
    for(int index = 0; index < attributeMap.count(); index++) {
      QDomNode attribute = attributeMap.item(index);
      list << attribute.nodeName() + "=\"" + attribute.nodeValue() + "\"";
    }
    newItem->setText(1, list.join(" "));

    // display value
    newItem->setText(2, element.text().split("\n").join(" ").trimmed());
  }
  
  edfTree->addTopLevelItem(newItem);
  
  if(!element.firstChildElement().isNull()) 
    insertEntry(element.firstChildElement(), newItem);
  
  insertEntry(element.nextSiblingElement(), parentItem);      
}

//----------------------------------------------------------------------------
void EdfEditor::setupEditor(QDomDocument &elmerDefs)
{
  // get root entry & recursively add all entries to the tree:
  edfTree->clear();
  root = elmerDefs.documentElement();
  insertEntry(root, NULL);
  edfTree->setCurrentItem(NULL);
}

//----------------------------------------------------------------------------
QSize EdfEditor::minimumSizeHint() const
{
  return QSize(64, 64);
}

//----------------------------------------------------------------------------
QSize EdfEditor::sizeHint() const
{
  return QSize(720, 480);
}

//----------------------------------------------------------------------------
void EdfEditor::addButtonClicked()
{
  QTreeWidgetItem *current = edfTree->currentItem();
  
  if(current == NULL)
    return;

  QTreeWidgetItem *newItem = new QTreeWidgetItem(current);
  newItem->setFlags(newItem->flags() | Qt::ItemIsEditable);
  newItem->setText(0, "[Tag]");
  newItem->setText(1, "[Attributes]");
  newItem->setText(2, "[Value]");
  current->addChild(newItem);
  newItem->parent()->setExpanded(true);
}

//----------------------------------------------------------------------------
void EdfEditor::removeButtonClicked()
{
  QTreeWidgetItem *current = edfTree->currentItem();

  if(current == NULL)
    return;

  QTreeWidgetItem *parent = current->parent();
  parent->removeChild(current);
}

//----------------------------------------------------------------------------
void EdfEditor::treeItemClicked(QTreeWidgetItem *item, int column)
{
  cout << "Item clicked: ";
  cout << string(item->text(column).trimmed().toAscii());
  cout << endl;
  cout.flush();
}
