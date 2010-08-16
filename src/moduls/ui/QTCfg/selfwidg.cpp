
//OpenSCADA system module UI.QTCfg file: selfwidg.cpp
/***************************************************************************
 *   Copyright (C) 2004-2008 by Roman Savochenko                           *
 *   rom_as@fromru.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; version 2 of the License.               *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QApplication>
#include <QPainter>
#include <QPaintEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QItemEditorFactory>
#include <QMetaProperty>
#include <QSpinBox>
#include <QCalendarWidget>
#include <QToolTip>
#include <QStatusBar>
#include <QMenu>

#include <tsys.h>

#include "tuimod.h"
#include "selfwidg.h"

using namespace QTCFG;

//*************************************************
//* ImgView: Image view widget.                   *
//*************************************************
ImgView::ImgView( QWidget * parent, Qt::WindowFlags f, int ih_sz, int iv_sz ) :
    QWidget(parent,f), h_sz(ih_sz), v_sz(iv_sz)
{

}

ImgView::~ImgView( )
{

}

bool ImgView::setImage( const string &imgdata )
{
    bool rez = m_img.loadFromData((const uchar*)imgdata.c_str(),imgdata.size());

    if(rez)
    {
	m_img = m_img.scaled(QSize(h_sz?vmin(h_sz,m_img.width()):m_img.width(),
				   v_sz?vmin(v_sz,m_img.height()):m_img.height()),Qt::KeepAspectRatio);
	setMinimumSize(m_img.width(),m_img.height());
    }
    else
    {
	m_img = QImage();
	setMinimumSize(200,40);
    }

    update();

    return rez;
}

void ImgView::paintEvent( QPaintEvent * )
{
    QPainter pnt( this );
    if(m_img.isNull())
    {
	pnt.setWindow(0,0,rect().width(),rect().height());
	pnt.setPen(QColor(255,0,0));
	pnt.setBackground(QBrush(QColor(210,237,234)));
	pnt.drawRect(0,0,199,39);
	pnt.drawText(3,3,194,34,Qt::AlignCenter,_("Picture is not set!"));
    }
    else
    {
	pnt.setWindow( 0, 0, rect().width(), rect().height() );
	pnt.drawImage(QPoint(0,0),m_img);
	pnt.setPen(QColor(0,0,255));
	pnt.drawRect( 0, 0, m_img.width()-1, m_img.height()-1 );
    }
}

//*********************************************************************************************
//* Universal edit line widget. Contain support of: QLineEdit, QSpinBox, QDoubleSpinBox,      *
//* QTimeEdit, QDateEdit and QDateTimeEdit.                                                   *
//*********************************************************************************************
LineEdit::LineEdit( QWidget *parent, LType tp, bool prev_dis ) :
    QWidget( parent ), m_tp((LineEdit::LType)-1), bt_fld(NULL), ed_fld(NULL), mPrev(!prev_dis)
{
    QHBoxLayout *box = new QHBoxLayout(this);
    box->setMargin( 0 );
    box->setSpacing( 0 );

    setType( tp );
}

void LineEdit::viewApplyBt( bool view )
{
    if( view == (bool)bt_fld ) return;

    if( view && !bt_fld )
    {
	bt_fld = new QPushButton(this);
	bt_fld->setIcon( QIcon(":/images/ok.png") );
	bt_fld->setIconSize( QSize(12,12) );
	bt_fld->setSizePolicy( QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed) );
	//bt_fld->setMaximumWidth( 15 );
	connect( bt_fld, SIGNAL( clicked() ), this, SLOT( applySlot() ) );
	layout()->addWidget( bt_fld );
    }
    if( !view && bt_fld ) { bt_fld->deleteLater(); bt_fld = NULL; }
}

bool LineEdit::isEdited( )	{ return bt_fld; }

void LineEdit::setType( LType tp )
{
    if( tp == m_tp ) return;

    //> Delete previous
    if( tp >= 0 && ed_fld ) delete ed_fld;

    //> Create new widget
    switch( tp )
    {
	case Text:
	    ed_fld = new QLineEdit(this);
	    connect( (QLineEdit*)ed_fld, SIGNAL( textEdited(const QString&) ), SLOT( changed() ) );
	    break;
	case Integer:
	    ed_fld = new QSpinBox(this);
	    connect( (QSpinBox*)ed_fld, SIGNAL( valueChanged(int) ), SLOT( changed() ) );
	    break;
	case Real:
	    ed_fld = new QDoubleSpinBox(this);
	    connect( (QDoubleSpinBox*)ed_fld, SIGNAL( valueChanged(double) ), SLOT( changed() ) );
	    break;
	case Time:
	    ed_fld = new QTimeEdit(this);
	    connect( (QTimeEdit*)ed_fld, SIGNAL( timeChanged(const QTime&) ), SLOT( changed() ) );
	    break;
	case Date:
	    ed_fld = new QDateEdit(this);
	    ((QDateEdit*)ed_fld)->setCalendarPopup(true);
	    ((QDateEdit*)ed_fld)->calendarWidget()->setGridVisible(true);
	    ((QDateEdit*)ed_fld)->calendarWidget()->setFirstDayOfWeek(Qt::Monday);
	    connect( (QDateEdit*)ed_fld, SIGNAL( dateChanged(const QDate&) ), SLOT( changed() ) );
	    break;
	case DateTime:
	    ed_fld = new QDateTimeEdit(this);
	    ((QDateTimeEdit*)ed_fld)->setCalendarPopup(true);
	    ((QDateTimeEdit*)ed_fld)->calendarWidget()->setGridVisible(true);
	    ((QDateTimeEdit*)ed_fld)->calendarWidget()->setFirstDayOfWeek(Qt::Monday);
	    connect( (QDateTimeEdit*)ed_fld, SIGNAL( dateTimeChanged(const QDateTime&) ), SLOT( changed() ) );
	    break;
	case Combo:
	    ed_fld = new QComboBox(this);
	    ((QComboBox*)ed_fld)->setEditable(true);
	    connect( (QComboBox*)ed_fld, SIGNAL( editTextChanged(const QString&) ), SLOT( changed() ) );
	    connect( (QComboBox*)ed_fld, SIGNAL( activated(int) ), this, SLOT( applySlot() ) );
	    break;
    }
    ((QBoxLayout*)layout())->insertWidget(0,ed_fld);
    setFocusProxy( ed_fld );

    m_tp = tp;
}

void LineEdit::changed( )
{
    //> Enable apply
    if( mPrev && !bt_fld )	viewApplyBt(true);

    emit valChanged(value());
}

void LineEdit::setValue(const QString &txt)
{
    if( ed_fld ) ed_fld->blockSignals(true);
    switch(type())
    {
	case Text:
	    ((QLineEdit*)ed_fld)->setText(txt);
	    ((QLineEdit*)ed_fld)->setCursorPosition(0);
	    break;
	case Integer:
	    ((QSpinBox*)ed_fld)->setValue(txt.toInt());
	    break;
	case Real:
	    ((QDoubleSpinBox*)ed_fld)->setValue(txt.toDouble());
	    break;
	case Time:
	    ((QTimeEdit*)ed_fld)->setTime(QTime().addSecs(txt.toInt()));
	    break;
	case Date: case DateTime:
	    ((QDateTimeEdit*)ed_fld)->setDateTime(QDateTime::fromTime_t(txt.toInt()));
	    break;
	case Combo:
	    if( ((QComboBox*)ed_fld)->findText(txt) < 0 ) ((QComboBox*)ed_fld)->addItem(txt);
	    ((QComboBox*)ed_fld)->setEditText(txt);
	    break;
    }
    if( ed_fld ) ed_fld->blockSignals(false);

    m_val = txt;

    if( bt_fld ) viewApplyBt(false);
}

void LineEdit::setCfg(const QString &cfg)
{
    switch(type())
    {
	case Text:	((QLineEdit*)ed_fld)->setInputMask(cfg);	break;
	case Integer:
	{
	    int		minv = 0, maxv = 100, sstep = 1;
	    string	pref, suff;
	    if( !cfg.isEmpty() )
	    {
		minv  = atoi(TSYS::strSepParse(cfg.toAscii().data(),0,':').c_str());
		maxv  = atoi(TSYS::strSepParse(cfg.toAscii().data(),1,':').c_str());
		sstep = atoi(TSYS::strSepParse(cfg.toAscii().data(),2,':').c_str());
		pref  = TSYS::strSepParse(cfg.toAscii().data(),3,':');
		suff  = TSYS::strSepParse(cfg.toAscii().data(),4,':');
	    }
	    ((QSpinBox*)ed_fld)->setRange(minv,maxv);
	    ((QSpinBox*)ed_fld)->setSingleStep(sstep);
	    ((QSpinBox*)ed_fld)->setPrefix(pref.c_str());
	    ((QSpinBox*)ed_fld)->setSuffix(suff.c_str());
	    break;
	}
	case Real:
	{
	    double minv = 0, maxv = 100, sstep = 1;
	    string pref, suff;
	    int    dec = 2;
	    if( !cfg.isEmpty() )
	    {
		minv  = atof(TSYS::strSepParse(cfg.toAscii().data(),0,':').c_str());
		maxv  = atof(TSYS::strSepParse(cfg.toAscii().data(),1,':').c_str());
		sstep = atof(TSYS::strSepParse(cfg.toAscii().data(),2,':').c_str());
		pref  = TSYS::strSepParse(cfg.toAscii().data(),3,':');
		suff  = TSYS::strSepParse(cfg.toAscii().data(),4,':');
		dec   = atoi(TSYS::strSepParse(cfg.toAscii().data(),5,':').c_str());
	    }
	    ((QDoubleSpinBox*)ed_fld)->setRange(minv,maxv);
	    ((QDoubleSpinBox*)ed_fld)->setSingleStep(sstep);
	    ((QDoubleSpinBox*)ed_fld)->setPrefix(pref.c_str());
	    ((QDoubleSpinBox*)ed_fld)->setSuffix(suff.c_str());
	    ((QDoubleSpinBox*)ed_fld)->setDecimals(dec);
	    break;
	}
	case Time: case Date: case DateTime:
	    ((QDateTimeEdit*)ed_fld)->setDisplayFormat(cfg);
	    break;
	case Combo:
	{
	    QString ctext = ((QComboBox*)ed_fld)->currentText();
	    ((QComboBox*)ed_fld)->clear();
	    ((QComboBox*)ed_fld)->addItems(cfg.split("\n"));
	    if( ((QComboBox*)ed_fld)->findText(ctext) < 0 ) ((QComboBox*)ed_fld)->addItem(ctext);
	    ((QComboBox*)ed_fld)->setEditText(ctext);
	    break;
	}
    }
    if( bt_fld ) viewApplyBt(false);
}

QString LineEdit::value()
{
    switch(type())
    {
	case Text:	return ((QLineEdit*)ed_fld)->text();
	case Integer:	return QString::number(((QSpinBox*)ed_fld)->value());
	case Real:	return QString::number(((QDoubleSpinBox*)ed_fld)->value());
	case Time:	return QString::number(QTime().secsTo(((QTimeEdit*)ed_fld)->time()));
	case Date: case DateTime:
	    return QString::number(((QDateTimeEdit*)ed_fld)->dateTime().toTime_t());
	case Combo:	return ((QComboBox*)ed_fld)->currentText();
    }
    return "";
}

void LineEdit::applySlot( )
{
    viewApplyBt(false);

    if( m_val == value() ) return;
    m_val = value();

    emit valChanged(value());
    emit apply();
}

bool LineEdit::event( QEvent * e )
{
    if( e->type() == QEvent::KeyRelease && bt_fld )
    {
	QKeyEvent *keyEvent = (QKeyEvent *)e;
	if( keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return )
	{
	    bt_fld->animateClick( );
	    return true;
	}
	else if(keyEvent->key() == Qt::Key_Escape )
	{
	    emit cancel();
	    setValue(m_val);
	    return true;
	}
    }
    return QWidget::event(e);
}

//*************************************************
//* TextEdit: Text edit widget.                   *
//*************************************************
TextEdit::TextEdit( QWidget *parent, const char *name, bool prev_dis ) :
    QWidget(parent), but_box(NULL), isInit(false)
{
    setObjectName(name);
    QVBoxLayout *box = new QVBoxLayout(this);
    box->setMargin(0);
    box->setSpacing(0);

    ed_fld = new QTextEdit(this);
    ed_fld->setContextMenuPolicy(Qt::CustomContextMenu);
    ed_fld->setTabStopWidth(20);
    ed_fld->setAcceptRichText(false);
    connect(ed_fld, SIGNAL(textChanged()), this, SLOT(changed()));
    connect(ed_fld, SIGNAL(cursorPositionChanged()), this, SLOT(curPosChange()));
    connect(ed_fld, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(ctrTreePopup()));
    box->addWidget(ed_fld);

    QImage ico_t;
    if( !ico_t.load(TUIS::icoPath("find").c_str()) ) ico_t.load(":/images/find.png");
    actFind = new QAction(QPixmap::fromImage(ico_t), _("Find"), ed_fld);
    actFind->setShortcut(Qt::CTRL+Qt::Key_F);
    connect(actFind, SIGNAL(activated()), this, SLOT(find()));
    ed_fld->addAction(actFind);
    actFindNext = new QAction(_("Find next"), ed_fld);
    actFindNext->setShortcut(Qt::Key_F3);
    connect(actFindNext, SIGNAL(activated()), this, SLOT(find()));
    ed_fld->addAction(actFindNext);

    if( !prev_dis )
    {
	but_box = new QDialogButtonBox( QDialogButtonBox::Apply|
					QDialogButtonBox::Cancel, Qt::Horizontal, this );
	QImage ico_t;
	but_box->button(QDialogButtonBox::Apply)->setText(_("Apply"));
	if(!ico_t.load(TUIS::icoPath("button_ok").c_str())) ico_t.load(":/images/button_ok.png");
	but_box->button(QDialogButtonBox::Apply)->setIcon(QPixmap::fromImage(ico_t));
	connect(but_box->button(QDialogButtonBox::Apply), SIGNAL(clicked()), this, SLOT(btApply()));
	but_box->button(QDialogButtonBox::Cancel)->setText(_("Cancel"));
	if(!ico_t.load(TUIS::icoPath("button_cancel").c_str())) ico_t.load(":/images/button_cancel.png");
	but_box->button(QDialogButtonBox::Cancel)->setIcon(QPixmap::fromImage(ico_t));
	connect(but_box, SIGNAL(rejected()), this, SLOT(btCancel()));
	but_box->setVisible(false);
	box->addWidget(but_box);
    }
}

bool TextEdit::isChanged( )
{
    if( but_box && but_box->isVisible() ) return true;
    return false;
}

QString TextEdit::text()
{
    return ed_fld->toPlainText();
}

bool TextEdit::hasFocus( ) const
{
    return ed_fld->hasFocus( );
}

void TextEdit::setText(const QString &text)
{
    isInit=true;
    ed_fld->setPlainText(text);
    ed_fld->document()->setModified(false);
    isInit=false;
    changed();
}

void TextEdit::changed()
{
    if( isInit ) return;
    if( but_box ) but_box->setVisible(ed_fld->document()->isModified());
    emit textChanged(text());
}

void TextEdit::btApply( )
{
    emit textChanged(text());
    but_box->setVisible(false);
    emit apply();
}

void TextEdit::btCancel( )
{
    but_box->setVisible(false);
    emit cancel();
}

void TextEdit::curPosChange( )
{
    ((QMainWindow*)window())->statusBar()->showMessage(QString(_("Cursor = (%1:%2)")).arg(ed_fld->textCursor().blockNumber()+1).arg(ed_fld->textCursor().columnNumber()+1),10000);
}

bool TextEdit::event( QEvent *e )
{
    if( but_box && e->type() == QEvent::KeyRelease )
    {
	QKeyEvent *keyEvent = (QKeyEvent *)e;
	if( (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return) && QApplication::keyboardModifiers()&Qt::ControlModifier )
	{
	    but_box->button(QDialogButtonBox::Apply)->animateClick( );
	    return true;
	}
	else if( keyEvent->key() == Qt::Key_Escape )
	{
	    but_box->button(QDialogButtonBox::Cancel)->animateClick( );
	    return true;
	}
    }
    return QWidget::event(e);
}

void TextEdit::ctrTreePopup( )
{
    QMenu *menu = ed_fld->createStandardContextMenu();
    menu->addSeparator();
    menu->addAction(actFind);
    menu->addAction(actFindNext);
    menu->exec(QCursor::pos());
    delete menu;
}

void TextEdit::find( )
{
    bool isFind = false;
    int fopt = (QTextDocument::FindFlag)actFind->objectName().section(':',0,0).toInt();
    QString fstr = actFind->objectName().section(':',1);
    if( sender() == actFind )
    {
	InputDlg dlg(this,actFind->icon(),QString(_("Enter text string for search:")),_("String search"),0,0);
	QLineEdit *le = new QLineEdit(fstr,&dlg);
	dlg.ed_lay->addWidget(le, 0, 0);
	QCheckBox *bw = new QCheckBox(_("Backward"),&dlg);
	if( fopt & QTextDocument::FindBackward ) bw->setCheckState(Qt::Checked);
	dlg.ed_lay->addWidget(bw, 1, 0);
	QCheckBox *cs = new QCheckBox(_("Case sensitively"),&dlg);
	if( fopt & QTextDocument::FindCaseSensitively ) cs->setCheckState(Qt::Checked);
	dlg.ed_lay->addWidget(cs, 2, 0);
	QCheckBox *ww = new QCheckBox(_("Whole words"),&dlg);
	if( fopt & QTextDocument::FindWholeWords ) ww->setCheckState(Qt::Checked);
	dlg.ed_lay->addWidget(ww, 3, 0);
	le->setFocus(Qt::OtherFocusReason);
	dlg.resize(400,210);
	if( dlg.exec() == QDialog::Accepted && !le->text().isEmpty() )
	{
	    fopt = (QTextDocument::FindFlag)0;
	    if( bw->checkState()==Qt::Checked ) fopt |= QTextDocument::FindBackward;
	    if( cs->checkState()==Qt::Checked ) fopt |= QTextDocument::FindCaseSensitively;
	    if( ww->checkState()==Qt::Checked ) fopt |= QTextDocument::FindWholeWords;
	    fstr = le->text();
	    isFind = true;
	}
    }
    else if( sender() == actFindNext && !fstr.isEmpty() ) isFind = true;

    if( isFind )
    {
	ed_fld->find(fstr,(QTextDocument::FindFlag)fopt);
	actFind->setObjectName(QString::number(fopt)+":"+fstr);
    }
}

//*************************************************
//* InputDlg: Id and name input dialog.           *
//*************************************************
InputDlg::InputDlg( QWidget *parent, const QIcon &icon, const QString &mess,
	const QString &ndlg, int with_id, int with_nm, QDialogButtonBox::StandardButtons buttons ) :
		QDialog(parent), mId(NULL), mName(NULL)
{
    setWindowTitle(ndlg);
    setMinimumSize( QSize( 120, 150 ) );
    setWindowIcon(icon);
    setSizeGripEnabled(true);

    QVBoxLayout *dlg_lay = new QVBoxLayout(this);
    dlg_lay->setMargin(10);
    dlg_lay->setSpacing(6);

    //- Icon label and text message -
    QHBoxLayout *intr_lay = new QHBoxLayout;
    intr_lay->setSpacing(6);

    QLabel *icon_lab = new QLabel(this);
    icon_lab->setSizePolicy( QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum) );
    icon_lab->setPixmap(icon.pixmap(48));
    intr_lay->addWidget(icon_lab);

    inpLab = new QLabel(mess,this);
    inpLab->setWordWrap(true);
    intr_lay->addWidget(inpLab);
    dlg_lay->addItem(intr_lay);

    //- Id and name fields -
    ed_lay = new QGridLayout;
    if( with_nm || with_id )
    {
	ed_lay->setSpacing(6);
	if( with_id )
	{
	    mIdLab = new QLabel(_("ID:"),this);
	    ed_lay->addWidget( mIdLab, 3, 0 );
	    mId = new QLineEdit(this);
	    mId->setMaxLength(with_id);
	    ed_lay->addWidget( mId, 3, 1 );
	}
	if( with_nm )
	{
	    mNameLab = new QLabel(_("Name:"),this);
	    ed_lay->addWidget( mNameLab, 4, 0 );
	    mName = new QLineEdit(this);
	    mName->setMaxLength(with_nm);
	    ed_lay->addWidget( mName, 4, 1 );
	}
    }
    dlg_lay->addItem(ed_lay);

    //- Qk and Cancel buttons -
    dlg_lay->addItem( new QSpacerItem( 10, 0, QSizePolicy::Minimum, QSizePolicy::Expanding ) );

    QFrame *sep = new QFrame(this);
    sep->setFrameShape( QFrame::HLine );
    sep->setFrameShadow( QFrame::Raised );
    dlg_lay->addWidget( sep );

    QDialogButtonBox *but_box = new QDialogButtonBox( buttons, Qt::Horizontal, this );
    QImage ico_t;
    if( buttons & QDialogButtonBox::Ok )
    {
	but_box->button(QDialogButtonBox::Ok)->setText(_("Ok"));
	if(!ico_t.load(TUIS::icoPath("button_ok").c_str())) ico_t.load(":/images/button_ok.png");
	but_box->button(QDialogButtonBox::Ok)->setIcon(QPixmap::fromImage(ico_t));
	connect(but_box, SIGNAL(accepted()), this, SLOT(accept()));
    }
    if( buttons & QDialogButtonBox::Cancel )
    {
	but_box->button(QDialogButtonBox::Cancel)->setText(_("Cancel"));
	if(!ico_t.load(TUIS::icoPath("button_cancel").c_str())) ico_t.load(":/images/button_cancel.png");
	but_box->button(QDialogButtonBox::Cancel)->setIcon(QPixmap::fromImage(ico_t));
	connect(but_box, SIGNAL(rejected()), this, SLOT(reject()));
    }
    dlg_lay->addWidget( but_box );

    resize(400,150+(35*(with_nm?1:0))+(35*(with_id?1:0)));
}

QString InputDlg::id()
{
    if( mId )  return mId->text();
    return "";
}

QString InputDlg::name()
{
    if( mName )return mName->text();
    return "";
}

QString InputDlg::mess( )
{
    return inpLab->text();
}

void InputDlg::setId(const QString &val)
{
    if( mId )  mId->setText(val);
}

void InputDlg::setName(const QString &val)
{
    if( mName ) mName->setText(val);
}

void InputDlg::setMess( const QString &val )
{
    inpLab->setText( val );
}

//*****************************************************
//* ReqIdNameDlg: Request node identifier and/or name *
//*****************************************************
ReqIdNameDlg::ReqIdNameDlg( QWidget *parent, const QIcon &icon, const QString &mess, const QString &ndlg ) :
    InputDlg( parent, icon, mess, ndlg , 20, 500 )
{
    itTpLab = new QLabel(_("Item type:"),this);
    ed_lay->addWidget( itTpLab, 0, 0 );
    itTp = new QComboBox(this);
    itTp->setSizePolicy( QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed) );
    ed_lay->addWidget( itTp, 0, 1 );
    connect( itTp, SIGNAL( currentIndexChanged(int) ), this, SLOT( selectItTp(int) ) );
}

string ReqIdNameDlg::target( )
{
    if( itTp->count() <= 0 ) return "";

    return itTp->itemData(itTp->currentIndex()).toString().toAscii().data();
}

void ReqIdNameDlg::setTargets( const vector<string> &tgs )
{
    itTp->clear();
    int defPos = 0;
    for( int i_t = 0; i_t < tgs.size(); i_t++ )
    {
	itTp->addItem( TSYS::strSepParse(tgs[i_t],3,'\n').c_str(), tgs[i_t].c_str() );
	if( atoi(TSYS::strSepParse(tgs[i_t],4,'\n').c_str()) ) defPos = itTp->count()-1;
    }
    if( tgs.size() ) itTp->setCurrentIndex(defPos);
    bool tpView = !(itTp->count()==1 && itTp->itemText(0).isEmpty());
    itTpLab->setVisible(tpView); itTp->setVisible(tpView);
}

void ReqIdNameDlg::selectItTp( int it )
{
    if( it < 0 ) return;
    string its = itTp->itemData(it).toString().toAscii().data();
    int idSz = atoi(TSYS::strSepParse(its,0,'\n').c_str());
    if( idSz>0 ) mId->setMaxLength(idSz);
    mIdLab->setVisible(idSz>=0); mId->setVisible(idSz>=0);
    bool idm = atoi(TSYS::strSepParse(its,1,'\n').c_str());
    mNameLab->setVisible(idm); mName->setVisible(idm);
}

//*************************************************
//* DlgUser: User select dialog                   *
//*************************************************
DlgUser::DlgUser( QWidget *parent ) : QDialog(parent)
{
    setWindowTitle(_("Select user"));

    QVBoxLayout *dlg_lay = new QVBoxLayout(this);
    dlg_lay->setMargin(10);
    dlg_lay->setSpacing(6);

    QGridLayout *ed_lay = new QGridLayout;
    ed_lay->setSpacing(6);
    ed_lay->addWidget( new QLabel(_("User:"),this), 0, 0 );
    users = new QComboBox(this);
    ed_lay->addWidget( users, 0, 1 );
    ed_lay->addWidget( new QLabel(_("Password:"),this), 1, 0 );
    passwd = new QLineEdit(this);
    passwd->setEchoMode( QLineEdit::Password );
    ed_lay->addWidget( passwd, 1, 1 );
    dlg_lay->addItem(ed_lay);

    dlg_lay->addItem( new QSpacerItem( 20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding ) );

    QFrame *sep = new QFrame(this);
    sep->setFrameShape( QFrame::HLine );
    sep->setFrameShadow( QFrame::Raised );
    dlg_lay->addWidget( sep );

    QDialogButtonBox *but_box = new QDialogButtonBox( QDialogButtonBox::Ok|
						      QDialogButtonBox::Cancel, Qt::Horizontal, this );
    QImage ico_t;
    but_box->button(QDialogButtonBox::Ok)->setText(_("Ok"));
    if(!ico_t.load(TUIS::icoPath("button_ok").c_str())) ico_t.load(":/images/button_ok.png");
    but_box->button(QDialogButtonBox::Ok)->setIcon(QPixmap::fromImage(ico_t));
    connect(but_box, SIGNAL(accepted()), this, SLOT(accept()));
    but_box->button(QDialogButtonBox::Cancel)->setText(_("Cancel"));
    if(!ico_t.load(TUIS::icoPath("button_cancel").c_str())) ico_t.load(":/images/button_cancel.png");
    but_box->button(QDialogButtonBox::Cancel)->setIcon(QPixmap::fromImage(ico_t));
    connect(but_box, SIGNAL(rejected()), this, SLOT(reject()));
    dlg_lay->addWidget( but_box );

    connect(this, SIGNAL(finished(int)), this, SLOT(finish(int)));

    //- Fill users list -
    vector<string> u_list;
    SYS->security().at().usrList(u_list);
    for(int i_l = 0; i_l < u_list.size(); i_l++ )
    {
	string simg = TSYS::strDecode(SYS->security().at().usrAt(u_list[i_l]).at().picture(),TSYS::base64);
	QImage img;
	if( img.loadFromData((const uchar*)simg.c_str(),simg.size()) )
	    users->addItem(QPixmap::fromImage(img),u_list[i_l].c_str());
	else users->addItem(u_list[i_l].c_str());
    }
}

QString DlgUser::user()
{
    return users->currentText();
}

QString DlgUser::password()
{
    return passwd->text();
}

void DlgUser::finish( int result )
{
    if( result )
    {
	//- Check user -
	if( SYS->security().at().usrPresent(user().toAscii().data()) &&
		SYS->security().at().usrAt(user().toAscii().data()).at().auth(password().toAscii().data()) )
	    setResult(SelOK);
	else setResult(SelErr);
    }
    else setResult(SelCancel);
}

//*********************************************
//* UserStBar: Status bar user widget.        *
//*********************************************
UserStBar::UserStBar( const QString &iuser, QWidget * parent ) : QLabel(parent)
{
    setUser(iuser);
}

QString UserStBar::user( )
{
    return user_txt;
}

void UserStBar::setUser( const QString &val )
{
    setText(QString("<font color='%1'>%2</font>").arg((val=="root")?"red":"green").arg(val));
    user_txt = val;
}

bool UserStBar::event( QEvent *event )
{
    if( event->type() == QEvent::MouseButtonDblClick )  userSel();
    return QLabel::event( event );
}

bool UserStBar::userSel( )
{
    DlgUser d_usr(parentWidget());
    int rez = d_usr.exec();
    if( rez == DlgUser::SelOK && d_usr.user() != user() )
    {
	setUser( d_usr.user() );
	emit userChanged();
	return true;
    }
    else if( rez == DlgUser::SelErr )
        mod->postMess(mod->nodePath().c_str(),_("Auth is wrong!!!"),TUIMod::Warning,this);

    return false;
}

//*************************************************
//* TableDelegate: Combobox table delegate.       *
//*************************************************
TableDelegate::TableDelegate(QObject *parent) : QItemDelegate(parent)
{

}

void TableDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    drawFocus(painter,option,option.rect.adjusted(+1,+1,-1,-1));

    QVariant value = index.data(Qt::DisplayRole);
    switch(value.type())
    {
	case QVariant::Bool:
	    //painter->save();
	    if(value.toBool())
	    {
		QImage img(":/images/ok.png");
		painter->drawImage(option.rect.center().x()-img.width()/2,option.rect.center().y()-img.height()/2,img);
	    }
	    //painter->restore();
	    break;
	default:
	    drawDisplay(painter,option,option.rect,value.toString());
	    break;
    }
}

QWidget *TableDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QWidget *w_del;
    if(!index.isValid()) return 0;

    QVariant value = index.data(Qt::DisplayRole);
    QVariant val_user = index.data(Qt::UserRole);

    if(val_user.isValid()) w_del = new QComboBox(parent);
    else
    {
	QItemEditorFactory factory;
	w_del = factory.createEditor(value.type(), parent);
    }
    w_del->installEventFilter(const_cast<TableDelegate*>(this));
    return w_del;
}

void TableDelegate::setEditorData( QWidget *editor, const QModelIndex &index ) const
{
    QVariant value = index.data(Qt::DisplayRole);
    QVariant val_user = index.data(Qt::UserRole);

    if( dynamic_cast<QComboBox*>(editor) )
    {
	QComboBox *comb = dynamic_cast<QComboBox*>(editor);
	if(value.type() == QVariant::Bool)	comb->setCurrentIndex(value.toBool());
	else if(val_user.isValid())
	{
	    comb->addItems(val_user.toStringList());
	    comb->setCurrentIndex(comb->findText(value.toString()));
	}
	return;
    }
    if( dynamic_cast<QLineEdit*>(editor) )
    {
	QLineEdit *led = dynamic_cast<QLineEdit*>(editor);
	led->setText(value.toString());
	return;
    }
}

void TableDelegate::setModelData( QWidget *editor, QAbstractItemModel *model, const QModelIndex &index ) const
{
    if(dynamic_cast<QComboBox*>(editor))
    {
	QComboBox *comb = dynamic_cast<QComboBox*>(editor);
	QVariant val_user = index.data(Qt::UserRole);
	if(!val_user.isValid())
	    model->setData(index,(bool)comb->currentIndex(),Qt::EditRole);
	else model->setData(index,comb->currentText(),Qt::EditRole);
	return;
    }
    if(dynamic_cast<QLineEdit*>(editor))
    {
	QLineEdit *led = dynamic_cast<QLineEdit*>(editor);
	model->setData(index,led->text(),Qt::EditRole);
	return;
    }
}

void TableDelegate::updateEditorGeometry( QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex & ) const
{
    editor->setGeometry(option.rect);
}

bool TableDelegate::eventFilter( QObject *object, QEvent *event )
{
    if( dynamic_cast<QComboBox*>(object) )
    {
	QComboBox *comb = dynamic_cast<QComboBox*>(object);
	if( event->type() == QEvent::KeyRelease )
	    switch( static_cast<QKeyEvent *>(event)->key() )
	    {
		case Qt::Key_Enter:
		case Qt::Key_Return:
		    emit commitData(comb);
		    emit closeEditor(comb, QAbstractItemDelegate::SubmitModelCache);
		    return true;
		case Qt::Key_Escape:
		    emit closeEditor(comb, QAbstractItemDelegate::RevertModelCache);
		    return true;
		default:
		    return false;
	    }
    }

    return QItemDelegate::eventFilter(object,event);
}
