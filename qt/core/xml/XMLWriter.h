/**********************************************************************

  Audacity: A Digital Audio Editor

  XMLWriter.h

  Leland Lucius

**********************************************************************/
#ifndef __AUDACITY_XML_XML_FILE_WRITER__
#define __AUDACITY_XML_XML_FILE_WRITER__

//#include <wx/arrstr.h>
//#include <wx/dynarray.h>
//#include <wx/ffile.h>

#include "../Audacity.h"
#include <type_traits>

///
/// XMLWriter
///
class AUDACITY_DLL_API XMLWriter /* not final */ {

 public:
   XMLWriter();
   virtual ~XMLWriter();

   virtual void StartTag(const QString &name);
   virtual void EndTag(const QString &name);

   // NOTE: WriteAttr() is only overridden by AutoRecovery...
   virtual void WriteAttr(const QString &name, const QString &value);
   //--virtual void WriteAttr(const QString &name, const wxChar *value);
   virtual void WriteAttr(const QString &name, const char *value);

   virtual void WriteAttr(const QString &name, bool value);
   virtual void WriteAttr(const QString &name, int value); // usually 4
   virtual void WriteAttr(const QString &name, long value); // 4 or 8 bytes
   virtual void WriteAttr(const QString &name, long long value); // 8 bytes
   virtual void WriteAttr(const QString &name, unsigned long long value); // 8 bytes

   //virtual void WriteAttr(const QString &name, size_t value); // 4 or 8 bytes usually
   virtual void WriteAttr(const QString &name, float value, int digits = -1);
   virtual void WriteAttr(const QString &name, double value, int digits = -1);

   // some more types to shutup compiler
   void WriteAttr(const QString &name, unsigned int value) {
      static_assert(sizeof(unsigned int) <= sizeof(unsigned long long), "illegal narrowing conversion");
      WriteAttr(name, (unsigned long long)value);
   }
   void WriteAttr(const QString &name, unsigned long int value) {
      static_assert(sizeof(long unsigned int) <= sizeof(unsigned long long), "illegal narrowing conversion");
      WriteAttr(name, (unsigned long long)value);
   }

   virtual void WriteData(const QString &value);

   virtual void WriteSubTree(const QString &value);

   virtual void Write(const QString &data) = 0;

   // Escape a string, replacing certain characters with their
   // XML encoding, i.e. '<' becomes '&lt;'
   static QString XMLEsc(const QString & s);

 protected:
   bool mInTag;
   int mDepth;
   //--wxArrayString mTagstack;
   //--wxArrayInt mHasKids;
   QStringList mTagstack;
   QList<bool> mHasKids;

};

///
/// XMLFileWriter
///

/// This writes to a provisional file, and replaces the previously existing
/// contents by a file rename in Commit() only after all writes succeed.
/// The original contents may also be retained at a backup path name, as
/// directed by the optional constructor argument.
/// If it is destroyed before Commit(), then the provisional file is removed.
/// If the construction and all operations are inside a GuardedCall or event
/// handler, then the default delayed handler action in case of exceptions will
/// notify the user of problems.
class AUDACITY_DLL_API XMLFileWriter final : /**--private wxFFile,**/ public XMLWriter {

 public:

   /// The caption is for message boxes to show in case of errors.
   /// Might throw.
   XMLFileWriter
      ( const QString &outputPath, const QString &caption,
        bool keepBackup = false );

   virtual ~XMLFileWriter();

   /// Close all tags and then close the file.
   /// Might throw.  If not, then create
   /// or modify the file at the output path.
   /// Composed of two steps, PreCommit() and PostCommit()
   void Commit();

   /// Write to file. Might throw.
   void Write(const QString &data) override;

   //--QString GetBackupName() const { return mBackupName; }

 private:
   /// Does the part of Commit that might fail because of exhaustion of space
   void PreCommit();

   /// Does other parts of Commit that are not likely to fail for exhaustion
   /// of space, but might for other reasons
   void PostCommit();

   void ThrowException(const QString &fileName, const QString &caption);

   /// Close file without automatically ending tags.
   /// Might throw.
   void CloseWithoutEndingTags(); // for auto-save files


   const QString mOutputPath;
   const QString mCaption;
   QString mBackupName;
   const bool mKeepBackup;

   QSaveFile mFile;
   //--wxFFile mBackupFile;

   bool mCommitted{ false };
};

///
/// XMLStringWriter
///
class XMLStringWriter final : public XMLWriter {

 public:

   XMLStringWriter() {}
   //--XMLStringWriter(size_t initialSize = 0);
   virtual ~XMLStringWriter() {}

   void Write(const QString &data) override;

   // get the raw UTF-8 data written
   const QByteArray& Get() const { return mData; }

 private:
   QByteArray mData;
};

#endif
