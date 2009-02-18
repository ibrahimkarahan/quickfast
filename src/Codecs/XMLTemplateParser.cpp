// Copyright (c) 2009, Object Computing, Inc.
// All rights reserved.
// See the file license.txt for licensing information.
#include <Common/QuickFASTPch.h>
#include "XMLTemplateParser.h"
#include <Codecs/TemplateRegistry.h>
#include <Codecs/Template.h>
#include <Codecs/FieldInstructionInt32.h>
#include <Codecs/FieldInstructionUInt32.h>
#include <Codecs/FieldInstructionInt64.h>
#include <Codecs/FieldInstructionUInt64.h>
#include <Codecs/FieldInstructionDecimal.h>
#include <Codecs/FieldInstructionExponent.h>
#include <Codecs/FieldInstructionMantissa.h>
#include <Codecs/FieldInstructionAscii.h>
#include <Codecs/FieldInstructionUtf8.h>
#include <Codecs/FieldInstructionByteVector.h>
#include <Codecs/FieldInstructionGroup.h>
#include <Codecs/FieldInstructionSequence.h>
#include <Codecs/FieldInstructionTemplateRef.h>

#include <Codecs/FieldOpConstant.h>
#include <Codecs/FieldOpDefault.h>
#include <Codecs/FieldOpCopy.h>
#include <Codecs/FieldOpDelta.h>
#include <Codecs/FieldOpIncrement.h>
#include <Codecs/FieldOpTail.h>

#include <Common/Exceptions.h>

#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/sax/InputSource.hpp>
#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>

#include <string>

using namespace ::QuickFAST;
using namespace ::QuickFAST::Codecs;
XERCES_CPP_NAMESPACE_USE

