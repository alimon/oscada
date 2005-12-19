// file "titem.h"

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

#ifndef TITEM
#define TITEM

#include <list>
#include <stack>

#include <qcanvas.h>
#include <qstring.h>
#include <qimage.h>
#include <qfile.h>

#include "xpm/tcrectangle.xpm"
#include "xpm/tcline.xpm"
#include "xpm/tctext.xpm"
#include "xpm/tcimage.xpm"

using std::list;
using std::stack;

namespace VISION
{
//��� �������� ��������:
typedef enum {unknownType, intType, floatType, stringType, enumType, colorType} TPropType;
typedef enum {Development, Runtime} TVISIONMode;
typedef list<QString> TListOfString;

//����������� ��������� �������� ��������:
enum {resizeNothing, resizeTop, resizeTopRight, resizeRight, resizeBottomRight, 
      resizeBottom, resizeBottomLeft, resizeLeft, resizeTopLeft};

//�������, �������� ��� �������� �����������:
typedef enum {ta_unknown, ta_eps, ta_value, ta_enum, ta_proportion, ta_name, ta_string, ta_real, ta_uint, 
              ta_format, ta_p, ta_n, ta_v, ta_o, ta_c, 
	      na_S, na_V, na_E, na_P, na_Format, na_EnumPair, na_P1, na_P2, na_P3, na_ParamOrValue, 
	      na_ParamOrValue2, na_Param_name_V, na_Param_name_E, na_Param_name_P3, na_Attr_name_V, 
	      na_Attr_name_E, na_Attr_name_P, na_Attr_name_P2, na_Attr_name_P3,
	      A_V_1, A_V_2, A_V_3, A_V_4, A_E_1, A_E_2, A_E_3, A_E_4,  A_P_1, A_P_2, A_P_3, A_P_4, A_P_5, 
	      A_P_6, A_P_7, A_P_8, A_P_9, A_P_10, A_P_11, A_P_12, A_V, A_E, A_P} TVocabulary;
	      
//��� ����������:
typedef enum{noDType, valueDType, enumDType, proportionDType} TDynamicType;

//������ ������:
#define epsSymbol "#"

//���� ������������� ��������:
typedef stack<TVocabulary> TStack;

//�������� �������� ���� "��������":
struct TDynamicTypeValue
{
   QString paramName;
   QString attrName;
   char format;
   int precision;
};

//�������� �������� ���� "������������":
struct TDynamicTypeEnumElem
{
   QString paramName;
   QString attrName;
   QString value;
};

typedef list<TDynamicTypeEnumElem> TDynamicTypeEnumList;

struct TDynamicTypeEnum
{
   TDynamicTypeEnumList dynamicEnumElem;
   QString valueDefault;
};

//�������� �������� ���� "���������":
struct TDynamicTypeProportion
{
   QString paramName0;
   QString attrName0;
   double val0;
   int size0;
   bool yesSize0;
   QString paramName100;
   QString attrName100;
   double val100;
   int size100;
   bool yesSize100;
   QString paramNameX;
   QString attrNameX;
};

class TDBGW;
class TItem;

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------����� ����������� ������� ��--------------------------------------------
class TDynamicProp
{
public:
   TDynamicProp(TItem *);
   
   //��������� ������������ ���������� �������� �������� text �������� propName
   bool isValid(const QString &text, const QString &propName); 
   
   //��� ��������������� ��������
   QString getPropName() const {return QString(propName);}; 
   
    //��������� �������� ��������
   QString getPropText() const {return QString(propText);};
   bool setPropText(const QString &text, const QString &name);
   
   bool doDynamic(); //�������� �������� propName �������� item
   TDynamicType getDynamicType();
private:
   TItem *item; //������� TItem
   QString propName; //��� ��������������� �������� �������� TItem
   QString propText; //��������� �������� ��������
   
   TDynamicType dynamicType; //������ �����������
   
