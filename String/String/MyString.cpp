#include <iostream>
#include <string.h>
using namespace std;
class MyString
{
  char *a;
public:
	MyString::MyString ()
{
  a=NULL;
}
MyString::MyString (char*ch)
{
  int len=strlen(ch)+1;
  a=new char[len];
  strcpy_s(a,len,ch);
}

	MyString::~MyString()
{
	cout<<"����"<<endl;
   if(a!=NULL)
   {
     delete[]a;
	 a=NULL;
   }
}
MyString::MyString(const MyString&n)
{
  int len =strlen(n.a)+1;
  a=new char[len];
  strcpy_s (a,len,n.a);

}
void show()
	{
	    cout<<a<<endl;
	}
MyString operator +(const MyString &a)
{
  if(this->a!=NULL)
   {
   int len;
   char*p=this->a;
   len=strlen(a.a)+strlen(p)+1;
   this->a=new char[len];
   strcpy_s(this->a,len,p);
   strcat_s(this->a,len,a.a);
   delete p;
   }	
 else
	{
	  int len=strlen(a.a)+1;
	  this->a=new char[len];
	  strcpy_s(this->a,len,a.a);
	}
  return *this;
}
 MyString &operator =(const MyString &s)
 {
       int len=strlen(s.a)+1;
	  this->a=new char[len];
	  strcpy_s(this->a,len,s.a);
	  return *this;
 }
 void cmp(MyString &m)
 {
	 if(strcmp(this->a,m.a)==0)
	 {
	   cout<<"��ͬ"<<endl;
	 }	
     else
	{
	   cout<<"��ͬ"<<endl;
	}
 }
 friend ostream  &operator <<(ostream &ct,MyString &q);
 friend istream  &operator >>(istream &ci,MyString &p);
};
ostream  &operator <<(ostream &ct,MyString &q)
{
   ct<<q.a<<"!"<<endl;
   return ct;
}
istream  &operator >>(istream &ci,MyString &p)
{
	ci>>p.a;
	return ci;
}

void main()
{
	MyString a("holle");//1
	MyString c("holle");
	MyString b(" world");
	MyString e;//8
	MyString d=a;//2
	d.show();
	a.cmp(c);//3
	a.cmp(b);
	a.show();
	cout<<a+b<<endl;//4
	a.show();
	cout<<"������һ���ַ�����"<<endl;
	cin>>a;//5
	a.show();
	a=b;//6
	e=c+" world";
	e.show();
   (d+b).show();//7
   while(1);
}