namespace
{
  /// @brief convert XERCES Attributes to a map for easier access
  class TemplateBuilder
    : public DefaultHandler
  {
  public:
    /// An easier-to-access version of the XML attributes
    typedef std::map<std::string, std::string> AttributeMap;

  public:
    TemplateBuilder(
      TemplateRegistryPtr registry,
      std::ostream * out
      )
      : registry_(registry)
      , out_(out)
    {
    }

    void makeAttrs(
      const Attributes& attributes,
      AttributeMap& attrs
      )
    {
      for (size_t index = 0; index < attributes.getLength(); ++index)
      {
        std::string name = XMLString::transcode(attributes.getQName(index));
        std::string value = XMLString::transcode(attributes.getValue(index));
        attrs[name] = value;
      }
    }

    virtual void startDocument()
    {
    }

    virtual void endDocument()
    {
      // todo: registry reconcile to let it do any final cleanup?
      registry_->finalize();
    }

    virtual void startElement(
      const XMLCh* const uri,
      const XMLCh* const localname,
      const XMLCh* const qname,
      const Attributes& attributes
      )
    {
      // convert the attributes to a convenient form
      AttributeMap attributeMap;
      makeAttrs(attributes, attributeMap);

      // then switch on element tag
      std::string tag(XMLString::transcode(localname));
      if(out_)
      {
        *out_ << std::string(2*schemaElements_.size(), ' ') << '<' << tag;
        for(AttributeMap::const_iterator it = attributeMap.begin();
          it != attributeMap.end();
          ++it)
        {
          *out_ << ' ' << it->first << '=' << it->second;
        }
        *out_ << '>' << std::endl;
      }
      if (tag == "templates")
      {
        parseTemplateRegistry(tag, attributeMap);
      }
      else if (tag == "template")
      {
        parseTemplate(tag, attributeMap);
      }
      else if (tag == "typeRef")
      {
        parseTypeRef(tag, attributeMap);
      }
      else if (tag == "int32")
      {
        parseInt32(tag, attributeMap);
      }
      else if (tag == "uInt32")
      {
        parseUInt32(tag, attributeMap);
      }
      else if (tag == "int64")
      {
        parseInt64(tag, attributeMap);
      }
      else if (tag == "uInt64")
      {
        parseUInt64(tag, attributeMap);
      }
      else if (tag == "decimal")
      {
        parseDecimal(tag, attributeMap);
      }
      else if (tag == "string")
      {
        parseString(tag, attributeMap);
      }
      else if (tag == "byteVector")
      {
        parseByteVector(tag, attributeMap);
      }
      else if (tag == "group")
      {
        parseGroup(tag, attributeMap);
      }
      else if (tag == "sequence")
      {
        parseSequence(tag, attributeMap);
      }
      else if (tag == "constant")
      {
        parseConstant(tag, attributeMap);
      }
      else if (tag == "default")
      {
        parseDefault(tag, attributeMap);
      }
      else if (tag == "copy")
      {
        parseCopy(tag, attributeMap);
      }
      else if (tag == "delta")
      {
        parseDelta(tag, attributeMap);
      }
      else if (tag == "increment")
      {
        parseIncrement(tag, attributeMap);
      }
      else if (tag == "tail")
      {
        parseTail(tag, attributeMap);
      }
      else if (tag == "length")
      {
        parseLength(tag, attributeMap);
      }
      else if (tag == "exponent")
      {
        parseExponent(tag, attributeMap);
      }
      else if (tag == "mantissa")
      {
        parseMantissa(tag, attributeMap);
      }
      else if (tag == "templateRef")
      {
        parseTemplateRef(tag, attributeMap);
      }
      else
      {
        std::string errMsg("unknown XML tag: ");
        errMsg += tag;
        throw std::runtime_error(errMsg);
      }
    }

    virtual void endElement(
      const XMLCh* const uri,
      const XMLCh* const localname,
      const XMLCh* const qname
      )
    {
      std::string tag(XMLString::transcode(localname));
      // don't finalize here for templates.  Because it's optional
      // it will be forcibly finalized at end of document
      if(schemaElements_.top().first == tag)
      {
        if(tag != "templates")
        {
          schemaElements_.top().second->finalize();
        }
        schemaElements_.pop();
      }
      if(out_)
      {
        *out_ << std::string(2*schemaElements_.size(), ' ') << "</" << tag << '>' << std::endl;;
      }
    }

    virtual void characters(
      const XMLCh* const chars,
      const XMLSize_t length
      )
    {
    }

    virtual void ignorableWhitespace(
      const XMLCh* const chars,
      const XMLSize_t length
      )
    {
    }

    virtual void processingInstruction(
      const XMLCh* const target,
      const XMLCh* const data
      )
    {
    }

    /*
    implement the virtual methods that handle various errors from the XML file:
       (warning, error, fatal, ...)
    */

    virtual void warning(
      const SAXParseException& exc
      )
    {
      throw TemplateDefinitionError(XMLString::transcode(exc.getMessage()));
    }

    virtual void error(
      const SAXParseException& exc
      )
    {
      throw TemplateDefinitionError(XMLString::transcode(exc.getMessage()));
    }

    virtual void fatalError(
      const SAXParseException& exc
      )
    {
      throw TemplateDefinitionError(XMLString::transcode(exc.getMessage()));
    }

  private:
    TemplateRegistryPtr registry_;

    const std::string & getRequiredAttribute(
    const AttributeMap& attributes,
    const std::string& name);

    bool getOptionalAttribute(
    const AttributeMap& attributes,
    const std::string& name,
    std::string & result);

    void parseTemplateRegistry(const std::string & tag, const AttributeMap& attributes);
    void parseTemplate(const std::string & tag, const AttributeMap& attributes);
    void parseTypeRef(const std::string & tag, const AttributeMap& attributes);
    void parseInt32(const std::string & tag, const AttributeMap& attributes);
    void parseUInt32(const std::string & tag, const AttributeMap& attributes);
    void parseInt64(const std::string & tag, const AttributeMap& attributes);
    void parseUInt64(const std::string & tag, const AttributeMap& attributes);
    void parseDecimal(const std::string & tag, const AttributeMap& attributes);
    void parseExponent(const std::string & tag, const AttributeMap& attributes);
    void parseMantissa(const std::string & tag, const AttributeMap& attributes);
    void parseString(const std::string & tag, const AttributeMap& attributes);
    void parseByteVector(const std::string & tag, const AttributeMap& attributes);
    void parseGroup(const std::string & tag, const AttributeMap& attributes);
    void parseSequence(const std::string & tag, const AttributeMap& attributes);
    void parseLength(const std::string & tag, const AttributeMap& attributes);
    void parseConstant(const std::string & tag, const AttributeMap& attributes);
    void parseDefault(const std::string & tag, const AttributeMap& attributes);
    void parseCopy(const std::string & tag, const AttributeMap& attributes);
    void parseDelta(const std::string & tag, const AttributeMap& attributes);
    void parseIncrement(const std::string & tag, const AttributeMap& attributes);
    void parseTail(const std::string & tag, const AttributeMap& attributes);
    void parseTemplateRef(const std::string & tag, const AttributeMap& attributes);

    void parseInitialValue(const std::string & tag, const AttributeMap& attributes, FieldOpPtr op);
    void parseOp(const std::string & tag, const AttributeMap& attributes, FieldOpPtr op);

  private:
    typedef std::pair<std::string, SchemaElementPtr> StackEntry;
    typedef std::stack<StackEntry> SchemaStack ;
    SchemaStack schemaElements_;
    std::ostream * out_;
  };
}

