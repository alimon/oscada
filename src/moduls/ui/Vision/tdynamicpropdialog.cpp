
#include <qlayout.h>
#include <qlabel.h>
#include <qwidgetstack.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qspinbox.h>
#include "tdynamicpropdialog.h"

using namespace VISION;

TDynamicPropDialog::TDynamicPropDialog(QWidget* parent, TItem *item, const QString &propName, QString &propText)//(QWidget* parent, const char* name, bool modal)
   : QDialog(parent, "", true, (  WType_Dialog | WStyle_Customize | WStyle_Title | WStyle_DialogBorder | WStyle_SysMenu ) )
{
   setCaption(tr("��������� �������� " + QString::number(item->rtti()) + ":" + propName));
   
   QLabel *lbDyn = new QLabel(tr(QString::number(item->rtti()) + ":" + propName + ". ��� ��������: "), this);
   QComboBox *qbType = new QComboBox(false, this);
   qbType->insertItem(tr("value"));
   qbType->insertItem(tr("enum"));
   qbType->insertItem(tr("proportion"));
   //return;
   
   
   //---������� value---
   QWidget *wValue = new QWidget();
   // ��� ��������� � ��� ��������:
   QHBoxLayout *hlV = new QHBoxLayout(wValue);//������� Value
   hlV->setMargin(11);
   hlV->setSpacing(6);
   QVBoxLayout *vlVParamAttr = new QVBoxLayout();//����� �������� (1) �� ���������, ��������
   QLabel *lbVValue = new QLabel(tr("��������:"), wValue);
   vlVParamAttr->addWidget(lbVValue);
   QHBoxLayout *hlVParamAttr = new QHBoxLayout();//����� �������� (2) �� ���������, ��������
   QLineEdit *leVParamAttr = new QLineEdit(wValue);
   hlVParamAttr->addWidget(leVParamAttr);
   QPushButton *btVParamAttr = new QPushButton("...", wValue);
   hlVParamAttr->addWidget(btVParamAttr);
   vlVParamAttr->addLayout(hlVParamAttr);
   // ������ ���������� �����:
   QVBoxLayout *vlVFormat = new QVBoxLayout();
   QLabel *lbVFormat = new QLabel(tr("������:"), wValue);
   vlVFormat->addWidget(lbVFormat);
   QHBoxLayout *hlVFormat = new QHBoxLayout();
   QComboBox *cbVFormat = new QComboBox(false, wValue);
   cbVFormat->insertItem(tr("f"));
   cbVFormat->insertItem(tr("E"));
   cbVFormat->insertItem(tr("e"));
   cbVFormat->insertItem(tr("G"));
   cbVFormat->insertItem(tr("g"));
   hlVFormat->addWidget(cbVFormat);
   QSpinBox *sbVFormat = new QSpinBox(0, 16, 1, wValue);
   sbVFormat->setValue(2);
   hlVFormat->addWidget(sbVFormat);
   vlVFormat->addLayout(hlVFormat);
   
   hlV->addLayout(vlVParamAttr);
   hlV->addLayout(vlVFormat);
   //---������� value---
   
   
   QWidgetStack *wStack = new QWidgetStack(this);
   wStack->setFrameStyle(QFrame::Box | QFrame::Sunken);
   wStack->addWidget(wValue, 1);
 
   QPushButton *btOk = new QPushButton(tr("&Ok"), this);
   QPushButton *btCancel = new QPushButton(tr("&Cancel"), this);
   //QWidget *wMain = new QWidget(this);
   QHBoxLayout *hlTop = new QHBoxLayout();
   hlTop->addWidget(lbDyn);
   hlTop->addWidget(qbType);
   
   QHBoxLayout *hlBottom = new QHBoxLayout();
   hlBottom->addStretch(1);
   hlBottom->addWidget(btOk);
   hlBottom->addWidget(btCancel);
      
   QVBoxLayout *vlMain = new QVBoxLayout(this);
   vlMain->setMargin(11);
   vlMain->setSpacing(6);
   vlMain->addLayout(hlTop);
   vlMain->addWidget(wStack);
   vlMain->addStretch(1);
   vlMain->addLayout(hlBottom);
   
   /*QVBoxLayout mainLayout;
   QHBoxLayout topLayout;
   QHBoxLayout middleLayout;
   QHBoxLayout bottomLayout;
   */
   wStack->raiseWidget(1);
}

TDynamicPropDialog::~TDynamicPropDialog()
{
}

QString TDynamicPropDialog::getDynamic(QWidget *parent, TItem *item, const QString &propName, QString &propText)
{
   TDynamicPropDialog *dlg = new TDynamicPropDialog(parent, item, propName, propText);
   
   //dlg->show();
   int resultCode = dlg->exec();
   
   delete dlg;
}