   bool getPropValue(QString &value) const; //�������� �������� ��������������� ��������
   
   TDynamicTypeValue dynamicValue;
   TDynamicTypeEnum dynamicEnum;
   TDynamicTypeProportion dynamicProportion;
   void clearStructs();
   //��������� ������:
   bool isitName(const QString &s, const int begin, int &end);
   bool isitString(const QString &s, const int begin, int &end);
   bool isitInt(const QString &s, const int begin, int &end);
   bool isitDecimal(const QString &s, const int begin, int &end);
   bool isitReal(const QString &s, const int begin, int &end);
   bool isitUInt(const QString &s, const int begin, int &end);
   bool isitFormat(const QString &s, const int begin, int &end);
   bool isitValue(const QString &s, const int begin, int &end);
   bool isitEnum(const QString &s, const int begin, int &end);
   bool isitProportion(const QString &s, const int begin, int &end);
   bool isitPoint(const QString &s, const int begin, int &end);
   bool isitN(const QString &s, const int begin, int &end);//";"
   bool isitV(const QString &s, const int begin, int &end);//"::"
   bool isitOpenScope(const QString &s, const int begin, int &end);
   bool isitCloseScope(const QString &s, const int begin, int &end);
   bool isitEps(const QString &s, const int begin, int &end);
   //�������� ��������� �������:
   TVocabulary getLexeme(const QString &s, int begin, int &end, QString &lexemeValue);
   //�������������� ������:
   bool parse(QString s);
   bool parseDoActions(TStack &st, const QString &lexemeValue); //���������� ��������, ���������� � ����������
   bool parseDoStack(TStack &st, const TVocabulary lexeme); //���������� ������ ���������� (���������� �����)
};
//------------------------------------------------------------------------------------------------------------
typedef list<TDynamicProp> TListDynamicProp;

//------------------------------------------------------------------------------------------------------------
//----------------------------------������� ����� ��----------------------------------------------------------
class TItem
{ 
public: 
  TItem(TVISIONMode m);
  virtual ~TItem();

  virtual int rtti() const = 0;
  
  //��������� ��������:
  bool isVisibleProp() const;
  virtual void setVisibleProp (bool yes) = 0;
  
  //������ �� ���������� ��������:
  //������ �������� ��������� ������� ��� ��������:
  virtual TListOfString getPropCategories() = 0;
  //������ �������� ������� � ��������� category:
  virtual TListOfString getPropNames(const QString *category) = 0;
  //��� �������� propertyName:
  virtual TPropType getPropType(const QString *propertyName) = 0; 
  //������ ��������� �������� ��� �������� ������������� ����:
  virtual TListOfString getPropEnumTypeValues(const QString *propertyName) = 0; 
  //�������� ��������:
  virtual QString getPropValue(const QString &propertyName) = 0; 
  //��������� �������� ��������:
  virtual bool setPropValue(const QString &propertyName, const QString &newValue, const bool doIt = true) = 0;
  
  //������ � ������� ��������, ������������:
  virtual bool isSelectedItem() = 0;
  virtual void setSelectedItem (bool yes) = 0;
  virtual double zItem() const = 0;
  virtual void setZItem(double z) = 0;
  virtual void showItem() = 0;
  virtual void moveByItem( double dx, double dy ) = 0;  
  
  //��������� ������� ���� (��������� ���� ������� � ����������� �� ��� ��������� ������������ ��������,
  //                              ��������� �������� �������� � ������� ����, ����������� �������� �����):
  //��������� ������� cursor (resizeItem) �� ��������������, ���� ������ ��������� � �������� 
  // ������� �������� (x, y - ��������� �������):
  virtual bool getCursorShape(int x, int y, char &resizeItem, int &cursor, int precision = 4) = 0;
  //��������� �������� �������� ����� (true - ���� ��������� ���������, false - ���� ��� 
  // (�-�, �������� ������������ ��������)):
  virtual bool resizeOnMouse(int x, int y, int prevX, int prevY, char resizeItem) = 0;
  //�������������� �������� ����� (true - ���� ��������� ���������, false - ���� ��� ):
  virtual bool moveOnMouse(int x, int y, int prevX, int prevY) = 0;
  