const std::string &
TemplateBuilder::getRequiredAttribute(
  const AttributeMap& attributes,
  const std::string& name
  )
{
  AttributeMap::const_iterator it = attributes.find(name);
  if(it == attributes.end())
  {
    std::string errMsg;
    errMsg +=
      "Missing required attribute \"" + name + "\"";
    throw std::runtime_error(errMsg);
  }
  return it->second;
}

bool
TemplateBuilder::getOptionalAttribute(
  const AttributeMap& attributes,
  const std::string& name,
  std::string & result
  )
{
  AttributeMap::const_iterator it = attributes.find(name);
  if(it == attributes.end())
  {
    return false;
  }
  result = it->second;
  return true;
}

void
TemplateBuilder::parseTemplateRegistry(const std::string & tag, const AttributeMap& attributes)
{
  // because <templates> is optional, we already have a registry.
  std::string ns;
  if(getOptionalAttribute(attributes, "ns", ns))
  {
    registry_->setNamespace(ns);
  }
  std::string templateNs;
  if(getOptionalAttribute(attributes, "templateNs", templateNs))
  {
    registry_->setTemplateNamespace(templateNs);
  }

  std::string dictionary;
  if (getOptionalAttribute(attributes, "dictionary", dictionary))
  {
    registry_->setDictionaryName(dictionary);
  }
  schemaElements_.push(StackEntry(tag, registry_));
}

void
TemplateBuilder::parseTemplate(const std::string & tag, const AttributeMap& attributes)
{
  TemplatePtr target(new Template);

  target->setTemplateName(getRequiredAttribute(attributes, "name"));
  std::string ns;
  if(getOptionalAttribute(attributes, "ns", ns))
  {
    target->setNamespace(ns);
  }

  std::string templateNs;
  if(getOptionalAttribute(attributes, "templateNs", templateNs))
  {
    target->setTemplateNamespace(templateNs);
  }

  std::string id;
  if (getOptionalAttribute(attributes, "id", id))
  {
    target->setId(
      boost::lexical_cast<template_id_t>(id)
      );
  }

  std::string dictionary;
  if (getOptionalAttribute(attributes, "dictionary", dictionary))
  {
    target->setDictionaryName(dictionary);
  }

  schemaElements_.top().second->addTemplate(target);
  schemaElements_.push(StackEntry(tag, target));
}

void
TemplateBuilder::parseTypeRef(const std::string & tag, const AttributeMap& attributes)
{
  std::string name = getRequiredAttribute(attributes, "name");
  std::string ns;
  getOptionalAttribute(attributes, "ns", ns);
  schemaElements_.top().second->setApplicationType(name, ns);
}

void
TemplateBuilder::parseInt32(const std::string & tag, const AttributeMap& attributes)
{
  std::string name = getRequiredAttribute(attributes, "name");
  std::string ns;
  getOptionalAttribute(attributes, "ns", ns);
  FieldInstructionPtr field(new FieldInstructionInt32(name, ns));
  std::string id;
  if (getOptionalAttribute(attributes, "id", id))
  {
    field->setId(id);
  }
  std::string presence;
  if(getOptionalAttribute(attributes, "presence", presence))
  {
    field->setPresence(presence == "mandatory");
  }
  schemaElements_.top().second->addInstruction(field);
  schemaElements_.push(StackEntry(tag, field));
}

