/**********************************************************************

  Audacity: A Digital Audio Editor

  DirManager.h

  Dominic Mazzoni

**********************************************************************/

#ifndef _DIRMANAGER_
#define _DIRMANAGER_

#include "MemoryX.h"

//#include <wx/list.h>
//#include <wx/string.h>
//#include <wx/filename.h>
//#include <wx/hashmap.h>
//#include <wx/utils.h>

//#include "audacity/Types.h"
#include "xml/XMLTagHandler.h"
//#include "wxFileNameWrapper.h"

class wxHashTable;
class BlockArray;
class BlockFile;
class SequenceTest;

#define FSCKstatus_CLOSE_REQ 0x1
#define FSCKstatus_CHANGED   0x2
#define FSCKstatus_SAVE_AUP  0x4 // used in combination with FSCKstatus_CHANGED

//--WX_DECLARE_HASH_MAP(int, int, wxIntegerHash, wxIntegerEqual, DirHash);
typedef QHash<int, int> DirHash;

class BlockFile;
using BlockFilePtr = std::shared_ptr<BlockFile>;

//--WX_DECLARE_HASH_MAP(QString, std::weak_ptr<BlockFile>, QStringHash,
//--                    QStringEqual, BlockHash);
typedef QHash<QString, std::weak_ptr<BlockFile>> BlockHash;

//--wxMemorySize GetFreeMemory();
uint64_t GetFreeMemory();

enum {
   kCleanTopDirToo = 1,        // The top directory can be excluded from clean.
   kCleanDirsOnlyIfEmpty = 2,  // Otherwise non empty dirs may be removed.
   kCleanFiles = 4,            // Remove files
   kCleanDirs = 8              // Remove dirs.
};


class PROFILE_DLL_API DirManager final : public XMLTagHandler {
 public:

   // MM: Construct DirManager
   DirManager();

   virtual ~DirManager();

   static void SetTempDir(const QString &_temp) { globaltemp = _temp; }

   // Returns true on success.
   // If SetProject is told NOT to create the directory
   // but it doesn't already exist, SetProject fails and returns false.
   bool SetProject(QString& newProjPath, QString& newProjName, const bool bCreate);

   QString GetProjectDataDir();
   QString GetProjectName();

   uint64_t GetFreeDiskSpace();

   BlockFilePtr
      NewSimpleBlockFile(samplePtr sampleData,
                                 size_t sampleLen,
                                 sampleFormat format,
                                 bool allowDeferredWrite = false);

   BlockFilePtr
      NewAliasBlockFile( const QString &aliasedFile, sampleCount aliasStart,
                                 size_t aliasLen, int aliasChannel);

   BlockFilePtr
      NewODAliasBlockFile( const QString &aliasedFile, sampleCount aliasStart,
                                 size_t aliasLen, int aliasChannel);

   BlockFilePtr
      NewODDecodeBlockFile( const QString &aliasedFile, sampleCount aliasStart,
                                 size_t aliasLen, int aliasChannel, int decodeType);

   /// Returns true if the blockfile pointed to by b is contained by the DirManager
   bool ContainsBlockFile(const BlockFile *b) const;
   /// Check for existing using filename using complete filename
   bool ContainsBlockFile(const QString &filepath) const;

   // Adds one to the reference count of the block file,
   // UNLESS it is "locked", then it makes a NEW copy of
   // the BlockFile.
   // May throw an exception in case of disk space exhaustion, otherwise
   // returns non-null.
   BlockFilePtr CopyBlockFile(const BlockFilePtr &b);

   //BlockFile *LoadBlockFile(const wxChar **attrs, sampleFormat format);
   BlockFile *LoadBlockFile(const QStringMap &attrs, sampleFormat format);
   void SaveBlockFile(BlockFile *f, int depth, FILE *fp);

#if LEGACY_PROJECT_FILE_SUPPORT
   BlockFile *LoadBlockFile(wxTextFile * in, sampleFormat format);
   void SaveBlockFile(BlockFile * f, wxTextFile * out);
#endif

   std::pair<bool, QString> CopyToNewProjectDirectory(BlockFile *f);

   bool EnsureSafeFilename(const QString &fName);

   void SetLoadingTarget(BlockArray *pArray, unsigned idx)
   {
      mLoadingTarget = pArray;
      mLoadingTargetIdx = idx;
   }
   void SetLoadingFormat(sampleFormat format) { mLoadingFormat = format; }
   void SetLoadingBlockLength(size_t len) { mLoadingBlockLen = len; }