  virtual bool saveToFile(QFile &);
  
  //���������/��������� ����������� �������� propName:
  bool setDynamic(const QString &propName, const QString &text);
  //����������� �������� propName:
  bool getDynamic(const QString &propName, QString &text);
  //����������� (Runtime):
  bool doDynamic();
protected: 
   QString name; //��� ��������
   TVISIONMode mode; //����� ������
   bool visibleItem; //� ������ Development ������� ��� ����� ����������, 
                     //� � ������ Runtime - � ����������� �� �������� ����� ��������;
    
    TListDynamicProp listDynamicProp;//������ ���������������� �������
private:

};
//----------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------
//---------------------------------------����� �� "�������������"-------------------------------------------
class TCRectangle : public TItem, public QCanvasRectangle
{ 
public: 
  TCRectangle(QCanvas *canvas, TVISIONMode m, double z, int x, int y, 
                                              int width = 40, int height = 30, void *f = NULL);
  virtual ~TCRectangle();
  
  static int RTTI;
  static QString itemName() {return "Rectangle";};
  static QImage itemImage() {return QImage(tcrectangle);};
  
  virtual QPointArray areaPoints () const;
  virtual int rtti() const;
     
  //��������� ���������:
  virtual void setVisibleProp (bool yes);

  //������ � ������� ��������, ������������:
  virtual void setSelectedItem (bool yes);
  virtual void showItem();
  virtual bool isSelectedItem();
  virtual double zItem() const;
  virtual void setZItem(double z);
  virtual void moveByItem( double dx, double dy );
 
  //������ �� ���������� ��������:
  //������ �������� ��������� ������� ��� ��������:
  virtual TListOfString getPropCategories();
  //������ �������� ������� � ��������� category ��� ��������:
  virtual TListOfString getPropNames(const QString *category); 
  //��� �������� propertyName:
  virtual TPropType getPropType(const QString *propertyName); 
  //������ ��������� �������� ��� �������� ������������� ����:
  virtual TListOfString getPropEnumTypeValues(const QString *propertyName); 
  //�������� ��������:
  virtual QString getPropValue(const QString &propertyName); 
  //��������� �������� ��������:
  virtual bool setPropValue(const QString &propertyName, const QString &newValue, const bool doIt = true); 
  
  //��������� ������� ���� (��������� ���� ������� � ����������� �� ��� ��������� ������������ ��������,
  //                        ��������� �������� �������� � ������� ����, ����������� �������� �����):
  //��������� ������� cursor (resizeItem) �� ��������������, ���� ������ ��������� � �������� ������� �������� 
  // (x, y - ��������� �������):
  virtual bool getCursorShape(int x, int y, char &resizeItem, int &cursor, int precision = 4);
  //��������� �������� �������� ����� (true - ���� ��������� ���������, false - ���� ��� 
  // (�-�, �������� ������������ ��������)):
  virtual bool resizeOnMouse(int x, int y, int prevX, int prevY, char resizeItem);
  //�������������� �������� ����� (true - ���� ��������� ���������, false - ���� ��� ):
  virtual bool moveOnMouse(int x, int y, int prevX, int prevY);
  
protected: 
   virtual void drawShape ( QPainter & p );
  
private:  

};
//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------
//------------------------------------����� �� "�����"--------------------------------------------------------
class TCLine : public TItem, public QCanvasLine
{ 
public: 
  TCLine(QCanvas *canvas, TVISIONMode m, double z, int x, int y, int width = 40, int height = 30, void *f = NULL);
  virtual ~TCLine();
  
