/**********************************************************************

  Audacity: A Digital Audio Editor

  XMLFileReader.h

  Dominic Mazzoni

**********************************************************************/
#pragma once
//#include "../Audacity.h"

#include <vector>
//#include "expat.h"
struct XML_ParserStruct;
typedef struct XML_ParserStruct *XML_Parser;

//#include "XMLTagHandler.h"
class XMLTagHandler;

class AUDACITY_DLL_API XMLFileReader final {
 public:
   XMLFileReader();
   ~XMLFileReader();

   bool Parse(XMLTagHandler *baseHandler,
              const QString &fname);

   QString GetErrorStr();

   // Callback functions for expat

   static void startElement(void *userData, const char *name,
                            const char **atts);

   static void endElement(void *userData, const char *name);

   static void charHandler(void *userData, const char *s, int len);

 private:
   XML_Parser       mParser;
   XMLTagHandler   *mBaseHandler;
   using Handlers = std::vector<XMLTagHandler*>;
   Handlers         mHandler;
   QString          mErrorStr;
};