void
TemplateBuilder::parseUInt32(const std::string & tag, const AttributeMap& attributes)
{
  std::string name = getRequiredAttribute(attributes, "name");
  std::string ns;
  getOptionalAttribute(attributes, "ns", ns);
  FieldInstructionPtr field(new FieldInstructionUInt32(name, ns));
  std::string id;
  if (getOptionalAttribute(attributes, "id", id))
  {
    field->setId(id);
  }
  std::string presence;
  if(getOptionalAttribute(attributes, "presence", presence))
  {
    field->setPresence(presence == "mandatory");
  }
  schemaElements_.top().second->addInstruction(field);
  schemaElements_.push(StackEntry(tag, field));
}

void
TemplateBuilder::parseInt64(const std::string & tag, const AttributeMap& attributes)
{
  std::string name = getRequiredAttribute(attributes, "name");
  std::string ns;
  getOptionalAttribute(attributes, "ns", ns);
  FieldInstructionPtr field(new FieldInstructionInt64(name, ns));
  std::string id;
  if (getOptionalAttribute(attributes, "id", id))
  {
    field->setId(id);
  }
  std::string presence;
  if(getOptionalAttribute(attributes, "presence", presence))
  {
    field->setPresence(presence == "mandatory");
  }
  schemaElements_.top().second->addInstruction(field);
  schemaElements_.push(StackEntry(tag, field));
}

void
TemplateBuilder::parseUInt64(const std::string & tag, const AttributeMap& attributes)
{
  std::string name = getRequiredAttribute(attributes, "name");
  std::string ns;
  getOptionalAttribute(attributes, "ns", ns);
  FieldInstructionPtr field(new FieldInstructionUInt64(name, ns));
  std::string id;
  if (getOptionalAttribute(attributes, "id", id))
  {
    field->setId(id);
  }
  std::string presence;
  if(getOptionalAttribute(attributes, "presence", presence))
  {
    field->setPresence(presence == "mandatory");
  }
  schemaElements_.top().second->addInstruction(field);
  schemaElements_.push(StackEntry(tag, field));
}

void
TemplateBuilder::parseDecimal(const std::string & tag, const AttributeMap& attributes)
{
  std::string name = getRequiredAttribute(attributes, "name");
  std::string ns;
  getOptionalAttribute(attributes, "ns", ns);
  FieldInstructionPtr field(new FieldInstructionDecimal(name, ns));
  std::string id;
  if (getOptionalAttribute(attributes, "id", id))
  {
    field->setId(id);
  }
  std::string presence;
  if(getOptionalAttribute(attributes, "presence", presence))
  {
    field->setPresence(presence == "mandatory");
  }
  schemaElements_.top().second->addInstruction(field);
  schemaElements_.push(StackEntry(tag, field));
}

void
TemplateBuilder::parseExponent(const std::string & tag, const AttributeMap& attributes)
{
  FieldInstructionExponentPtr field(new FieldInstructionExponent);
  std::string presence;
  if(getOptionalAttribute(attributes, "presence", presence))
  {
    field->setPresence(presence == "mandatory");
  }
  schemaElements_.top().second->setExponentInstruction(field);
  schemaElements_.push(StackEntry(tag, field));
}

void
TemplateBuilder::parseMantissa(const std::string & tag, const AttributeMap& attributes)
{
  FieldInstructionMantissaPtr field(new FieldInstructionMantissa);
  std::string presence;
  if(getOptionalAttribute(attributes, "presence", presence))
  {
    field->setPresence(presence == "mandatory");
  }
  schemaElements_.top().second->setMantissaInstruction(field);
  schemaElements_.push(StackEntry(tag, field));
}

void
TemplateBuilder::parseString(const std::string & tag, const AttributeMap& attributes)
{
  std::string name = getRequiredAttribute(attributes, "name");
  std::string ns;
  getOptionalAttribute(attributes, "ns", ns);

  std::string charset = "ascii";
  getOptionalAttribute(attributes, "charset", charset);
  FieldInstructionPtr field;
  if(charset == "unicode")
  {
    field.reset(new FieldInstructionUtf8(name, ns));
  }
  else
  {
    field.reset(new FieldInstructionAscii(name, ns));
  }
  std::string id;
  if (getOptionalAttribute(attributes, "id", id))
  {
    field->setId(id);
  }
  std::string presence;
  if(getOptionalAttribute(attributes, "presence", presence))
  {
    field->setPresence(presence == "mandatory");
  }
  schemaElements_.top().second->addInstruction(field);
  schemaElements_.push(StackEntry(tag, field));
}

