#include "TestBlockFile.h"

#include "core/blockfile/ODDecodeBlockFile.h"
#include "core/ondemand/ODDecodeFlacTask.h"

#include <QtTest>
#include "TestHelpers.h"

// stubs
#include "core/WaveTrack.h"
#include "core/WaveClip.h"
double WaveTrack::GetRate() const { return 44100; }
double WaveClip::GetStartTime() const { return 0; }
BlockArray* WaveClip::GetSequenceBlockArray() { return nullptr; }
// stubs

class TestODDecodeBlockFile : public TestBlockFile
{
   Q_OBJECT

virtual void buildFromXml(DirManager& dm, const QString& tag, const QStringMap& attrs) override
{
   Q_REQUIRE(tag == "oddecodeblockfile");
   _loadedBlockFile = ODDecodeBlockFile::BuildFromXML(dm, attrs);
}

private Q_SLOTS:

void initTestcase()
{
   _implementation = this;
}

void testODDecodeBlockFile()
{
   BlockFile::gBlockFileDestructionCount = 0;

   TestData t;
   makeTestData("odbf.flac", t);

   QString baseFileName = "odbf";
   QFile(baseFileName+".au").remove();
   QFile(baseFileName+".xml").remove();

   {
      ODDecodeBlockFile obf(baseFileName, t.fileName, 0, t.numSamples,
                            0, 0);
      BlockFile& bf = obf;
      baseFileName = bf.GetFileName();

      QVERIFY( !QFileInfo(bf.GetFileName()).exists() );
      QVERIFY( bf.GetLength() == t.numSamples );
      QVERIFY( ! bf.IsAlias() );
      QVERIFY( ! bf.IsSummaryAvailable() );
      QVERIFY( ! bf.IsDataAvailable() );
      QVERIFY( ! bf.IsSummaryBeingComputed() );
      QVERIFY( bf.GetSpaceUsage() == 0);
      QVERIFY( bf.GetLength() == t.numSamples );

      checkLockUnlock(bf);
      checkCloseLock(bf);

      checkGetMinMaxRMS(bf, t, EXPECT_DEFAULTS, EXPECT_THROW);
      checkGetMinMaxRMSOverflows(bf, t, EXPECT_THROW);
      checkReadData(bf, t, EXPECT_THROW);

      // checkReadData() covers this
      //checkReadDataOverflows(bf, t);

      checkReadSummary(256, bf, t, EXPECT_DEFAULTS);
      checkReadSummary(64*1024, bf, t, EXPECT_DEFAULTS);

      checkCopy(bf, false);
      checkSetFileName(bf);

      // Recover() does nothing, there's no data file
      bf.Recover();
   }

   QCOMPARE( (int)BlockFile::gBlockFileDestructionCount, 2 );
   QVERIFY( ! QFileInfo(baseFileName).exists() );


   // xml save/load
   QString xmlName = "oddecodeblockfile.xml";
   {
      auto bf = make_blockfile<ODDecodeBlockFile>(baseFileName, t.fileName,
                                                  0, t.numSamples, 0, 0);
      checkSaveXML(*bf, xmlName, "ODDecodeBlockFile");
   }

   {
      checkLoadFromXML(xmlName, baseFileName, t);

      auto& bf = *(_loadedBlockFile.get());

      QVERIFY( bf.GetSpaceUsage() == 0 );
      checkGetMinMaxRMS(bf, t, EXPECT_DEFAULTS, EXPECT_THROW);

      _loadedBlockFile.reset();
   }

   QFile(xmlName).remove();
}

void testODDecodeBlockFile_FLAC()
{
   BlockFile::gBlockFileDestructionCount = 0;

   TestData t;

   makeTestData("odbf.flac", t);

   QString baseFileName = "odbf";
   QFile(baseFileName+".au").remove();
   QFile(baseFileName+".xml").remove();
   QString fileName;

   {
      ODDecodeBlockFile obf(baseFileName, t.fileName, 0, t.numSamples,
                            0, 0);
      BlockFile& bf = obf;

      ODDecodeFlacTask task;
      ODFileDecoder *decoder = task.CreateFileDecoder(t.fileName);
      QVERIFY(decoder);

      QVERIFY(decoder->ReadHeader());
      obf.SetODFileDecoder(decoder);

      int ret = obf.DoWriteBlockFile();
      QVERIFY(ret == 1);

      // the file is decoded and summarized
      // should now behave like a SimpleBlockFile...
      QVERIFY( bf.GetFileName().endsWith(".au") );
      fileName = bf.GetFileName();
      QVERIFY( QFileInfo(fileName).exists() );

      QVERIFY( bf.GetLength() == t.numSamples );
      QVERIFY( ! bf.IsAlias() );
      QVERIFY( bf.IsSummaryAvailable() );
      QVERIFY( bf.IsDataAvailable() );
      QVERIFY( ! bf.IsSummaryBeingComputed() );
      QVERIFY( bf.GetSpaceUsage() >= t.numSamples*SAMPLE_SIZE_DISK(t.fmt));

      checkLockUnlock(bf);
      checkCloseLock(bf);

      checkGetMinMaxRMS(bf, t);
      checkGetMinMaxRMSOverflows(bf, t, EXPECT_THROW);

      checkReadData(bf, t);
      checkReadDataOverflows(bf, t, EXPECT_THROW);

      checkReadSummary(256, bf, t);
      checkReadSummary(64*1024, bf, t);

      checkCopy(bf);
      checkSetFileName(bf);
      checkRecover(bf);
   }

   QVERIFY(   BlockFile::gBlockFileDestructionCount == 2 );
}

};

QTEST_MAIN(TestODDecodeBlockFile)
#include "TestODDecodeBlockFile.moc"
