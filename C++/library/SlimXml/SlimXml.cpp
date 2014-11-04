#include "SlimXml.h"
#include <cassert>
#include <fstream>

namespace slim
{

	///////////////////////////////////////////////////////////////////////////////////////////////////
	void XmlBase::assignString(Char* &str, Char* buffer, size_t length, bool transferCharacter)
	{
		const Char* found = NULL;
		if (!transferCharacter || (found = Memchr(buffer, T('&'), length)) == NULL)
		{
			str = buffer;
			str[length] = 0;
			return;
		}
		String temp;
		for (; found != NULL; found = Memchr(buffer, T('&'), length))
		{
			temp.append(buffer, found - buffer);
			length -= (found - buffer + 1);
			buffer = const_cast<Char *>(found + 1);
			if (length >= 5)
			{
				if (Strncmp(buffer, T("quot"), 4) == 0)
				{
					buffer += 4;
					length -= 4;
					temp.append(1, T('\"'));
					continue;
				}
				if (Strncmp(found + 1, T("apos"), 4) == 0)
				{
					buffer += 4;
					length -= 4;
					temp.append(1, T('\''));
					continue;
				}
			}
			if (length >= 4)
			{
				if (Strncmp(buffer, T("amp"), 3) == 0)
				{
					buffer += 3;
					length -= 3;
					temp.append(1, T('&'));
					continue;
				}
			}
			if (length >= 3)
			{
				if (Strncmp(buffer, T("lt"), 2) == 0)
				{
					buffer += 2;
					length -= 2;
					temp.append(1, T('<'));
					continue;
				}
				else if (Strncmp(buffer, T("gt"), 2) == 0)
				{
					buffer += 2;
					length -= 2;
					temp.append(1, T('>'));
					continue;
				}
			}
			temp.append(1, T('&'));
		}
		temp.append(buffer, length);
		size_t actualLength = temp.length();
		memcpy(str, temp.c_str(), sizeof(Char) * actualLength);
		str[actualLength] = 0;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	XmlNode::XmlNode(NodeType type, XmlNode* parent)
		: m_type(type)
		, m_parent(parent)
	{
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	XmlNode::~XmlNode()
	{
		clearAttribute();
		clearChild();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	XmlNode* XmlNode::findChild(const Char* name) const
	{
		assert(name != NULL);
		for (NodeList::const_iterator iter = m_children.begin();
		iter != m_children.end();
			++iter)
		{
			XmlNode* child = *iter;
			assert(child != NULL);
			if (Strcmp(child->m_name, name) == 0)
			{
				return child;
			}
		}
		return NULL;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	XmlNode* XmlNode::findFirstChild(const Char* name, NodeIterator& iter) const
	{
		assert(name != NULL);
		iter = m_children.begin();
		while (iter != m_children.end())
		{
			XmlNode* child = *iter;
			assert(child != NULL);
			if (Strcmp(child->m_name, name) == 0)
			{
				return child;
			}
			++iter;
		}
		return NULL;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	XmlNode* XmlNode::findNextChild(const Char* name, NodeIterator& iter) const
	{
		assert(name != NULL);
		if (iter != m_children.end())
		{
			while (++iter != m_children.end())
			{
				XmlNode* child = *iter;
				assert(child != NULL);
				if (Strcmp(child->m_name, name) == 0)
				{
					return child;
				}
			}
		}
		return NULL;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	size_t XmlNode::getChildCount(const Char* name) const
	{
		assert(name != NULL);

		size_t count = 0;
		for (NodeIterator iter = m_children.begin();
		iter != m_children.end();
			++iter)
		{
			XmlNode* child = *iter;
			assert(child != NULL);
			if (Strcmp(child->m_name, name) == 0)
			{
				++count;
			}
		}
		return count;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	void XmlNode::removeChild(XmlNode* node)
	{
		assert(node != NULL);
		for (NodeList::iterator iter = m_children.begin();
		iter != m_children.end();
			++iter)
		{
			if (*iter == node)
			{
				delete node;
				m_children.erase(iter);
				return;
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	void XmlNode::clearChild()
	{
		for (NodeList::iterator iter = m_children.begin();
		iter != m_children.end();
			++iter)
		{
			XmlNode* child = *iter;
			assert(child != NULL);
			delete child;
		}
		m_children.clear();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	XmlNode* XmlNode::addChild(const Char* name, NodeType type)
	{
		if (type != COMMENT && type != ELEMENT)
		{
			return NULL;
		}
		XmlNode* child = new XmlNode(type, this);
		if (name != NULL)
		{
			child->setName(name);
		}
		m_children.push_back(child);
		return child;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	XmlAttribute* XmlNode::addAttribute(const Char* name, const Char* value)
	{
		XmlAttribute* attribute = new XmlAttribute;
		if (name != NULL)
		{
			attribute->setName(name);
		}
		if (value != NULL)
		{
			attribute->setString(value);
		}
		m_attributes.push_back(attribute);
		return attribute;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	XmlAttribute* XmlNode::addAttribute(const Char* name, bool value)
	{
		XmlAttribute* attribute = addAttribute(name);
		attribute->setBool(value);
		return attribute;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	XmlAttribute* XmlNode::addAttribute(const Char* name, int value)
	{
		XmlAttribute* attribute = addAttribute(name);
		attribute->setInt(value);
		return attribute;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	XmlAttribute* XmlNode::addAttribute(const Char* name, float value)
	{
		XmlAttribute* attribute = addAttribute(name);
		attribute->setFloat(value);
		return attribute;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	XmlAttribute* XmlNode::addAttribute(const Char* name, double value)
	{
		XmlAttribute* attribute = addAttribute(name);
		attribute->setDouble(value);
		return attribute;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	XmlAttribute* XmlNode::findAttribute(const Char* name) const
	{
		for (AttributeList::const_iterator iter = m_attributes.begin();
		iter != m_attributes.end();
			++iter)
		{
			XmlAttribute* attribute = *iter;
			assert(attribute != NULL);
			if (Strcmp(attribute->getName(), name) == 0)
			{
				return attribute;
			}
		}
		return NULL;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	const Char* XmlNode::readAttributeAsString(const Char* name, const Char* defaultValue) const
	{
		XmlAttribute* attribute = findAttribute(name);
		if (attribute == NULL)
		{
			return defaultValue;
		}
		return attribute->getString();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	bool XmlNode::readAttributeAsBool(const Char* name, bool defaultValue) const
	{
		XmlAttribute* attribute = findAttribute(name);
		if (attribute == NULL)
		{
			return defaultValue;
		}
		return attribute->getBool();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	int XmlNode::readAttributeAsInt(const Char* name, int defaultValue) const
	{
		XmlAttribute* attribute = findAttribute(name);
		if (attribute == NULL)
		{
			return defaultValue;
		}
		return attribute->getInt();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	void XmlNode::readAttributeAsIntArray(const Char* name, int* out, unsigned long length, int defaultValue) const
	{
		XmlAttribute* attribute = findAttribute(name);
		if (attribute == NULL)
		{
			for (unsigned long i = 0; i < length; ++i)
			{
				out[i] = defaultValue;
			}
			return;
		}
		unsigned long stringLen = Strlen(attribute->getString());
		Char* tempBuffer = new Char[stringLen + 1];
		Strcpy(tempBuffer, attribute->getString());
		Char* current = tempBuffer;

		unsigned long index = 0;
		while (index < length)
		{
			Char* found = (Char*)Memchr(current, T(','), stringLen);
			if (found != NULL)
			{
				*found = 0;
			}
			out[index] = StrToI(current);
			stringLen -= (found - current + 1);
			current = found + 1;
			++index;
			if (found == NULL)
			{
				break;
			}
		}
		for (; index < length; ++index)
		{
			out[index] = defaultValue;
		}
		delete[] tempBuffer;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	unsigned long XmlNode::readAttributeAsHex(const Char* name, unsigned long defaultValue) const
	{
		XmlAttribute* attribute = findAttribute(name);
		if (attribute == NULL)
		{
			return defaultValue;
		}
		return attribute->getHex();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	float XmlNode::readAttributeAsFloat(const Char* name, float defaultValue) const
	{
		XmlAttribute* attribute = findAttribute(name);
		if (attribute == NULL)
		{
			return defaultValue;
		}
		return attribute->getFloat();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	double XmlNode::readAttributeAsDouble(const Char* name, double defaultValue) const
	{
		XmlAttribute* attribute = findAttribute(name);
		if (attribute == NULL)
		{
			return defaultValue;
		}
		return attribute->getDouble();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	unsigned long XmlNode::readAttributeAsEnum(const Char* name, const Char* const* enumNames,
		unsigned long enumCount, unsigned long defaultValue) const
	{
		XmlAttribute* attribute = findAttribute(name);
		if (attribute == NULL)
		{
			return defaultValue;
		}
		for (unsigned long i = 0; i < enumCount; ++i)
		{
			if (Strcmp(enumNames[i], attribute->getString()) == 0)
			{
				return i;
			}
		}
		return defaultValue;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	void XmlNode::removeAttribute(XmlAttribute* attribute)
	{
		assert(attribute != NULL);
		for (AttributeList::iterator iter = m_attributes.begin();
		iter != m_attributes.end();
			++iter)
		{
			if (*iter == attribute)
			{
				delete attribute;
				m_attributes.erase(iter);
				return;
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	void XmlNode::clearAttribute()
	{
		for (AttributeList::iterator iter = m_attributes.begin();
		iter != m_attributes.end();
			++iter)
		{
			delete *iter;
		}
		m_attributes.clear();
	}
	
	///////////////////////////////////////////////////////////////////////////////////////////////////
	//class CXmlDocument
	///////////////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////////////
	XmlDocument::XmlDocument()
		: XmlNode(DOCUMENT, NULL)
		, m_buffer(NULL)
	{
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	XmlDocument::~XmlDocument()
	{
		if (m_buffer != NULL)
		{
			delete[] m_buffer;
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	bool XmlDocument::loadFromFile(const Char* ufilename)
	{
		wchar_t * filename = const_cast<wchar_t *>(ufilename);
		Matrix::File file(filename);
		size_t file_len = 0;
		const wchar_t * text = file.Text(0, &file_len);

		return parse(const_cast<wchar_t *>(text), file_len);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	bool XmlDocument::parse(Char* input, size_t size)
	{
		Char* cur = input;
		Char* end = input + size;

		Char* label = NULL;
		size_t labelSize = 0;
		int depth = 0;
		XmlNode* currentNode = this;

		while (cur < end)
		{
			assert(depth >= 0);
			assert(currentNode != NULL);

			Char* lastPos = cur;
			if (!findLabel(cur, end - cur, label, labelSize))
			{
				break;
			}
			switch (*label)
			{
			case T('/'):	//node ending
				if (depth < 1)
				{
					return false;
				}
				if (currentNode->getType() == ELEMENT && !currentNode->hasChild())
				{
					currentNode->assignString(currentNode->m_value, lastPos, label - lastPos - 1, true);
				}
				currentNode = currentNode->getParent();
				--depth;
				break;
			case T('?'):	//xml define node, ignore
				break;
			case T('!'):	//comment node
			{
				//ignore !-- and --
				if (labelSize < 5)
				{
					return false;
				}
				XmlNode* comment = currentNode->addChild(NULL, COMMENT);
				comment->assignString(comment->m_name, label + 3, labelSize - 5, false);
			}
				break;
			default:	//node start
			{
				XmlNode* newNode = currentNode->addChild(NULL, ELEMENT);

				bool emptyNode = parseLabel(newNode, label, labelSize);

				if (!emptyNode)
				{
					currentNode = newNode;
					++depth;
				}
			}
			}
		} // while(cur < end)

		if (depth != 0)
		{
			return false;
		}
		assert(currentNode == this);
		return true;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	bool XmlDocument::findLabel(Char* &begin, size_t size, Char* &label, size_t &labelSize)
	{
		label = (Char*)Memchr(begin, T('<'), size);
		if (label == NULL)
		{
			return false;
		}
		++label;
		size -= (label - begin);

		//comment is special, won't end without "-->"
		if (size > 6 //Strlen(T("!---->"))
			&& label[0] == T('!')
			&& label[1] == T('-')
			&& label[2] == T('-'))
		{
			//buffer is not NULL-terminated, so we can't use strstr, shit! is there a "safe" version of strstr?
			Char* cur = label + 3;	//skip !--
			size -= 5; //(Strlen(T("!---->")) - 1);
			while (true)
			{
				Char* end = (Char*)Memchr(cur, T('-'), size);
				if (end == NULL)
				{
					return false;
				}
				if (*(end + 1) == T('-') && *(end + 2) == T('>'))
				{
					//get it
					labelSize = end - label + 2;
					begin = end + 3;
					return true;
				}
				size -= (end - cur + 1);
				cur = end + 1;
			}
		}
		begin = (Char*)Memchr(label, T('>'), size);
		if (begin == NULL)
		{
			return false;
		}
		labelSize = begin - label;
		++begin;
		if (labelSize == 0)
		{
			return false;
		}
		return true;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	bool XmlDocument::parseLabel(XmlNode* node, Char* label, size_t labelSize)
	{
		//get name
		Char* cur = label;
		while (*cur != T(' ') && *cur != T('/') && *cur != T('>'))
		{
			++cur;
		}
		Char next = *cur;
		node->assignString(node->m_name, label, cur - label, true);
		if (next != T(' '))
		{
			return next == T('/');
		}
		//get attributes
		Char* end = label + labelSize;
		++cur;
		while (cur < end)
		{
			while (*cur == T(' '))
			{
				++cur;
			}
			//attribute name
			Char* attrName = cur;
			while (*cur != T(' ') && *cur != T('=') && *cur != T('/') && *cur != T('>'))
			{
				++cur;
			}
			next = *cur;
			size_t attrNameSize = cur - attrName;

			//attribute value
			cur = (Char*)Memchr(cur, T('"'), end - cur);
			if (cur == NULL)
			{
				break;
			}
			Char* attrValue = ++cur;
			cur = (Char*)Memchr(cur, T('"'), end - cur);
			if (NULL == cur)
			{
				return false;
			}
			size_t attrValueSize = cur - attrValue;
			XmlAttribute* attribute = node->addAttribute();
			attribute->assignString(attribute->m_name, attrName, attrNameSize, true);
			attribute->assignString(attribute->m_value, attrValue, attrValueSize, true);
			++cur;
		}
		return next == T('/');
	}

}
