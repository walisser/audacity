#include <QtTest>

#include "core/DirManager.h"
#include "TestBlockFile.h"

//#include "core/blockfile/SimpleBlockFile.h"
//#include "core/blockfile/PCMAliasBlockFile.h"
#include "core/blockfile/ODPCMAliasBlockFile.h"
#include "core/blockfile/ODDecodeBlockFile.h"

#include "core/xml/XMLWriter.h"
#include "core/xml/XMLFileReader.h"
#include "core/Sequence.h"
#include "core/DialogManager.h"

#include "TestHelpers.h"

// A bit unusual, but must do this here because TestBlockFile otherwise defines stubs for DirManager
// All unit tests must use the same build configuration (DEFINES, INCLUDEPATH, etc)
#define TESTING_DIRMANAGER
#include "TestBlockFile.cpp"

// inherit TestBlockFile for some helper functions
class TestDirManager : public TestBlockFile, public XMLTagHandler
{
   Q_OBJECT
private:
   std::unique_ptr<DirManager> _d;
   QString _tmpPath;
   QVector<BlockFilePtr> _blockFiles;
   BlockArray _seqBlocks; // array of SeqBlock

// TestBlockFile wants this, we want to use the methods of DirManager
void buildFromXml(DirManager& dm, const QString& tag, const QStringMap& attrs) override
{
   (void)dm, (void)tag, (void)attrs;
   qFatal("Don't call this");
}

private:

// Reset test members to defaults, empty tmpDir and construct a new DirManager
void resetDirManager()
{
   _blockFiles.clear();
   _seqBlocks.clear();
   _d.reset();

   QDir tmpDir(_tmpPath);
   QString dirName = tmpDir.dirName();
   QVERIFY( tmpDir.removeRecursively() );
   QVERIFY( qDirIsEmpty(tmpDir) );
   QVERIFY( tmpDir.cdUp() && tmpDir.mkdir(dirName) && tmpDir.cd(dirName) );

   _d = std::make_unique<DirManager>();
}

private Q_SLOTS:

void initTestCase()
{
   // set the global tmp dir and reset
   QString tmpName = "dirmanager_tmp";
   _tmpPath = QDir::current().absoluteFilePath(tmpName);
   DirManager::SetTempDir(_tmpPath);
   resetDirManager();
}

void cleanupTestCase()
{
   _d.reset();
   QDir(_tmpPath).removeRecursively();
}

// test state immediately after construction
void testDefaultState()
{
   // these are empty until we call SetProjectName
   QVERIFY( _d->GetProjectDataDir().isEmpty() );
   QVERIFY( _d->GetProjectName().isEmpty() );

   // global temporary dir exists, local tmp does not
   {
      QDir dir(_d->GetDataFilesDir());
      QVERIFY( !dir.exists() );
      dir.cdUp();
      QVERIFY(dir.exists());
   }

   QVERIFY( _d->GetFreeDiskSpace() == QStorageInfo(_tmpPath).bytesAvailable() );
}

void testSetProject()
{
   // empty values
   QVERIFY( ! _d->SetProject("", "", true));

   // illegal/nonexistent parent dir
   QVERIFY( ! _d->SetProject("/:^$#@", "", true));

   // parent ok, empty project name
   QVERIFY( ! _d->SetProject(_tmpPath, "", true));

   // parent ok, illegal filename
   QVERIFY( ! _d->SetProject(_tmpPath, "^$:/#@", true));

   // both ok, but not allowed to create
   QVERIFY( !_d->SetProject(_tmpPath, "myproject", false));

   QVERIFY(_d->SetProject(_tmpPath, "myproject", true));
}

void testNewSimpleBlockFile()
{
   TestData t;
   makeTestData("", t);

   BlockFilePtr b = _d->NewSimpleBlockFile(t.samples.ptr(), t.numSamples, t.fmt);
   QVERIFY(b != nullptr);
   QVERIFY(QFile(b->GetFileName()).exists());

   // keep after destruct to test cleanup function
   b->Lock();
   _blockFiles.append(b);

   QVERIFY(_d->ContainsBlockFile(b.get()));
   QVERIFY(_d->ContainsBlockFile( QFileInfo(b->GetFileName()).baseName()));

   // test deferred write
   // NOTE: actually has no effect since DEPRECATED_AUDIO_CACHE
   b = _d->NewSimpleBlockFile(t.samples.ptr(), t.numSamples, t.fmt, true);
   QVERIFY(b != nullptr);
   QVERIFY(QFile(b->GetFileName()).exists());
   b->Lock();
   _blockFiles.append(b);
}

void testNewAliasBlockFile()
{
   TestData t;
   makeTestData("alias.wav", t);
   BlockFilePtr b = _d->NewAliasBlockFile(t.fileName, 0, t.numSamples, 0);
   QVERIFY(b != nullptr);
   QVERIFY( QFile(b->GetFileName()).exists() );
   b->Lock();

   QVERIFY(_d->ContainsBlockFile(b.get()));
   QVERIFY(_d->ContainsBlockFile( QFileInfo(b->GetFileName()).baseName()));
   _blockFiles.append(b);
}

void testNewODAliasBlockFile()
{
   TestData t;
   makeTestData("alias.flac", t);
   BlockFilePtr b = _d->NewODAliasBlockFile(t.fileName, 0, t.numSamples, 0);
   QVERIFY(b != nullptr);

   // doesn't exist until background thread creates it
   QVERIFY( !QFile(b->GetFileName()).exists() );
   b->Lock();

   QVERIFY(_d->ContainsBlockFile(b.get()));
   QVERIFY(_d->ContainsBlockFile( QFileInfo(b->GetFileName()).baseName()));
   _blockFiles.append(b);
}

void testNewODDecodeBlockFile()
{
   TestData t;
   makeTestData("alias.flac", t);
   BlockFilePtr b = _d->NewODDecodeBlockFile(t.fileName, 0, t.numSamples, 0, 0);
   QVERIFY(b != nullptr);

   // doesn't exist until background thread creates it
   QVERIFY( !QFile(b->GetFileName()).exists() );
   b->Lock();

   QVERIFY(_d->ContainsBlockFile(b.get()));
   QVERIFY(_d->ContainsBlockFile(QFileInfo(b->GetFileName()).baseName()));
   _blockFiles.append(b);
}

void testSetProject_LowDisk()
{
   FileSizeLimiter limiter;
   if (limiter.apply(100))
   {
      QString projName = "Low Disk Project";

      // this will fail to copy at some point
      QVERIFY( ! _d->SetProject(_tmpPath, projName, true));

      // should be cleaned up
      QVERIFY( ! QDir(_tmpPath).exists(projName) );

      // and original is intact

   }
   else
      qWarning("Low disk scenario can't be tested");
}

void testSetProject_NonEmpty()
{
   // the internal block file list should be the same as our list
   QVERIFY( (int)_d->mBlockFileHash.size() == _blockFiles.count());

   const QString projPath = _tmpPath;
   const QString projName = "testDirManager";

   QDir dir(projPath);
   QVERIFY(!dir.exists(projName) || dir.rmdir(projName));
   QVERIFY(!dir.exists(projName));
   QVERIFY(_d->SetProject(projPath, projName, true));

   // files were copied into this dir
   QVERIFY(dir.exists(projName) && dir.cd(projName));

   for (BlockFilePtr b : _blockFiles)
   {
      // file paths should be updated
      QVERIFY(b->GetFileName().startsWith(dir.absolutePath()));

      // only block files with realized data should be present on disk
      // TESTME: OD types will not until ODManager is ported
      bool shouldExist =
            nullptr == dynamic_cast<ODPCMAliasBlockFile*>(b.get()) &&
            nullptr == dynamic_cast<ODDecodeBlockFile*>(b.get());

      QVERIFY(0 == (shouldExist ^ QFileInfo(b->GetFileName()).exists()));
      QVERIFY(_d->ContainsBlockFile(b.get()));
      QVERIFY(_d->ContainsBlockFile(QFileInfo(b->GetFileName()).baseName()));
   }
}

//
// HandleXMLTag() needs to be driven from another HandleXMLTag() since
// it requires SetLoadingTarget() before each child tag.
//
// To test that, we'll create our own handler
//
private:
bool HandleXMLTag(const QString& tag, const QStringMap& attrs) override
{
   if (tag == "blockfiles")
      return true;

   _seqBlocks.push_back(SeqBlock{});
   _d->SetLoadingTarget(&_seqBlocks, _seqBlocks.size()-1);

   return _d->HandleXMLTag(tag, attrs);
}

XMLTagHandler* HandleXMLChild(const QString & tag) override
{
   (void)tag;
   return this;
}

private Q_SLOTS:
void testHandleXMLTag()
{
   // generate some xml from blockfiles we added in previous tests
   const QString xmlFile = "dirmanager.xml";
   {
      XMLFileWriter writer(xmlFile, "TestDirManager");

      writer.StartTag("blockfiles");

      for (BlockFilePtr b : _blockFiles)
         b->SaveXML(writer);

      writer.EndTag("blockfiles");
      writer.Commit();
   }

   // Parse the file, adding to _seqBlocks array
   // Since we haven't dropped our references to the block files,
   // DirManager should see this and avoid making duplicates.
   {
      XMLFileReader reader;
      QVERIFY(reader.Parse(this, xmlFile));
   }

   // check we in fact parsed everything we wrote
   QVERIFY( _blockFiles.count() == (int)_seqBlocks.size() );

   for (int i = 0; i < _blockFiles.count(); i++)
   {
      const BlockFilePtr& oldBf = _blockFiles[i];
      const BlockFilePtr& newBf = _seqBlocks[i].f;

      // no duplicates were created
      QVERIFY( oldBf.get() == newBf.get() );
   }

   //
   // Parse again, after releasing the files referenced
   // in the xml. The loaded files should be new.
   //
   // Make copies for testing the results. Note only
   // lockable blockfiles can be copied, otherwise their
   // refcount is bumped
   //
   _seqBlocks.clear();
   for (BlockFilePtr &b : _blockFiles)
   {
      const QString oldName = b->GetFileName();
      bool wasLocked = b->IsLocked();

      // The assignment releases the old block file
      b = _d->CopyBlockFile(b);

      // Locked, should have made a copy
      if (wasLocked)
         QVERIFY(oldName != b->GetFileName());

      // Copy should not be locked, we'll lock it again to test it later
      QVERIFY(!b->IsLocked());
      b->Lock();
   }

   {
      XMLFileReader reader;
      QVERIFY(reader.Parse(this, xmlFile));
   }

   for (int i = 0; i < _blockFiles.count(); i++)
   {
      const BlockFilePtr& oldBf = _blockFiles[i];
      const BlockFilePtr& newBf = _seqBlocks[i].f;

      // We copied the (locked) blockfiles so these are unique now
      if (oldBf->IsLocked())
      {
         QVERIFY( oldBf.get() != newBf.get() );
         QVERIFY( oldBf->GetFileName() != newBf->GetFileName());
      }
      else
      {
         QVERIFY( oldBf.get() == newBf.get() );
      }

      // Check a few things to see if parse worked. Blockfiles have
      // their own unit test for the rest.
      QCOMPARE(typeid(*oldBf.get()).name(), typeid(*newBf.get()).name());
      QVERIFY(oldBf->GetLength() == newBf->GetLength());
      QVERIFY(oldBf->IsAlias() == newBf->IsAlias());
   }

   QFile::remove(xmlFile);
}

void testsProjectFSCK_AutoRecovery()
{
   _blockFiles.clear();
   _seqBlocks.clear();
   _d.reset();
   _d = std::make_unique<DirManager>();

   const QString projPath = _tmpPath;
   const QString projName = "testFsck_auto";

   QDir dir(projPath);
   QVERIFY(!dir.exists(projName) || dir.rmdir(projName));
   QVERIFY(!dir.exists(projName));
   QVERIFY(_d->SetProject(projPath, projName, true));
   QVERIFY(dir.exists(projName) && dir.cd(projName));

   TestData t;
   makeTestData("", t);
   BlockFilePtr b = _d->NewSimpleBlockFile(t.samples.ptr(), t.numSamples, t.fmt);

   // verify block file contains the test data
   checkReadData(*b.get(), t);

   // nothing should be wrong
   int status =_d->ProjectFSCK(false, true);
   QVERIFY(status == 0);

   // 1) remove the data file (.au) and see if FSCK replaces with silence
   QVERIFY(QFile(b->GetFileName()).remove());

   status =_d->ProjectFSCK(false, true);
   QVERIFY(status & FSCKstatus_CHANGED);

   SampleBuffer dst(t.numSamples, t.fmt);
   setOne(dst, t.fmt, 0, t.numSamples);
   QVERIFY(t.numSamples == b->ReadData(dst.ptr(), t.fmt, 0, t.numSamples));
   QVERIFY(isZero(dst, t.fmt, 0, t.numSamples));

   // nothing should be wrong now
   QVERIFY(0 == _d->ProjectFSCK(false, true));

   // same thing for alias files
   makeTestData("test.wav", t);
   b = _d->NewAliasBlockFile(t.fileName, 0, t.numSamples, 0);

   // check the summary data is correct
   checkGetMinMaxRMS(*b.get(), t);

   // 2) nuke summary file (.auf), it should be rebuilt
   QVERIFY(QFileInfo(b->GetFileName()).exists());
   QVERIFY(QFile(b->GetFileName()).remove());
   status = _d->ProjectFSCK(false, true);
   QVERIFY(status & FSCKstatus_CHANGED);

   // summary is still correct
   checkGetMinMaxRMS(*b.get(), t);

   // nothing wrong now
   QVERIFY(0 == _d->ProjectFSCK(false, true));

   // 3) nuke the aliased file (.wav), should be replaced with silence
   AliasBlockFile* alias = dynamic_cast<AliasBlockFile*>(b.get());
   QVERIFY(alias);
   QVERIFY(QFileInfo(alias->GetAliasedFileName()).exists());
   QVERIFY(QFile(alias->GetAliasedFileName()).remove());
   status = _d->ProjectFSCK(false, true);
   QVERIFY(status & FSCKstatus_CHANGED);

   // summary should reflect silence
   auto minMax = b->GetMinMaxRMS();
   QCOMPARE(minMax.min, 0.0f);
   QCOMPARE(minMax.max, 0.0f);
   QCOMPARE(minMax.RMS, 0.0f);

   // data as well
   setOne(dst, t.fmt, 0, t.numSamples);
   QVERIFY(t.numSamples == b->ReadData(dst.ptr(), t.fmt, 0, t.numSamples));
   QVERIFY(isZero(dst, t.fmt, 0, t.numSamples));

   // 4) TESTME: orphaned block files, requires clipboard model
}



// the click path is the same for missing blockfile or aliased file
void checkProjectFSCK_Dialogs_Common(const TestData &t, const BlockFile &b)
{
   // 1.1) force error dialog at the start, and click the cancel changes button
   DialogManager::PushButtonClick(0);
   int status =_d->ProjectFSCK(true, false);
   QVERIFY(status == FSCKstatus_CLOSE_REQ);

   // project wasn't modified so ReadData must throw
   SampleBuffer dst(t.numSamples, t.fmt);
   QVERIFY_EXCEPTION_THROWN(b.ReadData(dst.ptr(), t.fmt, 0, t.numSamples), AudacityException);

   // 1.2) don't force dialog at start, expect dialog and no changes
   DialogManager::PushButtonClick(0);
   status =_d->ProjectFSCK(false, false);
   QVERIFY(status == FSCKstatus_CLOSE_REQ);
   QVERIFY_EXCEPTION_THROWN(b.ReadData(dst.ptr(), t.fmt, 0, t.numSamples), AudacityException);

   // 1.3) silence the error logging from blockfiles, don't modify project
   // TESTME: is the error logging actually silenced? ReadData will still throw...
   DialogManager::PushButtonClick(1);
   status =_d->ProjectFSCK(false, false);
   QVERIFY(status == 0);
   QVERIFY_EXCEPTION_THROWN(b.ReadData(dst.ptr(), t.fmt, 0, t.numSamples), AudacityException);

   // 1.4) force error dialog at the start, and
   // -click the proceed with repairs button
   // -click the replace with silence button
   DialogManager::PushButtonClick(2); // second dialog
   DialogManager::PushButtonClick(1); // first dialog
   status =_d->ProjectFSCK(true, false);
   QVERIFY(status == (FSCKstatus_CHANGED|FSCKstatus_SAVE_AUP));
   setOne(dst, t.fmt, 0, t.numSamples);
   QVERIFY(t.numSamples == b.ReadData(dst.ptr(), t.fmt, 0, t.numSamples));
   QVERIFY(isZero(dst, t.fmt, 0, t.numSamples));
}

void testsProjectFSCK_Dialogs_MissingBlockFile()
{
   TestData t;
   makeTestData("", t);
   resetDirManager();

   BlockFilePtr b = _d->NewSimpleBlockFile(t.samples.ptr(), t.numSamples, t.fmt);

   // verify block file contains the test data
   checkReadData(*b.get(), t);

   // nothing should be wrong
   int status =_d->ProjectFSCK(false, true);
   QVERIFY(status == 0);

   // 1) remove the data file (.au) and see if FSCK replaces with silence
   QVERIFY(QFile(b->GetFileName()).remove());

   checkProjectFSCK_Dialogs_Common(t, *b.get());
}

void testsProjectFSCK_Dialogs_MissingAliasedFile()
{
   QDir dir;
   TestData t;
   makeTestData("alias.wav", t);
   resetDirManager();
   BlockFilePtr b = _d->NewAliasBlockFile(t.fileName, 0, t.numSamples, 0);

   // verify block file contains the test data
   checkReadData(*b.get(), t);

   // nothing should be wrong
   int status =_d->ProjectFSCK(false, true);
   QVERIFY(status == 0);

   // 1) remove the alias file (.wav) and see if FSCK replaces with silence
   QVERIFY(QFile(t.fileName).remove());

   checkProjectFSCK_Dialogs_Common(t, *b.get());
}

void testsProjectFSCK_Dialogs_MissingSummaryFile()
{
   QDir dir;
   TestData t;
   makeTestData("alias.wav", t);
   resetDirManager();
   BlockFilePtr b = _d->NewAliasBlockFile(t.fileName, 0, t.numSamples, 0);

   // verify block file contains the test data
   checkReadData(*b.get(), t);

   // nothing should be wrong
   int status =_d->ProjectFSCK(false, true);
   QVERIFY(status == 0);

   // 1) remove the summary file (.auf)
   QVERIFY(QFile(b->GetFileName()).remove());

   // common test doesn't work, button click path is different, error effects are different
   //checkProjectFSCK_Dialogs_Common(t, *b.get());

   // 1.1) force error dialog at the start, and click the cancel changes button
   DialogManager::PushButtonClick(0);
   status =_d->ProjectFSCK(true, false);
   QVERIFY(status == FSCKstatus_CLOSE_REQ);
   // summary file wasn't regenerated
   QVERIFY(!QFile::exists(b->GetFileName()));

   // 1.2) don't force dialog at start, click no changes button
   DialogManager::PushButtonClick(2);
   status =_d->ProjectFSCK(false, false);
   QVERIFY(status == FSCKstatus_CLOSE_REQ);
   QVERIFY(!QFile::exists(b->GetFileName()));

   // 1.3) silence the error logging from blockfiles, don't modify project
   // TESTME: is the error logging actually silenced? ReadData will still throw...
   DialogManager::PushButtonClick(1);
   status =_d->ProjectFSCK(false, false);
   QVERIFY(status == 0);
   QVERIFY(!QFile::exists(b->GetFileName()));

   // 1.4) force error dialog at the start, and
   // -click the proceed with repairs button
   // -click the regenerate summary files button
   DialogManager::PushButtonClick(0); // second dialog
   DialogManager::PushButtonClick(1); // first dialog
   status =_d->ProjectFSCK(true, false);
   QVERIFY(status == (FSCKstatus_CHANGED|FSCKstatus_SAVE_AUP));
   QVERIFY(QFile::exists(b->GetFileName()));
}

void testRemoveOrphanBlockFiles()
{
   // TESTME: requires clipboard model
   _d->RemoveOrphanBlockfiles();
}

void testEnsureSafeFileName()
{
   TestData t;
   makeTestData("test.wav", t);
   BlockFilePtr b = _d->NewAliasBlockFile(t.fileName, 0, t.numSamples, 0);

   AliasBlockFile* a = dynamic_cast<AliasBlockFile*>(b.get());
   QVERIFY(a);

   QString oldFileName = a->GetAliasedFileName();
   QVERIFY(QFile(oldFileName).exists());

   // moves existing test.wav to test-old%d.wav
   QVERIFY(_d->EnsureSafeFilename(t.fileName));

   // old file was moved
   QVERIFY(!QFile(oldFileName).exists());

   // but we didn't lose it
   QVERIFY(QFile(a->GetAliasedFileName()).exists());

   // clean up
   QFile::remove(a->GetAliasedFileName());
}

void testDestruct()
{
   // test temp is only cleaned if there are no more live DirManagers

   // release any mess from other tests
   _blockFiles.clear();
   _seqBlocks.clear();
   _d.reset();
   QDir tmpDir(_tmpPath);
   QString dirName = tmpDir.dirName();
   QVERIFY( tmpDir.removeRecursively() );
   QVERIFY( qDirIsEmpty(tmpDir) );
   QVERIFY( tmpDir.cdUp() && tmpDir.mkdir(dirName) && tmpDir.cd(dirName) );

   TestData t;
   makeTestData(1000, t);

   std::unique_ptr<DirManager>d1 = std::make_unique<DirManager>();
   d1->NewSimpleBlockFile(t.samples.ptr(), t.numSamples, t.fmt)->Lock();

   std::unique_ptr<DirManager>d2 = std::make_unique<DirManager>();
   d2->NewSimpleBlockFile(t.samples.ptr(), t.numSamples, t.fmt)->Lock();

   // tmp dir should not be touched here
   qDebug() << tmpDir.entryList();
   d1.reset();
   qDebug() << tmpDir.entryList();

   QVERIFY( !qDirIsEmpty(tmpDir) );

   qDebug() << tmpDir.entryList();
   d2.reset();
   QVERIFY( qDirIsEmpty(tmpDir) );
}

};

QTEST_MAIN(TestDirManager)
#include "TestDirManager.moc"