  static int RTTI;
  static QString itemName() {return "Line";};
  static QImage itemImage() {return QImage(tcline);};
  
  virtual QPointArray areaPoints () const;
  virtual int rtti() const;
  
  //��������� ���������:
  virtual void setVisibleProp (bool yes);
  
  virtual double zItem() const;
  virtual void setZItem(double z);
  
  void setSize(int width, int height);
  int width() const;
  int height() const;

  //������ � ������� ��������, ������������:
  virtual void setSelectedItem (bool yes);
  virtual void showItem();
  virtual bool isSelectedItem();
  virtual void moveByItem(double dx, double dy);
  virtual void moveBy(double dx, double dy);
 
  //������ �� ���������� ��������:
  //������ �������� ��������� ������� ��� ��������:
  virtual TListOfString getPropCategories(); 
  //������ �������� ������� � ��������� category ��� ��������:
  virtual TListOfString getPropNames(const QString *category); 
  //��� �������� propertyName:
  virtual TPropType getPropType(const QString *propertyName); 
  //������ ��������� �������� ��� �������� ������������� ����:
  virtual TListOfString getPropEnumTypeValues(const QString *propertyName); 
  //�������� ��������:
  virtual QString getPropValue(const QString &propertyName); 
  //��������� �������� ��������:
  virtual bool setPropValue(const QString &propertyName, const QString &newValue, const bool doIt = true); 
  
  //��������� ������� ���� (��������� ���� ������� � ����������� �� ��� ��������� ������������ ��������,
  //                        ��������� �������� �������� � ������� ����, ����������� �������� �����):
  // P.S. � QCanvasLine ��������� ����� ���������� ����� ����� ������ �������� � ���������� ���������� �����.
  //      ����� moveBy �������� ���������� ������ ��������, � �� ����� ��� ��� ��������� - ���������� ����� (�����)
  //��������� ������� cursor (resizeItem) �� ��������������, ���� ������ ��������� � �������� ������� �������� 
  // (x, y - ��������� �������):
  virtual bool getCursorShape(int x, int y, char &resizeItem, int &cursor, int precision = 4);
  //��������� �������� �������� ����� (true - ���� ��������� ���������, false - ���� ��� 
  // (�-�, �������� ������������ ��������)):
  virtual bool resizeOnMouse(int x, int y, int prevX, int prevY, char resizeItem);
  //�������������� �������� ����� (true - ���� ��������� ���������, false - ���� ��� ):
  virtual bool moveOnMouse(int x, int y, int prevX, int prevY);

protected: 
   virtual void drawShape ( QPainter & p ); //��. P.S.
   int x1, x2, y1, y2;
  
private:  

};
//-----------------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------------
//------------------------------����� �� "�����"-------------------------------------------------------------
class TCText : public TItem, public QCanvasText
{ 
public: 
  TCText(QCanvas *canvas, TVISIONMode m, double z, int x, int y, const QString& text, void *f = NULL);
  virtual ~TCText();
  
  static int RTTI;
  static QString itemName() {return "Text";};
  static QImage itemImage() {return QImage(tctext);};

  virtual int rtti() const;
  
  //��������� ���������:
  virtual void setVisibleProp (bool yes);

  //������ � ������� ��������, ������������:
  virtual void setSelectedItem (bool yes);
  virtual void showItem();
  virtual bool isSelectedItem();
  virtual double zItem() const;
  virtual void setZItem(double z);
  virtual void moveByItem( double dx, double dy );
 
  //������ �� ���������� ��������:
  //������ �������� ��������� ������� ��� ��������:
  virtual TListOfString getPropCategories(); 
  //������ �������� ������� � ��������� category ��� ��������:
  virtual TListOfString getPropNames(const QString *category); 
  //��� �������� propertyName:
  virtual TPropType getPropType(const QString *propertyName); 
  //������ ��������� �������� ��� �������� ������������� ����:
  virtual TListOfString getPropEnumTypeValues(const QString *propertyName); 
  //�������� ��������:
  virtual QString getPropValue(const QString &propertyName); 
  //��������� �������� ��������:
  virtual bool setPropValue(const QString &propertyName, const QString &newValue, const bool doIt = true); 
  
