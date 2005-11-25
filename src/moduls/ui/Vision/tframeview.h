// file "tframeview.h"

/***************************************************************************
 *   Copyright (C) 2005 by Evgen Zaichuk                               
 *   evgen@diyaorg.dp.ua                                                     
 *                                                                         
 *   This program is free software; you can redistribute it and/or modify  
 *   it under the terms of the GNU General Public License as published by  
 *   the Free Software Foundation; either version 2 of the License, or     
 *   (at your option) any later version.                                   
 *                                                                         
 *   This program is distributed in the hope that it will be useful,       
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         
 *   GNU General Public License for more details.                          
 *                                                                         
 *   You should have received a copy of the GNU General Public License     
 *   along with this program; if not, write to the                         
 *   Free Software Foundation, Inc.,                                       
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             
 ***************************************************************************/
 
#ifndef TFRAMEVIEW
#define TFRAMEVIEW

#include <qcanvas.h> 
#include <list.h> 
#include <vector.h> 
#include <qxml.h> 
#include <qtimer.h>

#include "titem.h"


namespace VISION
{

typedef list<TItem *> itemList;
typedef vector<TItem *> itemVector;

//����� "������������� �����"
class TFrameView : public QCanvasView
{ 
  Q_OBJECT 
  
public: 
  TFrameView(QCanvas*, TVISIONMode, QWidget* parent=0, const char* name=0, WFlags f=0);
  
  TItem* addItem(int itemRtti, int x, int y, bool toSelect = true);
  void setInsertingNewItem(bool inserting); //����� ���������� ������ ��������
  
  void clear();

  bool delSelectedItems();
  bool delItem(TItem *);
  void selectAll(); //�������� ��� ��������
  
  void moveBySelected(const int dx, const int dy);
  
  int getCountSelected(itemList * = NULL); //����� ���������� ���������, 
                                           // ���������� �������� ���������� � ������ itemlist
  int getItems(itemList * = NULL); //����� ���������, �������� ���������� � ������ itemlist
  
  QString getName() const;
  bool setName(const QString &name);
  //������ �� ���������� �����:
  TListOfString getPropCategories(); //������ �������� ��������� ������� ��� �����;
  TListOfString getPropNames(const QString *category); //������ �������� ������� � ��������� category ��� �����;
  TPropType getPropType(const QString *propertyName); //��� �������� propertyName;
  TListOfString getPropEnumTypeValues(const QString *propertyName); //������ ��������� �������� 
                                                                    //��� �������� ������������� ����;
  QString getPropValue(const QString *propertyName); //�������� ��������;
  bool setPropValue(const QString *propertyName, const QString *newValue); //��������� �������� ��������;
     
  void startDynamic(); //������ �������� (������� ��� ���������� (�����������) ���������)
  void stopDynamic(); //������� �������� (������� ��� ���������� (�����������) ���������)
signals:
  void addItem(int x, int y);
  void selectingItem();
  void movingItem();
  void resizingItem();
  
  void mouseMove(int x, int y);
  void mousePress(int x, int y);
  
protected:
  void contentsMousePressEvent(QMouseEvent*);
  void contentsMouseMoveEvent(QMouseEvent*);
  void contentsMouseReleaseEvent(QMouseEvent*);
  itemList collisions (const int x, const int y) const; //���������� ������ ���������, 
                                                        // �������������� � ������ (x, y). 
							// ������ ������������ �� z-���������� ��������
  void unselectAll(); //����� ��������� ���� ���������
  bool contains(itemList *, TItem *); //�������� �� ������ itemList ������� TItem?

private slots:
  void timerDynamic(); //��������� ������� ������� ���������� ��������

private:
  friend class TSaxHandler;
  QTimer timer;
  TVISIONMode mode;
  
  double z; //������� ������������ ���������� z
  
  QString name;
  int updateCycle; //������ ���������� (� ��)
  double rotate;
  double scaleX;
  double scaleY;
   
  QCanvasItem* itemToMove;
  QPoint movingStart;
  itemList itemsList;
  
  //��������� ������� ����:
  bool moving; //� ������ ������ ������������ ����������� �������� (��);
  bool resizing; //� ������ ������ ������������ ��������� ������� ��������;
  QPoint pointStart; //��������� ����� ��������� ���� ��� ��������������/��������� �������;
  char resizeItem; //����������� ��������� �������� ��������;
  bool insertingNewItem;
};

//------------------------������ � ������-----------------------------------
class TSaxHandler : public QXmlDefaultHandler 
{ 
public: 
  TSaxHandler(TFrameView *frame); 
  
  bool startElement(const QString &namespaceURI, 
                    const QString &localName, 
                    const QString &qName, 
                    const QXmlAttributes &attribs); 
  bool endElement(const QString &namespaceURI, 
                  const QString &localName, 
                  const QString &qName); 
  bool characters(const QString &str); 
  bool fatalError(const QXmlParseException &exception); 
  
private:
  QString currentProp;
  TItem *currentItem;
  TFrameView *frame;
  bool valueWas;
  bool dynamicWas;
  bool addingItems;
  bool addingFrame;
};

}
#endif
