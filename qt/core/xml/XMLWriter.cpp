/**********************************************************************

  Audacity: A Digital Audio Editor

  XMLWriter.cpp

  Leland Lucius

*******************************************************************//**

\class XMLWriter
\brief Base class for XMLFileWriter and XMLStringWriter that provides
the general functionality for creating XML in UTF8 encoding.

*//****************************************************************//**

\class XMLFileWriter
\brief Wrapper to output XML data to files.

*//****************************************************************//**

\class XMLStringWriter
\brief Wrapper to output XML data to strings.

*//*******************************************************************/

//#include "../Audacity.h"
//#include <wx/defs.h>
//#include <wx/ffile.h>
//#include <wx/intl.h>

//#include <string.h>

#include "XMLWriter.h"
//#include "../Internat.h"
#include "../FileException.h"

//table for xml encoding compatibility with expat decoding
//see wxWidgets-2.8.12/src/expat/lib/xmltok_impl.h
//and wxWidgets-2.8.12/src/expat/lib/asciitab.h
static int charXMLCompatiblity[] =
  {

/* 0x00 */ 0, 0, 0, 0,
/* 0x04 */ 0, 0, 0, 0,
/* 0x08 */ 0, 1, 1, 0,
/* 0x0C */ 0, 1, 0, 0,
/* 0x10 */ 0, 0, 0, 0,
/* 0x14 */ 0, 0, 0, 0,
/* 0x18 */ 0, 0, 0, 0,
/* 0x1C */ 0, 0, 0, 0,
  };

/**--
// These are used by XMLEsc to handle surrogate pairs and filter invalid characters outside the ASCII range.
#define MIN_HIGH_SURROGATE static_cast<wxUChar>(0xD800)
#define MAX_HIGH_SURROGATE static_cast<wxUChar>(0xDBFF)
#define MIN_LOW_SURROGATE static_cast<wxUChar>(0xDC00)
#define MAX_LOW_SURROGATE static_cast<wxUChar>(0xDFFF)

// Unicode defines other noncharacters, but only these two are invalid in XML.
#define NONCHARACTER_FFFE static_cast<wxUChar>(0xFFFE)
#define NONCHARACTER_FFFF static_cast<wxUChar>(0xFFFF)
--**/


static void WriteAttrString(XMLWriter& w, const QString& name, const QString& value) {
   w.Write(QString(" %1=\"%2\"").arg(name).arg(w.XMLEsc(value)));
}

template<typename T>
static void WriteAttrScalar(XMLWriter& w, const QString& name, T value) {
   static_assert(!std::is_convertible<T, QString>::value, "String types must be escaped");
   w.Write(QString(" %1=\"%2\"").arg(name).arg(value));
}

static void WriteAttrDouble(XMLWriter& w, const QString& name, double value, int digits) {
   w.Write(QString(" %1=\"%2\"").arg(name).arg(value, 0, 'f', digits));
}


///
/// XMLWriter base class
///
XMLWriter::XMLWriter()
{
   mDepth = 0;
   mInTag = false;
   mHasKids.append(false);
}

XMLWriter::~XMLWriter()
{
}

void XMLWriter::StartTag(const QString &name)
// may throw
{
   int i;

   if (mInTag) {
      Write(wxT(">\n"));
      mInTag = false;
   }

   for (i = 0; i < mDepth; i++) {
      Write(wxT("\t"));
   }

   //--Write(wxString::Format(wxT("<%s"), name.c_str()));
   Write(QString(wxT("<%1")).arg(name));

   mTagstack.insert(0, name);
   mHasKids[0] = true;
   mHasKids.insert(0, false);
   mDepth++;
   mInTag = true;
}

void XMLWriter::EndTag(const QString &name)
// may throw
{
   int i;

   if (mTagstack.count() > 0) {
      if (mTagstack[0] == name) {
         if (mHasKids[1]) {  // There will always be at least 2 at this point
            if (mInTag) {
               Write(wxT("/>\n"));
            }
            else {
               for (i = 0; i < mDepth - 1; i++) {
                  Write(wxT("\t"));
               }
               //--Write(wxString::Format(wxT("</%s>\n"), name.c_str()));
               Write(QString(wxT("</%1>\n")).arg(name));
            }
         }
         else {
            Write(wxT(">\n"));
         }
         mTagstack.removeAt(0);
         mHasKids.removeAt(0);
      }
   }

   mDepth--;
   mInTag = false;
}

void XMLWriter::WriteAttr(const QString &name, const QString &value)
// may throw from Write()
{
   WriteAttrString(*this, name, value);
}

