#include <QtTest>

#include "core/blockfile/SilentBlockFile.h"
#include "core/DirManager.h"
#include "core/Internat.h"
#include "core/AudacityException.h"
#include "core/xml/XMLWriter.h"
#include "core/xml/XMLFileReader.h"

#include "TestHelpers.h"


// stubs

static BlockFilePtr _dirManagerBlockFile;

DirManager::DirManager() {}
DirManager::~DirManager() {}
QString DirManager::GetProjectDataDir() { return "."; }

bool DirManager::HandleXMLTag(const QString &tag, const QStringMap &attrs)
{
   Q_REQUIRE(tag == "silentblockfile");
   _dirManagerBlockFile = SilentBlockFile::BuildFromXML(*this, attrs);
   return true;
}

// end stubs

class TestSilentBlockFile : public QObject
{
   Q_OBJECT
private Q_SLOTS:

//void initTestCase() {}

//void cleanupTestCase() {}
void testSilentBlockFile()
{
   testSilentBlockFile(44100);
}

private:

void testSilentBlockFile(const size_t numSamples)
{
   BlockFile::gBlockFileDestructionCount = 0;

   QString fileName;
   {
      SilentBlockFile sbf(numSamples);
      BlockFile& bf = sbf;

      //QVERIFY( bf.GetFileName().endsWith(".au") );
      //fileName = bf.GetFileName();
      QVERIFY( !QFileInfo(bf.GetFileName()).exists() );

      QVERIFY( bf.GetLength() == numSamples );
      QVERIFY( ! bf.IsAlias() );
      QVERIFY( bf.IsSummaryAvailable() );
      QVERIFY( bf.IsDataAvailable() );
      QVERIFY( ! bf.IsSummaryBeingComputed() );
      QVERIFY( bf.GetSpaceUsage() == 0);

      QVERIFY( ! bf.IsLocked() );
      bf.Lock();
      bf.Unlock();
      QVERIFY( ! bf.IsLocked() );

      // GetMinMaxRMS()
      auto m = bf.GetMinMaxRMS();
      QCOMPARE( m.min, 0.0f );
      QCOMPARE( m.max, 0.0f );
      QCOMPARE( m.RMS, 0.0f );

      // try to read too much
      (void)bf.GetMinMaxRMS(0, numSamples+1, false); // must not throw

      // doesn't have to throw
      //QVERIFY_EXCEPTION_THROWN( bf.GetMinMaxRMS(0, numSamples+1), AudacityException );

      // ReadData()
      sampleFormat fmt = int16Sample;
      SampleBuffer dst(numSamples, fmt);
      memset(dst.ptr(), 0xffff, numSamples*SAMPLE_SIZE(fmt));
      QVERIFY( numSamples == bf.ReadData(dst.ptr(), fmt, 0, numSamples) );

      uint16_t* ptr = (uint16_t*)dst.ptr();
      for (size_t i = 0; i < numSamples; i++)
         QCOMPARE(ptr[i], (uint16_t)0);

      // Read256()
      {
         Q_STATIC_ASSERT( sizeof(BlockFile::MinMaxRMS) == sizeof(float)*3 );

         // if it doesn't divide evenly, get one more
         size_t summaryLen = numSamples/256;
         if (numSamples % 256 != 0)
            summaryLen++;

         ArrayOf<BlockFile::MinMaxRMS> summaryData( summaryLen );

         QVERIFY( bf.Read256((float*)summaryData.get(), 0, summaryLen) );
         QCOMPARE( summaryData.get()[0].min, 0.0f );
         QCOMPARE( summaryData.get()[0].max, 0.0f );
         QCOMPARE( summaryData.get()[0].RMS, 0.0f );
         QCOMPARE( summaryData.get()[summaryLen-1].min, 0.0f );
         QCOMPARE( summaryData.get()[summaryLen-1].max, 0.0f );
         QCOMPARE( summaryData.get()[summaryLen-1].RMS, 0.0f );
      }

      // Read64K()
      {
         size_t summaryLen = numSamples/(64*1024);
         if (numSamples % (64*1024) != 0)
            summaryLen++;

         ArrayOf<BlockFile::MinMaxRMS> summaryData( summaryLen );

         QVERIFY( bf.Read64K((float*)summaryData.get(), 0, summaryLen) );
         QCOMPARE( summaryData.get()[0].min, 0.0f );
         QCOMPARE( summaryData.get()[0].max, 0.0f );
         QCOMPARE( summaryData.get()[0].RMS, 0.0f );
         QCOMPARE( summaryData.get()[summaryLen-1].min, 0.0f );
         QCOMPARE( summaryData.get()[summaryLen-1].max, 0.0f );
         QCOMPARE( summaryData.get()[summaryLen-1].RMS, 0.0f );
      }

      // Copy()
      {
         QString copyName = "copy.au";
         BlockFilePtr copy = bf.Copy(copyName);
         BlockFile& cp = *(copy.get());
         QCOMPARE( cp.GetFileName(), QString("") ); // has no file name
         QCOMPARE( cp.GetLength(), bf.GetLength() );
         auto rms1 = cp.GetMinMaxRMS();
         auto rms2 = bf.GetMinMaxRMS();
         QCOMPARE( rms1.min, rms2.min );
         QCOMPARE( rms1.max, rms2.max );
         QCOMPARE( rms1.RMS, rms2.RMS );

         QVERIFY( ! QFileInfo(copyName).exists() );
      }

      // Recover() does nothing, there's no data file
      bf.Recover();
      QVERIFY( bf.GetLength() == numSamples );
   }

   QCOMPARE( (int)BlockFile::gBlockFileDestructionCount, 2 );
   QVERIFY( ! QFileInfo(fileName).exists() );


   // xml save/load
   QString xmlName = "silence.xml";
   {
      auto bf = make_blockfile<SilentBlockFile>(numSamples);
      XMLFileWriter writer(xmlName, "SilentBlockFile");
      bf.get()->SaveXML(writer);
      writer.Commit(); // save xml to disk
      bf.get()->Lock(); // prevent file deletion
   }

   {
      DirManager handler;
      XMLFileReader reader;
      QVERIFY( reader.Parse(&handler, xmlName) );

      auto& bf = *(_dirManagerBlockFile.get());

      QCOMPARE( bf.GetLength(), numSamples );
      QVERIFY( bf.GetSpaceUsage() == 0 );

      auto m = bf.GetMinMaxRMS();
      QCOMPARE( bf.GetFileName(), fileName );
      QCOMPARE( m.min, 0.0f );
      QCOMPARE( m.max, 0.0f );
      QCOMPARE( m.RMS, 0.0f);

      _dirManagerBlockFile.reset();
   }

   QFile(xmlName).remove();
}

};

QTEST_MAIN(TestSilentBlockFile)
#include "TestSilentBlockFile.moc"