  //��������� ������� ���� (��������� ���� ������� � ����������� �� ��� ��������� ������������ ��������,
  //                        ��������� �������� �������� � ������� ����, ����������� �������� �����):
  //��������� ������� cursor (resizeItem) �� ��������������, ���� ������ ��������� � �������� ������� �������� 
  // (x, y - ��������� �������):
  virtual bool getCursorShape(int x, int y, char &resizeItem, int &cursor, int precision = 4);
  //��������� �������� �������� ����� (true - ���� ��������� ���������, false - ���� ��� 
  // (�-�, �������� ������������ ��������)):
  virtual bool resizeOnMouse(int x, int y, int prevX, int prevY, char resizeItem);
  //�������������� �������� ����� (true - ���� ��������� ���������, false - ���� ��� ):
  virtual bool moveOnMouse(int x, int y, int prevX, int prevY);
  
protected: 
   virtual void draw( QPainter & p );
  
private:  

};
//-----------------------------------------------------------------------------------------------------------


//-----------------------------------------------------------------------------------------------------------
//----------------------------����� �� "�����������"---------------------------------------------------------
class TCImage : public TItem, public QCanvasRectangle
{ 
public: 
  TCImage(QCanvas *canvas, TVISIONMode m, double z, int x, int y, void *f = NULL);
  virtual ~TCImage();
  
  static int RTTI;
  static QString itemName() {return "Image";};
  static QImage itemImage() {return QImage(tcimage);};

  virtual int rtti() const;
  
  //��������� ���������:
  virtual void setVisibleProp (bool yes);

  //������ � ������� ��������, ������������:
  virtual void setSelectedItem (bool yes);
  virtual void showItem();
  virtual bool isSelectedItem();
  virtual double zItem() const;
  virtual void setZItem(double z);
  virtual void moveByItem( double dx, double dy );
 
  //������ �� ���������� ��������:
  //������ �������� ��������� ������� ��� ��������:
  virtual TListOfString getPropCategories(); 
  //������ �������� ������� � ��������� category ��� ��������:
  virtual TListOfString getPropNames(const QString *category);
  //��� �������� propertyName: 
  virtual TPropType getPropType(const QString *propertyName); 
  //������ ��������� �������� ��� �������� ������������� ����:
  virtual TListOfString getPropEnumTypeValues(const QString *propertyName); 
  //�������� ��������;
  virtual QString getPropValue(const QString &propertyName); 
  //��������� �������� ��������:
  virtual bool setPropValue(const QString &propertyName, const QString &newValue, const bool doIt = true); 
  
  //��������� ������� ���� (��������� ���� ������� � ����������� �� ��� ��������� ������������ ��������,
  //                        ��������� �������� �������� � ������� ����, ����������� �������� �����):
  //��������� ������� cursor (resizeItem) �� ��������������, ���� ������ ��������� � �������� ������� 
  // �������� (x, y - ��������� �������):
  virtual bool getCursorShape(int x, int y, char &resizeItem, int &cursor, int precision = 4);
  //��������� �������� �������� ����� (true - ���� ��������� ���������, false - ���� ��� 
  // (�-�, �������� ������������ ��������)):
  virtual bool resizeOnMouse(int x, int y, int prevX, int prevY, char resizeItem);
  //�������������� �������� ����� (true - ���� ��������� ���������, false - ���� ��� ):
  virtual bool moveOnMouse(int x, int y, int prevX, int prevY);
  
protected: 
   virtual void draw( QPainter & p );
  
private:  
   QImage image;
   QString fileName;
};
//----------------------------------------------------------------------------------------------------------

}
#endif
