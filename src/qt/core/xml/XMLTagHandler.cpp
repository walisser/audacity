/**********************************************************************

  Audacity: A Digital Audio Editor

  XMLTagHandler.cpp

  Dominic Mazzoni
  Vaughan Johnson


*//****************************************************************//**

\class XMLTagHandler
\brief This class is an interface which should be implemented by
  classes which wish to be able to load and save themselves
  using XML files.

\class XMLValueChecker
\brief XMLValueChecker implements static bool methods for checking
  input values from XML files.

*//*******************************************************************/

//#include "../Audacity.h"
#include "XMLTagHandler.h"

//#include "../MemoryX.h"
//#include "../Internat.h"

//#ifdef _WIN32
//   #include <windows.h>
//   #include <wx/msw/winundef.h>
//#endif

//#include <wx/defs.h>
//#include <wx/arrstr.h>
//#include <wx/filename.h>

#include "../SampleFormat.h"
#include "../Track.h"

bool XMLValueChecker::IsGoodString(const QString & str)
{
   //--size_t len = str.length();
   //--int nullIndex = str.Find('\0', false);
   
   // Shouldn't be any reason for longer strings, except intentional file corruption.
   // No null characters except terminator.
   if ((str.length() <= PLATFORM_MAX_PATH) &&
      str.count(QChar())==0)      
      //--(nullIndex == -1))
      return true;
   else
      return false; // good place for a breakpoint
}

// "Good" means the name is well-formed and names an existing file or folder.
bool XMLValueChecker::IsGoodFileName(const QString & strFileName, const QString & strDirName /* = "" */)
{
   // Test strFileName.
   if (!IsGoodFileString(strFileName) ||
         (strDirName.length() + 1 + strFileName.length() > PLATFORM_MAX_PATH))
      return false;

   // Test the corresponding wxFileName.
   //--wxFileName fileName(strDirName, strFileName);
   //--return (fileName.IsOk() && fileName.FileExists());

   QFileInfo info(strDirName, strFileName);
   
   return info.exists();
}

bool XMLValueChecker::IsGoodFileString(const QString &str)
{
   return (IsGoodString(str) &&
            !str.isEmpty() &&

            // FILENAME_MAX is 260 in MSVC, but inconsistent across platforms,
            // sometimes huge, but we use 260 for all platforms.
            (str.length() <= 260) &&

            (!str.contains(PLATFORM_PATH_SEPARATOR)));//(str.find(wxFileName::GetPathSeparator()) == -1)); // No path separator characters.
}

bool XMLValueChecker::IsGoodSubdirName(const QString & strSubdirName, const QString & strDirName /* = "" */)
{
   // Test strSubdirName.
   // Note this prevents path separators, and relative path to parents (strDirName),
   // so fixes vulnerability #3 in the NGS report for UmixIt,
   // where an attacker could craft an AUP file with relative pathnames to get to system files, for example.
   if (!IsGoodFileString(strSubdirName) ||
         (strSubdirName == ".") || (strSubdirName == "..") ||
         (strDirName.length() + 1 + strSubdirName.length() > PLATFORM_MAX_PATH))
      return false;

   // Test the corresponding wxFileName.
   //--wxFileName fileName(strDirName, strSubdirName);
   //--return (fileName.IsOk() && fileName.DirExists());
   QFileInfo info(strDirName, strSubdirName);
   return info.dir().exists();
}

bool XMLValueChecker::IsGoodPathName(const QString & strPathName)
{
   // Test the corresponding wxFileName.
   //--wxFileName fileName(strPathName);
   //--return XMLValueChecker::IsGoodFileName(fileName.GetFullName(), fileName.GetPath(wxPATH_GET_VOLUME));

   QFileInfo info(strPathName);
   return XMLValueChecker::IsGoodFileName(info.fileName(), info.path());
}

bool XMLValueChecker::IsGoodPathString(const QString &str)
{
   return (IsGoodString(str) &&
            !str.isEmpty() &&
            (str.length() <= PLATFORM_MAX_PATH));
} 

bool XMLValueChecker::IsGoodInt(const QString & str)
{
   if (!IsGoodString(str))
      return false;

   QByteArray strInt = str.toUtf8();
   
   // Check that the value won't overflow.
   // Signed long: -2,147,483,648 to +2,147,483,647, i.e., -2^31 to 2^31-1
   // We're strict about disallowing spaces and commas, and requiring minus sign to be first char for negative.
   const size_t lenMAXABS = strlen("2147483647");
   const size_t lenStrInt = strInt.length();

   if (lenStrInt > (lenMAXABS + 1))
      return false;
   else if ((lenStrInt == (lenMAXABS + 1)) && (strInt[0] == '-'))
   {
      const int digitsMAXABS[] = {2, 1, 4, 7, 4, 8, 3, 6, 4, 9};
      unsigned int i;
      for (i = 0; i < lenMAXABS; i++)
         if (strInt[i+1] < '0' || strInt[i+1] > '9')
            return false; // not a digit

      for (i = 0; i < lenMAXABS; i++)
         if (strInt[i+1] - '0' < digitsMAXABS[i])
            return true; // number is small enough
      return false;
   }
   else if (lenStrInt == lenMAXABS)
   {
      const int digitsMAXABS[] = {2, 1, 4, 7, 4, 8, 3, 6, 4, 8};
      unsigned int i;
      for (i = 0; i < lenMAXABS; i++)
         if (strInt[i] < '0' || strInt[i+1] > '9')
            return false; // not a digit
      for (i = 0; i < lenMAXABS; i++)
         if (strInt[i] - '0' < digitsMAXABS[i])
            return true; // number is small enough
      return false;
   }
   return true;
}

