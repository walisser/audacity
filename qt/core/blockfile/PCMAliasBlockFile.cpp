/**********************************************************************

  Audacity: A Digital Audio Editor

  PCMAliasBlockFile.cpp

  Joshua Haberman

**********************************************************************/

//#include "../Audacity.h"
#include "PCMAliasBlockFile.h"

//#include <wx/file.h>
//#include <wx/utils.h>
//#include <wx/wxchar.h>
//#include <wx/log.h>

#include <sndfile.h>

//#include "../AudacityApp.h"
//#include "../FileFormats.h"
//#include "../Internat.h"
//#include "../MemoryX.h"

//#include "../ondemand/ODManager.h"
//#include "../AudioIO.h"
#include "core/xml/XMLWriter.h"
#include "core/xml/XMLTagHandler.h"
#include "core/DirManager.h"
//extern AudioIO *gAudioIO;

PCMAliasBlockFile::PCMAliasBlockFile(
      const QString &fileName,
      const QString &aliasedFileName,
      sampleCount aliasStart,
      size_t aliasLen, int aliasChannel)
: AliasBlockFile{ fileName, aliasedFileName,
                  aliasStart, aliasLen, aliasChannel }
{
   AliasBlockFile::WriteSummary();
}

PCMAliasBlockFile::PCMAliasBlockFile(
      const QString &fileName,
      const QString &aliasedFileName,
      sampleCount aliasStart,
      size_t aliasLen, int aliasChannel,bool writeSummary)
: AliasBlockFile{ fileName, aliasedFileName,
                  aliasStart, aliasLen, aliasChannel }
{
   if(writeSummary)
      AliasBlockFile::WriteSummary();
}

PCMAliasBlockFile::PCMAliasBlockFile(
      const QString &existingSummaryFileName,
      const QString &aliasedFileName,
      sampleCount aliasStart,
      size_t aliasLen, int aliasChannel,
      float min, float max, float rms)
: AliasBlockFile{ existingSummaryFileName, aliasedFileName,
                  aliasStart, aliasLen,
                  aliasChannel, min, max, rms }
{
}

PCMAliasBlockFile::~PCMAliasBlockFile()
{
}

/// Reads the specified data from the aliased file, using libsndfile,
/// and converts it to the given sample format.
///
/// @param data   The buffer to read the sample data into.
/// @param format The format to convert the data into
/// @param start  The offset within the block to begin reading
/// @param len    The number of samples to read
size_t PCMAliasBlockFile::ReadData(samplePtr data, sampleFormat format,
                                size_t start, size_t len, bool mayThrow) const
{
   auto locker = LockForRead();

   if (mAliasedFileName.isEmpty()){ // intentionally silenced
      memset(data, 0, SAMPLE_SIZE(format) * len);
      return len;
   }

   return CommonReadData( mayThrow,
      mAliasedFileName, mSilentAliasLog, this, mAliasStart, mAliasChannel,
      data, format, start, len);
}

/// Construct a NEW PCMAliasBlockFile based on this one, but writing
/// the summary data to a NEW file.
///
/// @param newFileName The filename to copy the summary data to.
BlockFilePtr PCMAliasBlockFile::Copy(const QString &newFileName)
{
   auto newBlockFile = make_blockfile<PCMAliasBlockFile>
      (newFileName, mAliasedFileName,//wxFileNameWrapper{mAliasedFileName},
       mAliasStart, mLen, mAliasChannel, mMin, mMax, mRMS);

   return newBlockFile;
}

void PCMAliasBlockFile::SaveXML(XMLWriter &xmlFile)
// may throw
{
   xmlFile.StartTag(wxT("pcmaliasblockfile"));

   xmlFile.WriteAttr(wxT("summaryfile"), QFileInfo(mFileName).fileName());//-- mFileName.GetFullName());
   xmlFile.WriteAttr(wxT("aliasfile"), mAliasedFileName); //-- mAliasedFileName.GetFullPath());
   xmlFile.WriteAttr(wxT("aliasstart"),
                     mAliasStart.as_long_long());
   xmlFile.WriteAttr(wxT("aliaslen"), mLen);
   xmlFile.WriteAttr(wxT("aliaschannel"), mAliasChannel);
   xmlFile.WriteAttr(wxT("min"), mMin);
   xmlFile.WriteAttr(wxT("max"), mMax);
   xmlFile.WriteAttr(wxT("rms"), mRMS);

   xmlFile.EndTag(wxT("pcmaliasblockfile"));
}