   // Note: following affects only the loading of block files when opening a project
   void SetLoadingMaxSamples(size_t max) { mMaxSamples = max; }

   //--bool HandleXMLTag(const wxChar *tag, const wxChar **attrs) override;
   bool HandleXMLTag(const QString &tag, const QStringMap &attrs) override;
   
   XMLTagHandler *HandleXMLChild(const QString & tag) override
      { Q_UNUSED(tag); return NULL; }
   bool AssignFile(QString &filename, const QString &value, bool check);

   // Clean the temp dir. Note that now where we have auto recovery the temp
   // dir is not cleaned at start up anymore. But it is cleaned when the
   // program is exited normally.
   static void CleanTempDir();
   static void CleanDir(
      const QString &path, 
      const QString &dirSpec, 
      const QString &fileSpec, 
      const QString &msg,
      int flags = 0);

   // Check the project for errors and possibly prompt user
   // bForceError: Always show log error alert even if no errors are found here.
   //    Important when you know that there are already errors in the log.
   // bAutoRecoverMode: Do not show any option dialogs for how to deal with errors found here.
   //    Too complicated during auto-recover. Just correct problems the "safest" way.
   int ProjectFSCK(const bool bForceError, const bool bAutoRecoverMode);

   void FindMissingAliasedFiles(
         BlockHash& missingAliasedFileAUFHash,     // output: (.auf) AliasBlockFiles whose aliased files are missing
         BlockHash& missingAliasedFilePathHash);   // output: full paths of missing aliased files
   void FindMissingAUFs(
         BlockHash& missingAUFHash);               // output: missing (.auf) AliasBlockFiles
   void FindMissingAUs(
         BlockHash& missingAUHash);                // missing data (.au) blockfiles
   // Find .au and .auf files that are not in the project.
   void FindOrphanBlockFiles(
         const QStringList& filePathArray,       // input: all files in project directory
         QStringList& orphanFilePathArray);      // output: orphan files


   // Remove all orphaned blockfiles without user interaction. This is
   // generally safe, because orphaned blockfiles are not referenced by the
   // project and thus worthless anyway.
   void RemoveOrphanBlockfiles();

   // Get directory where data files are in. Note that projects are normally
   // not interested in this information, but it is important for the
   // auto-save functionality
   QString GetDataFilesDir() const;

   // This should only be used by the auto save functionality
   void SetLocalTempDir(const QString &path);

   // Do not DELETE any temporary files on exit. This is only called if
   // auto recovery is cancelled and should be retried later
   static void SetDontDeleteTempFiles() { dontDeleteTempFiles = true; }

   // Write all write-cached block files to disc, if any
   void WriteCacheToDisk();

   // (Try to) fill cache of blockfiles, if caching is enabled (otherwise do
   // nothing)
   // A no-fail operation that does not throw
   void FillBlockfilesCache();

 private:

   QString MakeBlockFileName();
   QString MakeBlockFilePath(const QString &value);

   BlockHash mBlockFileHash; // repository for blockfiles

   // Hashes for management of the sub-directory tree of _data
   struct BalanceInfo
   {
      DirHash   dirTopPool;    // available toplevel dirs
      DirHash   dirTopFull;    // full toplevel dirs
      DirHash   dirMidPool;    // available two-level dirs
      DirHash   dirMidFull;    // full two-level dirs
   } mBalanceInfo;

   // Accessor for the balance info, may need to do a delayed update for
   // deletion in case other threads DELETE block files
   BalanceInfo &GetBalanceInfo();

   void BalanceInfoDel(const QString&);
   void BalanceInfoAdd(const QString&);
   void BalanceFileAdd(int);
   int BalanceMidAdd(int, int);

   QString projName;
   QString projPath;
   QString projFull;

   QString lastProject;

   QStringList aliasList;

   BlockArray *mLoadingTarget;
   unsigned mLoadingTargetIdx;
   sampleFormat mLoadingFormat;
   size_t mLoadingBlockLen;

   size_t mMaxSamples; // max samples per block

   unsigned long mLastBlockFileDestructionCount { 0 };

   static QString globaltemp;
   QString mytemp;
   static int numDirManagers;
   static bool dontDeleteTempFiles;

   friend class SequenceTest;
};

#endif
