// Copyright (c) 2009, 2010 Object Computing, Inc.
// All rights reserved.
// See the file license.txt for licensing information.
#pragma once

#pragma unmanaged
#include <Messages/ValueMessageBuilder.h>
#pragma managed

namespace QuickFAST
{
  namespace DotNet
  {
    ref class DNMessageDeliverer;

    class ImplBuilderBase
      : public Messages::ValueMessageBuilder
    {
    public:
      ImplBuilderBase(DNMessageDeliverer ^ deliverer);
      virtual ~ImplBuilderBase();

      void setApplicationType(const std::string & applicationType, const std::string & applicationTypeNamespace = "");

      ///@brief set the least interesting log message level
      /// setting this here avoids unnecessary conversions to System.String
      void setLogLimit(unsigned short limit);

      //////////////////////////
      // Implement ValueMessageBuilder
      virtual const std::string & getApplicationType()const;
      virtual const std::string & getApplicationTypeNs()const;
      virtual void addValue(Messages::FieldIdentityCPtr & identity, ValueType::Type type, const int64 value);
      virtual void addValue(Messages::FieldIdentityCPtr & identity, ValueType::Type type, const uint64 value);
      virtual void addValue(Messages::FieldIdentityCPtr & identity, ValueType::Type type, const int32 value);
      virtual void addValue(Messages::FieldIdentityCPtr & identity, ValueType::Type type, const uint32 value);
      virtual void addValue(Messages::FieldIdentityCPtr & identity, ValueType::Type type, const int16 value);
      virtual void addValue(Messages::FieldIdentityCPtr & identity, ValueType::Type type, const uint16 value);
      virtual void addValue(Messages::FieldIdentityCPtr & identity, ValueType::Type type, const int8 value);
      virtual void addValue(Messages::FieldIdentityCPtr & identity, ValueType::Type type, const uchar value);
      virtual void addValue(Messages::FieldIdentityCPtr & identity, ValueType::Type type, const Decimal& value);
      virtual void addValue(Messages::FieldIdentityCPtr & identity, ValueType::Type type, const unsigned char * value, size_t length);
      virtual Messages::ValueMessageBuilder & startMessage(
        const std::string & applicationType,
        const std::string & applicationTypeNamespace,
        size_t size);
      virtual bool endMessage(Messages::ValueMessageBuilder & messageBuilder);
      virtual bool ignoreMessage(Messages::ValueMessageBuilder & messageBuilder);
      virtual Messages::ValueMessageBuilder & startSequence(
        Messages::FieldIdentityCPtr & identity,
        const std::string & applicationType,
        const std::string & applicationTypeNamespace,
        size_t fieldCount,
        Messages::FieldIdentityCPtr & lengthIdentity,
        size_t length);
      virtual void endSequence(
        Messages::FieldIdentityCPtr & identity,
        Messages::ValueMessageBuilder & sequenceBuilder);
      virtual Messages::ValueMessageBuilder & startSequenceEntry(
        const std::string & applicationType,
        const std::string & applicationTypeNamespace,
        size_t size) ;
      virtual void endSequenceEntry(Messages::ValueMessageBuilder & entry);
      virtual Messages::ValueMessageBuilder & startGroup(
        Messages::FieldIdentityCPtr & identity,
        const std::string & applicationType,
        const std::string & applicationTypeNamespace,
        size_t size) ;
      virtual void endGroup(
        Messages::FieldIdentityCPtr & identity,
        Messages::ValueMessageBuilder & groupBuilder);

      //////////////////////////
      // Implement Logger
      virtual bool wantLog(unsigned short level);
      virtual bool logMessage(unsigned short level, const std::string & logMessage);
      virtual bool reportDecodingError(const std::string & errorMessage);
      virtual bool reportCommunicationError(const std::string & errorMessage);

    protected:
      std::string applicationType_;
      std::string applicationTypeNs_;

      unsigned short logLimit_;
      gcroot<DNMessageDeliverer ^> deliverer_;
    };
  }
}