// BuildFromXML methods should always return a BlockFile, not NULL,
// even if the result is flawed (e.g., refers to nonexistent file),
// as testing will be done in DirManager::ProjectFSCK().
BlockFilePtr PCMAliasBlockFile::BuildFromXML(DirManager &dm, const QStringMap &attrs)
{
   QString summaryFileName;
   QString aliasFileName;
   int aliasStart=0, aliasLen=0, aliasChannel=0;
   float min = 0.0f, max = 0.0f, rms = 0.0f;
   double dblValue;
   long nValue;
   long long nnValue;

   //while(*attrs)
   for (auto it=attrs.constBegin();
        it != attrs.constEnd();
        ++it)
   {
      //const wxChar *attr =  *attrs++;
      //const wxChar *value = *attrs++;
      //if (!value)
      //   break;
      const QString &key = it.key();
      const QString &value = it.value();

      bool ok = false;

      //const wxString strValue = value;
      if (key == wxT("summaryfile") &&
            // Can't use XMLValueChecker::IsGoodFileName here, but do part of its test.
            XMLValueChecker::IsGoodFileString(value) &&
            (value.length() + 1 + dm.GetProjectDataDir().length() <= PLATFORM_MAX_PATH))
      {
         if (!dm.AssignFile(summaryFileName, value, false))
            // Make sure summaryFileName is back to uninitialized state so we can detect problem later.
            summaryFileName = QString();
      }
      else if (key == wxT("aliasfile"))
      {
         if (XMLValueChecker::IsGoodPathName(value))
            aliasFileName = value;
         else if (XMLValueChecker::IsGoodFileName(value, dm.GetProjectDataDir()))
            // Allow fallback of looking for the file name, located in the data directory.
            aliasFileName = dm.GetProjectDataDir() + PLATFORM_PATH_SEPARATOR + value;//.Assign(dm.GetProjectDataDir(), value);
         else if (XMLValueChecker::IsGoodPathString(value))
            // If the aliased file is missing, we failed XMLValueChecker::IsGoodPathName()
            // and XMLValueChecker::IsGoodFileName, because both do existence tests,
            // but we want to keep the reference to the missing file because it's a good path string.
            aliasFileName = value;
      }
      else if (key == wxT("aliasstart"))
      {
         if (XMLValueChecker::IsGoodInt64(value) &&
             (ok=false, nnValue = value.toLongLong(&ok)) >= 0 &&
             ok)
            aliasStart = nnValue;
      }
      else if (XMLValueChecker::IsGoodInt(value) && (ok=false, nValue=value.toLong(&ok), ok))
      {
         // integer parameters
         if (key == wxT("aliaslen") && nValue >= 0)
            aliasLen = nValue;
         else if (key == wxT("aliaschannel") && XMLValueChecker::IsValidChannel(aliasChannel))
            aliasChannel = nValue;
         else if (key == wxT("min"))
            min = nValue;
         else if (key == wxT("max"))
            max = nValue;
         else if (key == wxT("rms") && nValue >= 0)
            rms = nValue;
      }
      // mchinen: the min/max can be (are?) doubles as well, so handle those cases.
      // Vaughan: The code to which I added the XMLValueChecker checks
      // used wxAtoi to convert the string to an int.
      // So it's possible some prior project formats used ints (?), so am keeping
      // those above, but yes, we need to handle floats.
      else if (XMLValueChecker::IsGoodString(value) && (ok=false, dblValue=value.toDouble(&ok), ok))
      {
         // double parameters
         if (key == wxT("min"))
            min = dblValue;
         else if (key == wxT("max"))
            max = dblValue;
         else if (key == wxT("rms") && dblValue >= 0.0)
            rms = dblValue;
      }
   }

   return make_blockfile<PCMAliasBlockFile>
      (summaryFileName, aliasFileName,
       aliasStart, aliasLen, aliasChannel, min, max, rms);
}

void PCMAliasBlockFile::Recover(void)
{
   WriteSummary();
}