bool XMLValueChecker::IsGoodInt64(const QString & str)
{
   if (!IsGoodString(str))
      return false;

   QByteArray strInt = str.toUtf8();
      
   // Check that the value won't overflow.
   // Signed 64-bit: -9223372036854775808 to +9223372036854775807, i.e., -2^63 to 2^63-1
   // We're strict about disallowing spaces and commas, and requiring minus sign to be first char for negative.
   const size_t lenMAXABS = strlen("9223372036854775807");
   const size_t lenStrInt = strInt.length();

   if (lenStrInt > (lenMAXABS + 1))
      return false;
   else if ((lenStrInt == (lenMAXABS + 1)) && (strInt[0] == '-'))
   {
      const int digitsMAXABS[] = {9,2,2,3,3,7,2,0,3,6,8,5,4,7,7,5,8,0,9};
      unsigned int i;
      for (i = 0; i < lenMAXABS; i++)
         if (strInt[i+1] < '0' || strInt[i+1] > '9')
            return false; // not a digit

      for (i = 0; i < lenMAXABS; i++)
         if (strInt[i+1] - '0' < digitsMAXABS[i])
            return true; // number is small enough
      return false;
   }
   else if (lenStrInt == lenMAXABS)
   {
      const int digitsMAXABS[] = {9,2,2,3,3,7,2,0,3,6,8,5,4,7,7,5,8,0,8};
      unsigned int i;
      for (i = 0; i < lenMAXABS; i++)
         if (strInt[i] < '0' || strInt[i+1] > '9')
            return false; // not a digit
      for (i = 0; i < lenMAXABS; i++)
         if (strInt[i] - '0' < digitsMAXABS[i])
            return true; // number is small enough
      return false;
   }
   return true;
}

bool XMLValueChecker::IsValidChannel(const int nValue)
{
   return (nValue >= Track::LeftChannel) && (nValue <= Track::MonoChannel);
}

#ifdef USE_MIDI
bool XMLValueChecker::IsValidVisibleChannels(const int nValue)
{
    return (nValue >= 0 && nValue < (1 << 16));
}
#endif

bool XMLValueChecker::IsValidSampleFormat(const int nValue)
{
   return (nValue == int16Sample) || (nValue == int24Sample) || (nValue == floatSample);
}

bool XMLTagHandler::ReadXMLTag(const char *tag, const char **attrs)
{
   //wxArrayString tmp_attrs;

   //while (*attrs) {
   //   const char *s = *attrs++;
   //   tmp_attrs.Add(UTF8CTOWX(s));
   //}
   
// JKC: Previously the next line was:
// const char **out_attrs = NEW char (const char *)[tmp_attrs.GetCount()+1];
// however MSVC doesn't like the constness in this position, so this is now
// added by a cast after creating the array of pointers-to-non-const chars.
   //auto out_attrs = std::make_unique<const wxChar *[]>(tmp_attrs.GetCount() + 1);
   //for (size_t i=0; i<tmp_attrs.GetCount(); i++) {
   //   out_attrs[i] = tmp_attrs[i].c_str();
   // }
   //out_attrs[tmp_attrs.GetCount()] = 0;

   //bool result = HandleXMLTag(UTF8CTOWX(tag).c_str(), out_attrs.get());

   QStringMap map;
   
   while (*attrs) {
      const char *key = *attrs++;
      Q_ASSERT(attrs);
      const char *value = *attrs++;
      map[ QString::fromUtf8(key) ] = QString::fromUtf8(value);
   }
   
   return HandleXMLTag(QString::fromUtf8(tag), map);
}

void XMLTagHandler::ReadXMLEndTag(const char *tag)
{
   HandleXMLEndTag(QString::fromUtf8(tag));//UTF8CTOWX(tag).c_str());
}

void XMLTagHandler::ReadXMLContent(const char *s, int len)
{
   HandleXMLContent(QString::fromUtf8(s, len));//(s, wxConvUTF8, len));
}

XMLTagHandler *XMLTagHandler::ReadXMLChild(const char *tag)
{
   return HandleXMLChild(QString::fromUtf8(tag));//UTF8CTOWX(tag).c_str());
}