void
TemplateBuilder::parseByteVector(const std::string & tag, const AttributeMap& attributes)
{
  std::string name = getRequiredAttribute(attributes, "name");
  std::string ns;
  getOptionalAttribute(attributes, "ns", ns);
  FieldInstructionPtr field(new FieldInstructionByteVector(name, ns));
  std::string id;
  if (getOptionalAttribute(attributes, "id", id))
  {
    field->setId(id);
  }
  std::string presence;
  if(getOptionalAttribute(attributes, "presence", presence))
  {
    field->setPresence(presence == "mandatory");
  }
  schemaElements_.top().second->addInstruction(field);
  schemaElements_.push(StackEntry(tag, field));
}

void
TemplateBuilder::parseGroup(const std::string & tag, const AttributeMap& attributes)
{
  std::string name = getRequiredAttribute(attributes, "name");
  std::string ns;
  getOptionalAttribute(attributes, "ns", ns);
  FieldInstructionPtr field(new FieldInstructionGroup(name, ns));
  std::string id;
  if (getOptionalAttribute(attributes, "id", id))
  {
    field->setId(id);
  }
  std::string presence;
  if(getOptionalAttribute(attributes, "presence", presence))
  {
    field->setPresence(presence == "mandatory");
  }
  std::string dictionary;
  if(getOptionalAttribute(attributes, "dictionary", dictionary))
  {
    field->setDictionaryName(dictionary);
  }
  SegmentBodyPtr body(new SegmentBody);
  field->setSegmentBody(body);

  schemaElements_.top().second->addInstruction(field);
  schemaElements_.push(StackEntry(tag, body));
}

void
TemplateBuilder::parseSequence(const std::string & tag, const AttributeMap& attributes)
{
  std::string name = getRequiredAttribute(attributes, "name");
  std::string ns;
  getOptionalAttribute(attributes, "ns", ns);
  FieldInstructionPtr field(new FieldInstructionSequence(name, ns));
  std::string id;
  if (getOptionalAttribute(attributes, "id", id))
  {
    field->setId(id);
  }
  std::string presence;
  bool mandatory = true;
  if(getOptionalAttribute(attributes, "presence", presence))
  {
    mandatory = presence == "mandatory";
    field->setPresence(mandatory);
  }
  std::string dictionary;
  if(getOptionalAttribute(attributes, "dictionary", dictionary))
  {
    field->setDictionaryName(dictionary);
  }
  SegmentBodyPtr body(new SegmentBody);
  field->setSegmentBody(body);
  body->allowLengthField();
  body->setMandatoryLength(mandatory);

  schemaElements_.top().second->addInstruction(field);
  schemaElements_.push(StackEntry(tag, body));
}

void
TemplateBuilder::parseLength(const std::string & tag, const AttributeMap& attributes)
{
  std::string name = getRequiredAttribute(attributes, "name");
  std::string ns;
  getOptionalAttribute(attributes, "ns", ns);
  FieldInstructionPtr field(new FieldInstructionUInt32(name, ns));
  schemaElements_.top().second->addLengthInstruction(field);
  // Is this push necessary?
  schemaElements_.push(StackEntry(tag, field));
}

void
TemplateBuilder::parseConstant(const std::string & tag, const AttributeMap& attributes)
{
  getRequiredAttribute(attributes, "value");
  FieldOpPtr op(new FieldOpConstant);
  parseInitialValue(tag, attributes, op);
}

void
TemplateBuilder::parseDefault(const std::string & tag, const AttributeMap& attributes)
{
  FieldOpPtr op(new FieldOpDefault);
  parseInitialValue(tag, attributes, op);
}

void
TemplateBuilder::parseCopy(const std::string & tag, const AttributeMap& attributes)
{
  FieldOpPtr op(new FieldOpCopy);
  parseOp(tag, attributes, op);
}