//--void XMLWriter::WriteAttr(const QString &name, const wxChar *value)
//-- may throw from Write()
//--{
//--   WriteAttr(name, QString(value));
//--}
void XMLWriter::WriteAttr(const QString &name, const char* value)
// may throw from Write()
{
   WriteAttrString(*this, name, value);
}

void XMLWriter::WriteAttr(const QString &name, int value)
// may throw from Write()
{
   WriteAttrScalar(*this, name, value);
}

void XMLWriter::WriteAttr(const QString &name, bool value)
// may throw from Write()
{
   WriteAttrScalar(*this, name, value);
}

void XMLWriter::WriteAttr(const QString &name, long value)
// may throw from Write()
{
   WriteAttrScalar(*this, name, value);
}

void XMLWriter::WriteAttr(const QString &name, long long value)
// may throw from Write()
{
   WriteAttrScalar(*this, name, value);
}

void XMLWriter::WriteAttr(const QString &name, unsigned long long value)
// may throw from Write()
{
   WriteAttrScalar(*this, name, value);
}

void XMLWriter::WriteAttr(const QString &name, float value, int digits)
// may throw from Write()
{
   WriteAttrDouble(*this, name, value, digits);
}

void XMLWriter::WriteAttr(const QString &name, double value, int digits)
// may throw from Write()
{
   WriteAttrDouble(*this, name, value, digits);

}

void XMLWriter::WriteData(const QString &value)
// may throw from Write()
{
   int i;

   for (i = 0; i < mDepth; i++) {
      Write(wxT("\t"));
   }

   Write(XMLEsc(value));
}

void XMLWriter::WriteSubTree(const QString &value)
// may throw from Write()
{
   if (mInTag) {
      Write(wxT(">\n"));
      mInTag = false;
      mHasKids[0] = true;
   }

   //Write(value.c_str());
   Write(value);
}

// See http://www.w3.org/TR/REC-xml for reference
QString XMLWriter::XMLEsc(const QString & s)
{
   QString result;
   const int len = s.length();

   for (int i=0; i < len; i++) {
      //--wxUChar c = s.GetChar(i);
      const QChar c = s[i];
      const int  lc = c.toLatin1();

      switch (lc) {
         case wxT('\''):
            result += wxT("&apos;");
            break;

         case wxT('"'):
            result += wxT("&quot;");
            break;

         case wxT('&'):
            result += wxT("&amp;");
            break;

         case wxT('<'):
            result += wxT("&lt;");
            break;

         case wxT('>'):
            result += wxT("&gt;");
            break;

         default:
            // unpaired unicode surrogates are not allowed in element or attribute names, remove them
            //--if (sizeof(c) == 2 && c >= MIN_HIGH_SURROGATE && c <= MAX_HIGH_SURROGATE && i < len - 1) {
            if (c.isHighSurrogate() && i < len - 1) {
               // If wxUChar is 2 bytes, then supplementary characters (those greater than U+FFFF) are represented
               // with a high surrogate (U+D800..U+DBFF) followed by a low surrogate (U+DC00..U+DFFF).
               // Handle those here.
               //--wxUChar c2 = s.GetChar(++i);
               const QChar c2 = s[++i];

               //if (c2 >= MIN_LOW_SURROGATE && c2 <= MAX_LOW_SURROGATE) {
               if (c2.isLowSurrogate()) {
                  // Surrogate pair found; simply add it to the output string.
                  result += c;
                  result += c2;
               }
               else {
                  // That high surrogate isn't paired, so ignore it.
                  i--;
               }
            }
            //--else if (!wxIsprint(c)) {
            else if (!c.isPrint()) {
               //ignore several characters such ase eot (0x04) and stx (0x02) because it makes expat parser bail
               //see xmltok.c in expat checkCharRefNumber() to see how expat bails on these chars.
               //also see wxWidgets-2.8.12/src/expat/lib/asciitab.h to see which characters are nonxml compatible
               //post decode (we can still encode '&' and '<' with this table, but it prevents us from encoding eot)
               //everything is compatible past ascii 0x20 except for surrogates and the noncharacters U+FFFE and U+FFFF,
               //so we don't check the compatibility table higher than this.
               int u = c.unicode();
               if ( (lc > 0x1F || charXMLCompatiblity[lc] != 0) && // expat compatible
                     !c.isSurrogate() && //--(c < MIN_HIGH_SURROGATE || c > MAX_LOW_SURROGATE) &&
                     u != 0xFFFE &&
                     u != 0xFFFF)
                  //result += wxString::Format(wxT("&#x%04x;"), u);
                  result += QString(wxT("&#x%1;")).arg(u, 4, 16, QLatin1Char('0'));
            }
            else {
               result += c;
            }
         break;
      }
   }

   return result;
}

