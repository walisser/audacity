#include "TestBlockFile.h"

#include "core/blockfile/PCMAliasBlockFile.h"
#include "core/blockfile/ODPCMAliasBlockFile.h"
//#include "core/DirManager.h"
//#include "core/Internat.h"
#include "core/AudacityException.h"
//#include "core/xml/XMLWriter.h"
//#include "core/xml/XMLFileReader.h"

#include <QtTest>
#include "TestHelpers.h"

class TestPCMAliasBlockFile : public TestBlockFile
{
   Q_OBJECT

virtual void buildFromXml(DirManager& dm, const QString& tag, const QStringMap& attrs) override
{
   if (tag == "pcmaliasblockfile")
      _loadedBlockFile = PCMAliasBlockFile::BuildFromXML(dm, attrs);
   else
   if (tag == "odpcmaliasblockfile")
      _loadedBlockFile = ODPCMAliasBlockFile::BuildFromXML(dm, attrs);
   else
      qFatal("Unhandled block file type");
}

void initTestcase()
{
   _implementation = this;
}

void testPCMAliasBlockFile()
{
   BlockFile::gBlockFileDestructionCount = 0;

   // TODO: this test code is mostly the same (and was copied from) SimpleBlockFile...
   TestData t;
   makeTestData("pcm.wav", t);

   QString aliasFileName = "alias";
   sampleCount startSample = 0;
   int channel = 0;

   // test extension is added and get the actual file name
   {
      PCMAliasBlockFile bf(aliasFileName, t.fileName, startSample, t.numSamples, channel);
      aliasFileName = bf.GetFileName();
   }

   // following tests assume the file doesn't exist
   QFile(aliasFileName).remove();

   // do not write summary
   {
      PCMAliasBlockFile bf(aliasFileName, t.fileName, startSample, t.numSamples, channel, false);
      QVERIFY( !QFileInfo(bf.GetFileName()).exists() );
   }

   // do write summary
   {
      PCMAliasBlockFile bf(aliasFileName, t.fileName, startSample, t.numSamples, channel, true);
      QVERIFY( QFileInfo(bf.GetFileName()).exists() );
   }

   {
      PCMAliasBlockFile pcmBf(aliasFileName, t.fileName, startSample, t.numSamples, channel);

      // test from the base class in case a virtual override was missed
      BlockFile& bf = pcmBf;

      QVERIFY( bf.GetFileName().endsWith(".auf") );
      aliasFileName = bf.GetFileName();

      QVERIFY( QFileInfo(aliasFileName).exists() );

      QCOMPARE( pcmBf.GetAliasedFileName(), t.fileName);
      QCOMPARE( bf.GetLength(), t.numSamples);

      QVERIFY( bf.IsAlias() );
      QVERIFY( bf.IsSummaryAvailable() );
      QVERIFY( bf.IsDataAvailable() );
      QVERIFY( ! bf.IsSummaryBeingComputed() );

      QVERIFY( bf.GetSpaceUsage() != 0 );
      QVERIFY( bf.GetSpaceUsage() == (BlockFile::DiskByteCount)QFileInfo(aliasFileName).size() );

      checkLockUnlock(bf);

      checkGetMinMaxRMS(bf, t);
      checkGetMinMaxRMSOverflows(bf, t, EXPECT_THROW);

      checkReadData(bf, t);
      checkReadDataOverflows(bf, t, EXPECT_THROW);

      checkReadSummary(256, bf, t);
      checkReadSummary(64*1024, bf, t);

      checkCopy(bf);
      checkRecover(bf);

      // Locking so it should exist after this scope exits
      bf.Lock();
   }

   QCOMPARE( (int)BlockFile::gBlockFileDestructionCount, 5 );
   QVERIFY( QFileInfo(aliasFileName).exists() );

   // pcm file doesn't exit, must throw
   QVERIFY_EXCEPTION_THROWN(
            make_blockfile<PCMAliasBlockFile>(aliasFileName, "boguspcm.wav",
                                              startSample, t.numSamples, channel), AudacityException );

   // directory doesn't exist - this is actually OK and should not throw
   // the summary data will be stored in memory as a fallback
   (void)make_blockfile<PCMAliasBlockFile>("bogusdir/aliasfile", t.fileName,
                                           startSample, t.numSamples, channel);

   // simulate low disk space
   // again this should not throw since summary data will go into memory
   {
      FileSizeLimiter limit;
      if (limit.apply(1024))
         (void)make_blockfile<PCMAliasBlockFile>(aliasFileName, t.fileName,
                                           startSample, t.numSamples, channel);
      else
         QWARN("Low disk simulation unsupported or disabled");
   }

   // xml save/load
   QString xmlName = aliasFileName+".xml";
   {
      auto bf = make_blockfile<PCMAliasBlockFile>(aliasFileName, t.fileName,
                                                  startSample, t.numSamples, channel);
      checkSaveXML(*(bf.get()), xmlName, "PCMAliasBlockFile");
   }

   {
      // only checks BlockFile name matches
      checkLoadFromXML(xmlName, aliasFileName, t);

      auto& pcmbf = *dynamic_cast<PCMAliasBlockFile*>(_loadedBlockFile.get());

      // could do more checks, but the xml doesn't store much
      checkGetMinMaxRMS(pcmbf, t);

      QCOMPARE( pcmbf.GetAliasedFileName(), t.fileName );

      // release the loaded file
      _loadedBlockFile.reset();
   }

   // blockfile should be gone since we didn't lock it above
   QVERIFY( ! QFileInfo(aliasFileName).exists() );

   QFile(xmlName).remove();
   QFile(t.fileName).remove();
}

void testODPCMAliasBlockFile()
{
   BlockFile::gBlockFileDestructionCount = 0;

   // TODO: this test code is mostly the same (and was copied from) SimpleBlockFile...
   TestData t;
   makeTestData("odpcm.wav", t);

   QString aliasFileName="odalias";
   sampleCount startSample = 0;
   int channel = 0;

   {
      ODPCMAliasBlockFile od(aliasFileName, t.fileName, startSample, t.numSamples, channel);
      aliasFileName = od.GetFileName();
   }

   QFile(aliasFileName).remove();

   {
      ODPCMAliasBlockFile od(aliasFileName, t.fileName, startSample, t.numSamples, channel);
      BlockFile& bf = od;

      QVERIFY( bf.GetFileName().endsWith(".auf") );
      aliasFileName = bf.GetFileName();

      // doesn't exist, we haven't calculated it yet
      QVERIFY( ! QFileInfo(aliasFileName).exists() );

      QCOMPARE( od.GetAliasedFileName(), t.fileName);
      QCOMPARE( bf.GetLength(), t.numSamples);

      QVERIFY( bf.IsAlias() );
      QVERIFY( ! bf.IsSummaryAvailable() );
      QVERIFY( bf.IsDataAvailable() );
      QVERIFY( ! bf.IsSummaryBeingComputed() );
      QVERIFY( bf.GetSpaceUsage() == 0 );

      //checkLockUnlock(bf);

      // GetMinMaxRMS() should throw and fill in defaults
      checkGetMinMaxRMS(bf, t, EXPECT_DEFAULTS, EXPECT_THROW);

      // GetMinMaxRMS(start,len) should throw on overflow
      checkGetMinMaxRMSOverflows(bf, t, EXPECT_THROW);

      // ReadData() should give us valid data, only the summary is unavailable
      checkReadData(bf, t);
      checkReadDataOverflows(bf, t, EXPECT_THROW);


      // Read256/Read64K() should return false and zero the samples out
      checkReadSummary(256, bf, t, EXPECT_DEFAULTS);
      checkReadSummary(64*1024, bf, t, EXPECT_DEFAULTS);

      checkCopy(bf);

      // Cannot recover since no data read yet
      //checkRecover(bf);

      // Locking so it should exist after this scope exits
      bf.Lock();
   }

   QCOMPARE( (int)BlockFile::gBlockFileDestructionCount, 3 );
   QVERIFY( ! QFileInfo(aliasFileName).exists() ); // no indexing performed yet!

   // pcm file doesn't exist,
   // no-throw because there won't be any attempt to access the file until DoWriteSummary
   (void)make_blockfile<ODPCMAliasBlockFile>(aliasFileName, "boguspcm.wav", startSample,
                                             t.numSamples, channel);

   QString xmlName = aliasFileName+".xml";
   {
      auto bf = make_blockfile<ODPCMAliasBlockFile>(aliasFileName, t.fileName,
                                                  startSample, t.numSamples, channel);
      checkSaveXML(*(bf.get()), xmlName, "ODPCMAliasBlockFile");
   }

   {
      // only checks BlockFile name
      checkLoadFromXML(xmlName, aliasFileName, t);

      // do some more checks on it?
      auto& pcmbf = *dynamic_cast<ODPCMAliasBlockFile*>(_loadedBlockFile.get());

      checkGetMinMaxRMS(pcmbf, t, EXPECT_DEFAULTS, EXPECT_THROW);

      QCOMPARE( pcmbf.GetAliasedFileName(), t.fileName );

      // release the loaded file
      _loadedBlockFile.reset();
   }
}

};

QTEST_MAIN(TestPCMAliasBlockFile)
#include "TestPCMAliasBlockFile.moc"
