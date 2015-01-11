
#include <iostream>
#include "SlimXml.h"
using namespace std;
using namespace slim;

int main()
{
	XmlDocument xml;

	xml.loadFromFile(L"C:\\Users\\909071\\Desktop\\XmlValidator\\config.xml");

	int i = xml.getChildCount();

	cout << "root node count:" << xml.getChildCount() << endl;

	NodeIterator it1;
	
	XmlNode *node1 = xml.getFirstChild(it1);

	wcout << L"1st layer node name:" << node1->getName() << endl;

	NodeIterator it2;

	XmlNode *node2;

	node2= node1->getFirstChild(it2);

	wcout << L"2st layer node name:" << node2->getString()<< endl;

	system("pause");
	return 0;
}