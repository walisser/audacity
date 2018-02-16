#include "TestBlockFile.h"

#include "core/blockfile/SimpleBlockFile.h"
#include "core/AudacityException.h"

#include <QtTest>
#include "TestHelpers.h"


class TestSimpleBlockFile : public TestBlockFile
{
   Q_OBJECT

virtual void buildFromXml(DirManager& dm, const QString& tag, const QStringMap& attrs) override
{
   Q_REQUIRE(tag == "simpleblockfile");
   _loadedBlockFile = SimpleBlockFile::BuildFromXML(dm, attrs);
}

private Q_SLOTS:

void initTestcase()
{
   _implementation = this;
}

void testSimpleBlockFile_int16()
{
   QString fileName = "audio_int16";
   TestData t;
   makeTestData("", t);
   testSimpleBlockFile(fileName, t);
}

void testSimpleBlockFile_int24()
{
   QString fileName = "audio_int24";
   TestData t;
   makeTestData24Bit("", t);
   testSimpleBlockFile("audio_int24", t);
}

void testSimpleBlockFile_float()
{
   QString fileName = "audio_float";
   TestData t;
   makeTestDataFloat("", t);
   testSimpleBlockFile("audio_float", t);
}

private:

void testSimpleBlockFile(QString fileName, TestData &t)
{
   BlockFile::gBlockFileDestructionCount = 0;

   {
      SimpleBlockFile sbf(fileName, t.samples.ptr(), t.numSamples, t.fmt);
      BlockFile& bf = sbf;

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

      /* TESTME maybe more complete test of Recover()
      // SetFileName() + Recover()
      {
         // changing the name doesn't write a new file or rename the existing one
         QString newName = "other.au";
         QFile(newName).remove();

         bf.SetFileName(newName);
         QCOMPARE( bf.GetFileName(), newName );
         QVERIFY( ! QFileInfo(newName).exists() );

         // writes zeros to the new name since it didn't exist
         bf.Recover();
         QVERIFY( QFileInfo(newName).exists() );
         QVERIFY( bf.GetSpaceUsage() >= numSamples*SAMPLE_SIZE_DISK(fmt));
         SampleBuffer dst(numSamples, fmt);
         QVERIFY( numSamples == bf.ReadData(dst.ptr(), fmt, 0, numSamples) );
         for (size_t i = 0; i < numSamples; i++)
            QCOMPARE( ((T*)dst.ptr())[i], (T)0 );

         // change the name back; it gets deleted on destruct
         bf.SetFileName(fileName);

         // clean up
         QFile(newName).remove();
      }
      */
   }

   QCOMPARE( (int)BlockFile::gBlockFileDestructionCount, 2 );
   QVERIFY( ! QFileInfo(fileName).exists() );

   // directory doesn't exist
   QVERIFY_EXCEPTION_THROWN(
      make_blockfile<SimpleBlockFile>("bogusdir/audio", t.samples.ptr(), t.numSamples, t.fmt), AudacityException );

   // simulate low disk space
   {
      FileSizeLimiter limit;
      if (limit.apply( t.numSamples/2 * SAMPLE_SIZE_DISK(t.fmt)))
         QVERIFY_EXCEPTION_THROWN(
            make_blockfile<SimpleBlockFile>("lowdisk", t.samples.ptr(), t.numSamples, t.fmt), AudacityException );
      else
         QWARN("Low disk simulation unsupported or disabled");
   }

   // xml save/load
   QString xmlName = fileName+".xml";
   {
      auto bf = make_blockfile<SimpleBlockFile>(fileName, t.samples.ptr(), t.numSamples, t.fmt);
      checkSaveXML(*bf, xmlName, "SimpleBlockFile");

      // prevent file deletion for the next test
      bf->Lock();
   }

   {
      checkLoadFromXML(xmlName, fileName, t);

      auto& bf = *(_loadedBlockFile.get());

      QVERIFY( bf.GetSpaceUsage() >= t.numSamples*SAMPLE_SIZE_DISK(t.fmt));
      checkGetMinMaxRMS(bf, t);

      _loadedBlockFile.reset();
   }

   // blockfile should be gone since we didn't lock it above
   QVERIFY( ! QFileInfo(fileName).exists() );

   QFile(xmlName).remove();
}

};

QTEST_MAIN(TestSimpleBlockFile)
#include "TestSimpleBlockFile.moc"