///
/// XMLFileWriter class
///
XMLFileWriter::XMLFileWriter
   ( const QString &outputPath, const QString &caption, bool keepBackup )
   : mOutputPath{ outputPath }
   , mCaption{ caption }
   , mKeepBackup{ keepBackup }
   , mFile{ outputPath }
// may throw
{
   if ( !mFile.open(QFile::WriteOnly) )
      ThrowException( mFile.fileName(), mCaption );

   /**--
   auto tempPath = wxFileName::CreateTempFileName( outputPath );
   if (!wxFFile::Open(tempPath, wxT("wb")) || !IsOpened())
      ThrowException( tempPath, mCaption );

   if (mKeepBackup) {
      int index = 0;
      QString backupName;

      do {
         wxFileName outputFn{ mOutputPath };
         index++;
         mBackupName =
         outputFn.GetPath() + wxFILE_SEP_PATH +
         outputFn.GetName() + wxT("_bak") +
         wxString::Format(wxT("%d"), index) + wxT(".") +
         outputFn.GetExt();
      } while( ::wxFileExists( mBackupName ) );

      // Open the backup file to be sure we can write it and reserve it
      // until committing
      if (! mBackupFile.Open( mBackupName, "wb" ) || ! mBackupFile.IsOpened() )
         ThrowException( mBackupName, mCaption );
   }
   --**/
}


XMLFileWriter::~XMLFileWriter()
{
   // Don't let a destructor throw!
   GuardedCall( [&] {
      if (!mCommitted) {
         //--auto fileName = GetName();
         //--if ( IsOpened() )
         if (mFile.isOpen())
            CloseWithoutEndingTags();
         //--::wxRemoveFile( fileName );
      }
   } );
}

void XMLFileWriter::Commit()
// may throw
{
   PreCommit();
   PostCommit();
}

void XMLFileWriter::PreCommit()
// may throw
{
   while (mTagstack.count()) {
      EndTag(mTagstack[0]);
   }

   CloseWithoutEndingTags();
}

void XMLFileWriter::PostCommit()
// may throw
{
   if (!mFile.commit())
   {
      qWarning("QSaveFile::commit() failed");
      ThrowException(mOutputPath, mCaption);
   }

/**--
   auto tempPath = GetName();
   if (mKeepBackup) {
      if (! mBackupFile.Close() ||
          ! wxRenameFile( mOutputPath, mBackupName ) )
         ThrowException( mBackupName, mCaption );
   }
   else {
      if ( wxFileName::FileExists( mOutputPath ) &&
           ! wxRemoveFile( mOutputPath ) )
         ThrowException( mOutputPath, mCaption );
   }

   // Now we have vacated the file at the output path and are committed.
   // But not completely finished with steps of the commit operation.
   // If this step fails, we haven't lost the successfully written data,
   // but just failed to put it in the right place.
   if (! wxRenameFile( tempPath, mOutputPath ) )
      throw FileException{
         FileException::Cause::Rename, tempPath, mCaption, mOutputPath
      };
--**/

   mCommitted = true;
}

void XMLFileWriter::ThrowException(const QString &fileName, const QString &caption)
{
   throw FileException{ FileException::Cause::Write, fileName, caption };
}

void XMLFileWriter::CloseWithoutEndingTags()
// may throw
{
   // Before closing, we first flush it, because if Flush() fails because of a
   // "disk full" condition, we can still at least try to close the file.
   //--if (!wxFFile::Flush())
   if (!mFile.flush())
   {
      //--wxFFile::Close();
      qWarning("QSaveFile::flush() failed");
      ThrowException( mFile.fileName(), mCaption );
   }

   // Note that this should never fail if flushing worked.
   //--if (!wxFFile::Close())
   //--   ThrowException( GetName(), mCaption );
}

void XMLFileWriter::Write(const QString &data)
// may throw
{
   //if (!wxFFile::Write(data, wxConvUTF8) || Error())
   if (!mFile.write(data.toUtf8()))
   {
      // When writing fails, we try to close the file before throwing the
      // exception, so it can at least be deleted.
      //--wxFFile::Close();
      ThrowException( mFile.fileName(), mCaption );
   }
}

///
/// XMLStringWriter class
///
void XMLStringWriter::Write(const QString &data)
{
   //qDebug("write:%s", qPrintable(data));
   //--Append(data);
   mData += data.toUtf8();
}