void
TemplateBuilder::parseDelta(const std::string & tag, const AttributeMap& attributes)
{
  FieldOpPtr op(new FieldOpDelta);
  parseOp(tag, attributes, op);
}

void
TemplateBuilder::parseIncrement(const std::string & tag, const AttributeMap& attributes)
{
  FieldOpPtr op(new FieldOpIncrement);
  parseOp(tag, attributes, op);
}

void
TemplateBuilder::parseTail(const std::string & tag, const AttributeMap& attributes)
{
  FieldOpPtr op(new FieldOpTail);
  parseOp(tag, attributes, op);
}

void
TemplateBuilder::parseTemplateRef(const std::string & tag, const AttributeMap& attributes)
{
  std::string name;
  getOptionalAttribute(attributes, "name", name);
  std::string ns;
  getOptionalAttribute(attributes, "ns", ns);
  FieldInstructionPtr field(new FieldInstructionTemplateRef(name, ns));
  schemaElements_.top().second->addInstruction(field);
  // Is this push necessary?
  schemaElements_.push(StackEntry(tag, field));
}

void
TemplateBuilder::parseInitialValue(const std::string & tag, const AttributeMap& attributes, FieldOpPtr op)
{
  std::string value;
  if(getOptionalAttribute(attributes, "value", value))
  {
    op->setValue(value);
  }
  schemaElements_.top().second->setFieldOp(op);
  // is this push necessary?
  schemaElements_.push(StackEntry(tag, op));
}

void
TemplateBuilder::parseOp(const std::string & tag, const AttributeMap& attributes, FieldOpPtr op)
{
  std::string dictionary;
  if(getOptionalAttribute(attributes, "dictionary", dictionary))
  {
    op->setDictionaryName(dictionary);
  }
  std::string key;
  if(getOptionalAttribute(attributes, "key", key))
  {
    op->setKey(key);
    std::string nsKey;
    if(getOptionalAttribute(attributes, "nsKey", nsKey))
    {
      op->setKeyNamespace(nsKey);
    }
  }
  std::string value;
  if(getOptionalAttribute(attributes, "value", value))
  {
    op->setValue(value);
  }
  schemaElements_.top().second->setFieldOp(op);
  // is this push necessary?
  schemaElements_.push(StackEntry(tag, op));
}


//////////////////////////////////////
/// XML Template Parser Implementation

XMLTemplateParser::XMLTemplateParser()
: out_(0)
{
  // This can throw an XMLException
  XMLPlatformUtils::Initialize();
}

XMLTemplateParser::~XMLTemplateParser()
{
  XMLPlatformUtils::Terminate();
}

TemplateRegistryPtr
XMLTemplateParser::parse(
  std::istream& xmlData
  )
{
  if(!xmlData.good())
  {
    throw TemplateDefinitionError("Can't read XML templates.");
  }

  TemplateRegistryPtr templateRegistry(
    new TemplateRegistry
    );

  TemplateBuilder templateBuilder(templateRegistry, out_);
  boost::shared_ptr<SAX2XMLReader> reader(XMLReaderFactory::createXMLReader());
  reader->setContentHandler(&templateBuilder);
  reader->setErrorHandler(&templateBuilder);
  reader->setFeature(xercesc::XMLUni::fgXercesSchema, true);

  // load external DTDs
  reader->setFeature(xercesc::XMLUni::fgXercesLoadExternalDTD, true);

  // enable validation
//  reader->setFeature(xercesc::XMLUni::fgSAX2CoreValidation, true);

  xmlData.seekg(0, std::ios::end);
  int length = xmlData.tellg();
  xmlData.seekg(0, std::ios::beg);

  boost::scoped_array<char> data(new char[length]);
  xmlData.read(data.get(), length);

  // Adapt the stream to what Xerces wants to see
  MemBufInputSource dataAdapter(
    reinterpret_cast<unsigned char*>(data.get()),
    length,
    "FAST"
    );

  // Parse; throws an exception on failure
  reader->parse(dataAdapter);

  return templateRegistry;
}

void
XMLTemplateParser::setVerboseOutput(std::ostream & out)
{
  out_ = &out;
}
