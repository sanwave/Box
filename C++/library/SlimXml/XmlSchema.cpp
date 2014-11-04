#include "XmlSchema.h"
#include <cassert>
#include <fstream>

namespace slim
{

const Char* ATTR_MULTIPLE = T("multiple");
const Char* ATTR_RECURSIVE = T("recursive");
const Char* ATTR_ATTRIBUTE = T("attribute");
const Char* ATTR_TYPE = T("type");
const Char* ATTR_DEFAULT = T("default");

#define LEFT_QUOTE L"L\""


///////////////////////////////////////////////////////////////////////////////////////////////////
bool XmlSchema::constructFromXml(XmlDocument* file)
{
	clearChild();

	XmlNode* comment = addChild(T("xml data schema"), COMMENT);
	return parseNodeStruct(this, file);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//parse data schema from node
//src		data node
//dst		schema node
bool XmlSchema::parseNodeStruct(XmlNode* dst, XmlNode* src)
{
	assert(dst != NULL);
	assert(src != NULL);

	NodeIterator nodeIterator;
	AttributeIterator attriIterator;

	for (XmlAttribute* attribute = src->getFirstAttribute(attriIterator);
		attribute != NULL;
		attribute = src->getNextAttribute(attriIterator))
	{
		XmlNode* structure = dst->findChild(attribute->getName());
		if (structure == NULL)
		{
			//first time show up
			structure = dst->addChild(attribute->getName());
			structure->addAttribute(ATTR_TYPE, guessType(attribute->getString()));
			structure->addAttribute(ATTR_ATTRIBUTE, T("true"));
		}
	}

	for (XmlNode* child = src->getFirstChild(nodeIterator);
		  child != NULL;
		  child = src->getNextChild(nodeIterator))
	{
		if (child->getType() != ELEMENT)
		{
			continue;
		}
		XmlNode* structure = dst->findChild(child->getName());
		if (structure == NULL)
		{
			//first time show up
			bool recursive = false;
			const XmlNode* parent = dst;
			while (parent != NULL)
			{
				if (Strcmp(parent->getName(), child->getName()) == 0)
				{
					recursive = true;
					break;
				}
				parent = parent->getParent();
			}
			structure = dst->addChild(child->getName());
			if (recursive)
			{
				structure->addAttribute(ATTR_RECURSIVE, T("true"));
			}
			else if (!child->hasChild() && !child->hasAttribute())
			{
				//simple type, must have a type attribute
				structure->addAttribute(ATTR_TYPE, guessType(child->getString()));
			}
		}
		else if (structure->findAttribute(ATTR_ATTRIBUTE) != NULL)
		{
			//child and attribute can't have same name
			return false;
		}

		XmlAttribute* multiple = structure->findAttribute(ATTR_MULTIPLE);
		if (multiple == NULL || !multiple->getBool())
		{
			NodeIterator iter;
			if (src->findFirstChild(child->getName(), iter) != NULL
				&& src->findNextChild(child->getName(), iter) != NULL)
			{
				if (multiple == NULL)
				{
					multiple = structure->addAttribute(ATTR_MULTIPLE);
				}
				multiple->setBool(true);
			}
		}

		if (!structure->findAttribute(ATTR_RECURSIVE) && (child->hasChild() || child->hasAttribute()))
		{
			parseNodeStruct(structure, child);
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
const Char* XmlSchema::guessType(const Char* content) const
{
	assert(content != NULL);

	if (Strcmp(content, T("true")) == 0
		|| Strcmp(content, T("false")) == 0)
	{
		return T("bool");
	}

	bool foundDot = false;
	bool foundSign = false;
	bool foundLeftBrack = false;
	if (*content == 0)
	{
		return T("string");
	}
	Char first = *content;
	int numberCount = 1;
	if (first == T('('))
	{		
		content++;
		foundLeftBrack = true;
	}
	for (; *content != 0; ++content)
	{
		if ((*content >= T('0') && *content <= T('9')))
		{
			continue;
		}
		if (*content == T('.'))
		{
			if (foundDot) 
			{
				return T("string");
			}
			foundDot = true;
		} 
		else if (*content == T('-'))
		{
			if (foundSign)
			{
				return T("string");
			}
			foundSign = true;
		}
		else if (*content == T(','))
		{
			numberCount++;
			foundSign = false;
			foundDot = false;			
		}
		else if (*content == T(')'))
		{
			if (foundLeftBrack)
			{
				break;
			}
			return T("string");
		}
		else
		{
			return T("string");
		}
	}
	
	switch(numberCount)
	{
	case 1 : 
		if (!foundDot)
		{
			return T("int");
		}
		return T("float");
	}
	return T("string");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool XmlSchema::generateCodeForNode(const XmlNode* node, String& headerCode, String& sourceCode) const
{
	assert(node != NULL);

	if (node->getType() == ELEMENT)
	{
		String structDefinition;
		structDefinition += T("///////////////////////////////////////////////////////////////////////////////////////////////////\r\n");
		structDefinition += T("struct ");
		structDefinition += node->getName();
		structDefinition += T("\r\n{\r\n");
		//constructor
		int index = 0;
		NodeIterator iter;
		for (XmlNode* child = node->getFirstChild(iter);
			  child != NULL;
			  child = node->getNextChild(iter))
		{
			if (child->isEmpty() && child->getType() == ELEMENT)
			{
				//simple type
				addConstructorItem(child, structDefinition, index);
			}
		}
		if (index > 0)
		{
			structDefinition += T("	{\r\n	}\r\n");
		}
		//destructor
		for (XmlNode* child = node->getFirstChild(iter);
			child != NULL;
			child = node->getNextChild(iter))
		{
			if (child->findAttribute(ATTR_RECURSIVE) != NULL
				&& child->findAttribute(ATTR_MULTIPLE) == NULL)
			{
				//recursive pointer, need to delete
				structDefinition += T("	~");
				structDefinition += node->getName();
				structDefinition += T("()\r\n	{\r\n		if (Child != NULL)\r\n		{\r\n			delete Child;\r\n");
				structDefinition += T("			Child = NULL;\r\n		}\r\n	}\r\n");
				break;	//can't have more than one
			}
		}

		structDefinition += T("	void read(const slim::XmlNode* node);\r\n	void write(slim::XmlNode* node) const;\r\n\r\n");

		String readingCode;
		readingCode += T("///////////////////////////////////////////////////////////////////////////////////////////////////\r\n");
		readingCode += T("void ");
		readingCode += node->getName();
		readingCode += T("::read(const XmlNode* node)\r\n{\r\n	assert(node != NULL);\r\n");
		readingCode += T("\r\n	NodeIterator iter;\r\n	const XmlNode* childNode = NULL;\r\n	const XmlAttribute* attribute = NULL;\r\n");

		String writingCode;
		writingCode += T("///////////////////////////////////////////////////////////////////////////////////////////////////\r\n");
		writingCode += T("void ");
		writingCode += node->getName();
		writingCode += T("::write(XmlNode* node) const\r\n{\r\n	assert(node != NULL);\r\n\r\n	node->clearChild();\r\n	node->clearAttribute();");
		writingCode += T("\r\n\r\n	XmlNode* childNode = NULL;\r\n	XmlAttribute* attribute = NULL;\r\n");

		size_t typeWidth = getNodeMemberTypeWidth(node);

		for (const XmlNode* child = node->getFirstChild(iter);
			  child != NULL;
			  child = node->getNextChild(iter))
		{
			if (child->getType() != ELEMENT)
			{
				continue;
			}
			XmlAttribute* multiple = child->findAttribute(ATTR_MULTIPLE);
			bool recursive = (child->findAttribute(ATTR_RECURSIVE) != NULL);
			if (child->isEmpty() && !recursive)
			{
				//simple type
				if (multiple != NULL && multiple->getBool())
				{
					addSimpleVector(child, structDefinition, typeWidth, readingCode, writingCode);
				}
				else
				{
					addSimpleMember(child, structDefinition, typeWidth, readingCode, writingCode);
				}
			}
			else
			{
				//struct type
				if (multiple != NULL && multiple->getBool())
				{
					addStructVector(child, structDefinition, typeWidth, readingCode, writingCode);
				}
				else
				{
					addStructMember(child, structDefinition, typeWidth, readingCode, writingCode);
				}
			}
		}
		structDefinition += T("};\r\n\r\n");

		readingCode += T("}\r\n\r\n");
		writingCode += T("}\r\n\r\n");

		sourceCode += readingCode;
		sourceCode += writingCode;

		//add to front
		headerCode = structDefinition + headerCode;
	}

	NodeIterator iter;
	for (const XmlNode* child = node->getFirstChild(iter);
		  child != NULL;
		  child = node->getNextChild(iter))
	{
		if (child->hasChild())
		{
			if (!generateCodeForNode(child, headerCode, sourceCode))
			{
				return false;
			}
		}
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
size_t XmlSchema::getNodeMemberTypeWidth(const XmlNode* node) const
{
	size_t maxWidth = 0;
	size_t width = 0;
	NodeIterator iter;
	for (const XmlNode* child = node->getFirstChild(iter);
		child != NULL;
		child = node->getNextChild(iter))
	{
		XmlAttribute* multiple = child->findAttribute(ATTR_MULTIPLE);

		if (child->isEmpty())
		{
			//simple type
			XmlAttribute* type = child->findAttribute(ATTR_TYPE);
			if (type == NULL)
			{
				continue;
			}
			width = getSimpleTypeString(type).size();
			if (multiple != NULL && multiple->getBool())
			{
				width += Strlen(T("std::vector<>"));
			}
		}
		else
		{
			//struct type
			width = Strlen(child->getName());
			if (multiple != NULL && multiple->getBool())
			{
				width += Strlen(T("std::vector<>"));
			}
		}
		if (width > maxWidth)
		{
			maxWidth = width;
		}
	}
	return maxWidth;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void XmlSchema::addSimpleMember(const XmlNode* child, String& structDefinition, size_t typeWidth,
								 String& readingFunction, String& writingFunction) const
{
	assert(child != NULL);

	XmlAttribute* type = child->findAttribute(ATTR_TYPE);
	XmlAttribute* defaultAttribute = child->findAttribute(ATTR_DEFAULT);
	bool inAttribute = (child->findAttribute(ATTR_ATTRIBUTE) != NULL);
	if (type == NULL)
	{
		return;
	}
	String typeString = getSimpleTypeString(type);
	size_t thisWidth = typeString.size();
	assert(thisWidth < typeWidth + 1);
	for (size_t i = 0; i < typeWidth + 1 - thisWidth; ++i)
	{
		typeString += T(" ");
	}
	
	structDefinition += T("	");
	structDefinition += typeString;
	structDefinition += child->getName();
	structDefinition += T(";\r\n");	

	readingFunction += inAttribute ? T("\r\n	attribute = node->findAttribute(")
									: T("\r\n	childNode = node->findChild(");
	readingFunction += LEFT_QUOTE;
	readingFunction += child->getName();
	readingFunction += inAttribute ? T("\");\r\n	if (attribute != NULL)\r\n	{\r\n		")
									: T("\");\r\n	if (childNode != NULL)\r\n	{\r\n		");
	readingFunction += child->getName();
	readingFunction += inAttribute ? T(" = attribute->get")
									: T(" = childNode->get");
	String typeName = type->getString();
	typeName[0] -= 32;
	readingFunction += typeName;
	readingFunction += T("();\r\n	}\r\n");

	writingFunction += T("\r\n	if (");
	writingFunction += child->getName();
	writingFunction += T(" != ");

	if (defaultAttribute != NULL)
	{
		//with initialized value
		if (Strcmp(type->getString(), T("string")) == 0)
		{
			writingFunction += LEFT_QUOTE;
		}
		writingFunction += defaultAttribute->getString();
		if (Strcmp(type->getString(), T("string")) == 0)
		{
			writingFunction += T("\"");
		}
	}
	else
	{
		writingFunction += getTypeDefaultValue(type->getString());
	}

	writingFunction += inAttribute ? T(")\r\n	{\r\n		attribute = node->addAttribute(")
									: T(")\r\n	{\r\n		childNode = node->addChild(");
	writingFunction += LEFT_QUOTE;
	writingFunction += child->getName();
	writingFunction += inAttribute ? T("\");\r\n		attribute->set")
									: T("\");\r\n		childNode->set");
	typeName = type->getString();
	typeName[0] -= 32;
	writingFunction += typeName;
	writingFunction += T("(");
	writingFunction += child->getName();
	writingFunction += T(");\r\n	}\r\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void XmlSchema::addSimpleVector(const XmlNode* child, String& structDefinition, size_t typeWidth,
								 String& readingFunction, String& writingFunction) const
{
	assert(child != NULL);
	XmlAttribute* type = child->findAttribute(ATTR_TYPE);
	if (type == NULL)
	{
		return;
	}
	
	String typeString = T("std::vector<");
	typeString += getSimpleTypeString(type);
	typeString += T(">");

	size_t thisWidth = typeString.size();
	assert(thisWidth < typeWidth + 1);
	for (size_t i = 0; i < typeWidth + 1 - thisWidth; ++i)
	{
		typeString += T(" ");
	}
	structDefinition += T("	");
	structDefinition += typeString;
	structDefinition += getPluralName(child->getName());
	structDefinition += T(";\r\n");	

	readingFunction += T("\r\n	childNode = node->findFirstChild(");
	readingFunction += LEFT_QUOTE;
	readingFunction += child->getName();
	readingFunction += T("\", iter);\r\n	while (childNode != NULL)\r\n	{\r\n		");
	readingFunction += getPluralName(child->getName());
	readingFunction += T(".resize(");
	readingFunction += getPluralName(child->getName());
	readingFunction += T(".size() + 1);\r\n		");
	readingFunction += getPluralName(child->getName());
	readingFunction += T(".back() = childNode->get");
	String typeName = type->getString();
	typeName[0] -= 32;
	readingFunction += typeName;
	readingFunction += T("();\r\n		childNode = node->findNextChild(");
	readingFunction += LEFT_QUOTE;
	readingFunction += child->getName();
	readingFunction += T("\", iter);\r\n	}\r\n");

	writingFunction += T("\r\n	for (std::vector<");
	writingFunction += getSimpleTypeString(type);
	writingFunction += T(">::const_iterator iter = ");
	writingFunction += getPluralName(child->getName());
	writingFunction += T(".begin();\r\n		  iter != ");
	writingFunction += getPluralName(child->getName());
	writingFunction += T(".end();\r\n		  ++iter)\r\n	{\r\n		const ");
	writingFunction += getSimpleTypeString(type);
	writingFunction += T("& value = *iter;\r\n");
	writingFunction += T("		childNode = node->addChild(");
	writingFunction += LEFT_QUOTE;
	writingFunction += child->getName();
	writingFunction += T("\");\r\n		childNode->set");
	writingFunction += typeName;
	writingFunction += T("(value);\r\n	}\r\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void XmlSchema::addStructMember(const XmlNode* child, String& structDefinition, size_t typeWidth,
								 String& readingFunction, String& writingFunction) const
{
	assert(child != NULL);
	bool recursive = (child->findAttribute(ATTR_RECURSIVE) != NULL);

	structDefinition += T("	");
	structDefinition += child->getName();
	size_t thisWidth = Strlen(child->getName());
	assert(thisWidth < typeWidth + 1);
	for (size_t i = 0;	i < typeWidth + 1 - thisWidth; ++i)
	{
		structDefinition += T(" ");
	}
	structDefinition += recursive ? T("*Child") : child->getName();
	structDefinition += T(";\r\n");

	readingFunction += T("\r\n	childNode = node->findChild(");
	readingFunction += LEFT_QUOTE;
	readingFunction += child->getName();
	readingFunction += T("\");\r\n	if (childNode != NULL)\r\n	{\r\n		");
	if (recursive)
	{
		readingFunction += T("Child = new ");
		readingFunction += child->getName();
		readingFunction += T(";\r\n		Child->read(childNode);\r\n	}\r\n");
	}
	else
	{
		readingFunction += child->getName();
		readingFunction += T(".read(childNode);\r\n	}\r\n");
	}
	if (recursive)
	{
		writingFunction += T("	if (Child != NULL)\r\n	{\r\n		childNode = node->addChild(");
		writingFunction += LEFT_QUOTE;
		writingFunction += child->getName();
		writingFunction += T("\");\r\n		Child->write(childNode);\r\n	}\r\n");
	}
	else
	{
		writingFunction += T("\r\n	childNode = node->addChild(");
		writingFunction += LEFT_QUOTE;
		writingFunction += child->getName();
		writingFunction += T("\");\r\n	");
		writingFunction += child->getName();
		writingFunction += T(".write(childNode);\r\n");
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void XmlSchema::addStructVector(const XmlNode* child, String& structDefinition, size_t typeWidth,
								 String& readingFunction, String& writingFunction) const
{
	assert(child != NULL);

	structDefinition += T("	std::vector<");
	structDefinition += child->getName();
	structDefinition += T(">");
	size_t thisWidth = Strlen(child->getName()) + Strlen(T("std::vector<>"));
	assert(thisWidth < typeWidth + 1);
	for (size_t i = 0;	i < typeWidth + 1 - thisWidth; ++i)
	{
		structDefinition += T(" ");
	}
	structDefinition += getPluralName(child->getName());
	structDefinition += T(";\r\n");

	readingFunction += T("\r\n	childNode = node->findFirstChild(");
	readingFunction += LEFT_QUOTE;
	readingFunction += child->getName();
	readingFunction += T("\", iter);\r\n	while (childNode != NULL)\r\n	{\r\n		");
	readingFunction += getPluralName(child->getName());
	readingFunction += T(".resize(");
	readingFunction += getPluralName(child->getName());
	readingFunction += T(".size() + 1);\r\n		");
	readingFunction += getPluralName(child->getName());
	readingFunction += T(".back().read(childNode);\r\n		childNode = node->findNextChild(");
	readingFunction += LEFT_QUOTE;
	readingFunction += child->getName();
	readingFunction += T("\", iter);\r\n	}\r\n");

	writingFunction += T("\r\n	for (std::vector<");
	writingFunction += child->getName();
	writingFunction += T(">::const_iterator iter = ");
	writingFunction += getPluralName(child->getName());
	writingFunction += T(".begin();\r\n		  iter != ");
	writingFunction += getPluralName(child->getName());
	writingFunction += T(".end();\r\n		  ++iter)\r\n	{\r\n		const ");
	writingFunction += child->getName();
	writingFunction += T("& obj = *iter;\r\n");
	writingFunction += T("		childNode = node->addChild(");
	writingFunction += LEFT_QUOTE;
	writingFunction += child->getName();
	writingFunction += T("\");\r\n		obj.write(childNode);\r\n	}\r\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void XmlSchema::addConstructorItem(const XmlNode* child, String& structDefinition, int& index) const
{
	XmlAttribute* type = child->findAttribute(ATTR_TYPE);
	bool recursive = (child->findAttribute(ATTR_RECURSIVE) != NULL);
	if (type == NULL && !recursive)
	{
		return;
	}
	XmlAttribute* multiple = child->findAttribute(ATTR_MULTIPLE);
	if (multiple != NULL && multiple->getBool())
	{
		//don't need to construct a vector member
		return;
	}
	XmlAttribute* defaultAttribute = child->findAttribute(ATTR_DEFAULT);
	if (type != NULL
		&& Strcmp(type->getString(), T("string")) == 0
		&& defaultAttribute == NULL)
	{
		//for String, need construction only when default value exist
		return;
	}

	if (index == 0)
	{
		structDefinition += T("	");
		structDefinition += child->getParent()->getName();
		structDefinition += T("()\r\n		:	");
	}
	else
	{
		structDefinition += T("		,	");
	}
	if (recursive)
	{
		structDefinition += T("Child");
	}
	else
	{
		structDefinition += child->getName();
	}
	structDefinition += T("(");
	++index;

	if (defaultAttribute != NULL)
	{
		if (Strcmp(type->getString(), T("string")) == 0)
		{
			structDefinition += LEFT_QUOTE;
			structDefinition += defaultAttribute->getString();
			structDefinition += T("\"");
		}
		else
		{
			structDefinition += defaultAttribute->getString();
		}
	}
	else
	{
		if (recursive)
		{
			structDefinition += T("NULL");
		}
		else
		{
			structDefinition += getTypeDefaultValue(type->getString());
		}
	}
	structDefinition += T(")\r\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
String XmlSchema::getSimpleTypeString(const XmlAttribute* type) const
{
	String typeString;
	if (Strcmp(type->getString(), T("string")) == 0)
	{
		typeString = L"std::wstring";
	}
	else
	{
		typeString = type->getString();
	}
	return typeString;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
const Char* XmlSchema::getTypeDefaultValue(const Char* type) const
{
	if (Strcmp(type, T("string")) == 0)
	{
		return LEFT_QUOTE T("\"");
	}
	if (Strcmp(type, T("bool")) == 0)
	{
		return T("false");
	}
	if (Strcmp(type, T("int")) == 0)
	{
		return T("0");
	}
	if (Strcmp(type, T("float")) == 0)
	{
		return T("0.0f");
	}
	return T("deadbeef");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
String XmlSchema::getPluralName(const String& name) const
{
	if (name.size() <= 0)
	{
		return T("");
	}
	String plural = name;
	if (name[name.size()-1] == T('o')
		|| name[name.size()-1] == T('s')
		|| name[name.size()-1] == T('x'))
	{
		plural += T("es");
	}
	else if (name[name.size() - 1] == T('y')
		&& (name.size() > 2 && name[name.size()-2] != T('a')
							 && name[name.size()-2] != T('e')
							 && name[name.size()-2] != T('i')
							 && name[name.size()-2] != T('o')
							 && name[name.size()-2] != T('u')))
	{
		plural[name.size() - 1] = T('i');
		plural += T("es");
	}
	else if (name.size() > 2 && name[name.size()-1] == T('h')
		&& (name[name.size()-2] == T('s') || name[name.size()-2] == T('c')))
	{
		plural += T("es");
	}
	else
	{
		plural += T("s");
	}
	//sorry, I don't want to be writing this for a whole day, so please don't name your data as sheep, child, deer, knife...
	return plural;
}

}
