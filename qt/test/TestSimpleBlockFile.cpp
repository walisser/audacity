#include <QtTest>

#include "core/blockfile/SimpleBlockFile.h"
#include "core/DirManager.h"
#include "core/Internat.h"
#include "core/AudacityException.h"
#include "core/xml/XMLWriter.h"
#include "core/xml/XMLFileReader.h"

#include "TestHelpers.h"


// FIXME: stubs for stuff that isn't ported yet
static BlockFilePtr _dirManagerBlockFile;

DirManager::DirManager() {}
DirManager::~DirManager() {}
QString DirManager::GetProjectDataDir() { return "."; }

bool DirManager::HandleXMLTag(const QString &tag, const QStringMap &attrs)
{
   Q_REQUIRE(tag == "simpleblockfile");
   _dirManagerBlockFile = SimpleBlockFile::BuildFromXML(*this, attrs);
   return true;
}

bool DirManager::AssignFile(QString &fileName, const QString &value, bool check)
{
   fileName = value;
   Q_UNUSED(check);
   return true;
}

bool Internat::CompatibleToDouble(const QString& stringToConvert, double* result)
{
   Q_ASSERT(result != nullptr);

   bool ok=false;
   *result = stringToConvert.toDouble(&ok);
   return ok;
}
// end stubs

class TestSimpleBlockFile : public QObject
{
   Q_OBJECT
private Q_SLOTS:

//void initTestCase() {}

//void cleanupTestCase() {}
void testSimpleBlockFile_int16()
{
   // setup a test buffer so its min/max is -/+ INT16_MAX
   const size_t numSamples = INT16_MAX*2 + 1;
   const sampleFormat fmt = int16Sample;

   SampleBuffer samples(numSamples, fmt);
   int16_t *samplePtr = (int16_t*)samples.ptr();
   for (size_t i = 0; i < numSamples; i++)
      samplePtr[i] = int16_t( -INT16_MAX + i );

   QCOMPARE((int)samplePtr[0], -INT16_MAX);
   QCOMPARE((int)samplePtr[numSamples-1], INT16_MAX);

   testSimpleBlockFile<int16_t>("audio_int16", samples, numSamples, fmt,
                       -INT16_MAX/float(1<<15), INT16_MAX/float(1<<15), 0.577341f);
}

void testSimpleBlockFile_int24()
{
   // setup a test buffer so its min/max is -/+ INT24_MAX
   const int INT24_MAX = (1<<23)-1;

   const size_t numSamples = (INT24_MAX>>7)+1; // 64k samples
   const sampleFormat fmt = int24Sample;

   SampleBuffer samples(numSamples, fmt);
   int32_t *samplePtr = (int32_t*)samples.ptr();
   for (size_t i = 0; i < numSamples; i++)
      samplePtr[i] = int32_t( -INT24_MAX + i*256 );

   // sequence doesn't work out...
   samplePtr[numSamples-1] = INT24_MAX;

   QCOMPARE((int)samplePtr[0], -INT24_MAX);
   QCOMPARE((int)samplePtr[numSamples-1], INT24_MAX);

   testSimpleBlockFile<int32_t>("audio_int24", samples, numSamples, fmt,
                       -INT24_MAX/float(1<<23), INT24_MAX/float(1<<23), 0.57735f);
}

void testSimpleBlockFile_float()
{
   // setup a test buffer so its min/max is -/+ 1.0f
   const size_t numSamples = 40000 + 1;
   const sampleFormat fmt = floatSample;

   SampleBuffer samples(numSamples, fmt);
   float *samplePtr = (float*)samples.ptr();
   for (size_t i = 0; i < numSamples; i++)
      samplePtr[i] = -1.0f + i*2.0f/40000;

   QCOMPARE(samplePtr[0], -1.0f);
   QCOMPARE(samplePtr[numSamples-1], 1.0f);

   testSimpleBlockFile<float>("audio_float", samples, numSamples, fmt,
                       -1.0f, 1.0f, 0.577365f);
}

private:

template< typename T >
void testSimpleBlockFile(QString fileName, const SampleBuffer& samples,
                         const size_t numSamples, const sampleFormat fmt,
                         const float minSample, const float maxSample, const float rms
                        )
{
   BlockFile::gBlockFileDestructionCount = 0;

   {
      SimpleBlockFile sbf(fileName, samples.ptr(), numSamples, fmt);
      BlockFile& bf = sbf;

      QVERIFY( bf.GetFileName().endsWith(".au") );
      fileName = bf.GetFileName();
      QVERIFY( QFileInfo(fileName).exists() );

      QVERIFY( bf.GetLength() == numSamples );
      QVERIFY( ! bf.IsAlias() );
      QVERIFY( bf.IsSummaryAvailable() );
      QVERIFY( bf.IsDataAvailable() );
      QVERIFY( ! bf.IsSummaryBeingComputed() );
      QVERIFY( bf.GetSpaceUsage() >= numSamples*SAMPLE_SIZE_DISK(fmt));

      QVERIFY( ! bf.IsLocked() );
      bf.Lock();
      bf.Unlock();
      QVERIFY( ! bf.IsLocked() );

      // GetMinMaxRMS()
      auto m = bf.GetMinMaxRMS();
      QCOMPARE( m.min, minSample );
      QCOMPARE( m.max, maxSample );
      QCOMPARE( m.RMS, rms);

      // try to read too much
      (void)bf.GetMinMaxRMS(0, numSamples+1, false); // must not throw
      QVERIFY_EXCEPTION_THROWN( bf.GetMinMaxRMS(0, numSamples+1), AudacityException );


      // ReadData()
      SampleBuffer dst(numSamples, fmt);
      ClearSamples(dst.ptr(), fmt, 0, numSamples);
      QVERIFY( numSamples == bf.ReadData(dst.ptr(), fmt, 0, numSamples) );
      //QVERIFY( memcmp(dst.ptr(), samples.ptr(), SAMPLE_SIZE(fmt)*numSamples) == 0 );
      T* dp = (T*)dst.ptr();
      T* sp = (T*)samples.ptr();
      for (size_t i = 0; i < numSamples; i++)
         if ( dp[i] != sp[i] )
            qFatal("samples differ at index %d/%d: src=%d dst=%d",
                   (int)i, (int)numSamples, (int)sp[i], (int)dp[i]);

      // overflow: no throw allowed, zero-fill the remainder
      size_t overLen = numSamples+10;
      SampleBuffer over(overLen, fmt);
      memset(over.ptr(), 0xff, overLen*SAMPLE_SIZE(fmt));
      QCOMPARE( numSamples, bf.ReadData(over.ptr(), fmt, 0, overLen, false) );
      QVERIFY( memcmp(over.ptr(), samples.ptr(), SAMPLE_SIZE(fmt)*numSamples) == 0 );
      for (size_t i = numSamples; i < overLen; i++)
         QVERIFY( ((T*)over.ptr())[i] == 0 );

      // overflow: expect exception but overflow is not zero filled
      memset(over.ptr(), 0xff, overLen*SAMPLE_SIZE(fmt));
      QVERIFY_EXCEPTION_THROWN( bf.ReadData(over.ptr(), fmt, 0, overLen), AudacityException );
      QVERIFY( memcmp(over.ptr(), samples.ptr(), SAMPLE_SIZE(fmt)*numSamples) == 0 );

      uint8_t* overPtr = ((uint8_t*)over.ptr()) + numSamples*SAMPLE_SIZE(fmt);
      for (size_t i = 0; i < (overLen-numSamples)*SAMPLE_SIZE(fmt); i++)
         QVERIFY( overPtr[i] == 0xff );


      // Read256()
      {
         Q_STATIC_ASSERT( sizeof(BlockFile::MinMaxRMS) == sizeof(float)*3 );

         // if it doesn't divide evenly, get one more
         size_t summaryLen = numSamples/256;
         if (numSamples % 256 != 0)
            summaryLen++;

         ArrayOf<BlockFile::MinMaxRMS> summaryData( summaryLen );

         QVERIFY( bf.Read256((float*)summaryData.get(), 0, summaryLen) );

         // the first/last summary block should have the same values as the
         // whole block
         QCOMPARE( summaryData.get()[0].min, minSample );
         QCOMPARE( summaryData.get()[summaryLen-1].max, maxSample );
      }

      // Read64K()
      {
         size_t summaryLen = numSamples/(64*1024);
         if (numSamples % (64*1024) != 0)
            summaryLen++;

         ArrayOf<BlockFile::MinMaxRMS> summaryData( summaryLen );

         QVERIFY( bf.Read64K((float*)summaryData.get(), 0, summaryLen) );

         // the first/last summary block should have the same values as the
         // whole blockfile
         QCOMPARE( summaryData.get()[0].min, minSample );
         QCOMPARE( summaryData.get()[summaryLen-1].max, maxSample );
      }

      // Copy()
      {
         QString copyName = "copy.au";
         BlockFilePtr copy = bf.Copy(copyName);
         BlockFile& cp = *(copy.get());
         QCOMPARE( cp.GetFileName(), copyName );
         QCOMPARE( cp.GetLength(), bf.GetLength() );
         auto rms1 = cp.GetMinMaxRMS();
         auto rms2 = bf.GetMinMaxRMS();
         QCOMPARE( rms1.min, rms2.min );
         QCOMPARE( rms1.max, rms2.max );
         QCOMPARE( rms1.RMS, rms2.RMS );

         QVERIFY( ! QFileInfo(copyName).exists() );
      }

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
   }

   QCOMPARE( (int)BlockFile::gBlockFileDestructionCount, 2 );
   QVERIFY( ! QFileInfo(fileName).exists() );

   // directory doesn't exit
   QVERIFY_EXCEPTION_THROWN(
      make_blockfile<SimpleBlockFile>("bogusdir/audio", samples.ptr(), numSamples, fmt), AudacityException );


   // simulate low disk space
   {
      FileSizeLimiter limit;
      if (limit.apply( numSamples/2 * SAMPLE_SIZE_DISK(fmt)))
         QVERIFY_EXCEPTION_THROWN(
            make_blockfile<SimpleBlockFile>("lowdisk", samples.ptr(), numSamples, fmt), AudacityException );
      else
         QWARN("Low disk simulation unsupported");
   }

   // xml save/load
   QString xmlName = fileName+".xml";
   {
      auto bf = make_blockfile<SimpleBlockFile>(fileName, samples.ptr(), numSamples, fmt);
      XMLFileWriter writer(xmlName, "SimpleBlockFile");
      bf.get()->SaveXML(writer);
      writer.Commit(); // save xml to disk
      bf.get()->Lock(); // prevent file deletion

      QVERIFY( QFileInfo(xmlName).exists() );
   }

   {
      DirManager handler;
      XMLFileReader reader;
      QVERIFY( reader.Parse(&handler, xmlName) );

      auto& bf = *(_dirManagerBlockFile.get());
      auto m = bf.GetMinMaxRMS();
      QCOMPARE( bf.GetFileName(), fileName );
      QCOMPARE( m.min, minSample );
      QCOMPARE( m.max, maxSample );
      QCOMPARE( m.RMS, rms);
      QVERIFY( bf.GetSpaceUsage() >= numSamples*SAMPLE_SIZE_DISK(fmt));

      _dirManagerBlockFile.reset();
   }

   // blockfile should be gone since we didn't lock it above
   QVERIFY( ! QFileInfo(fileName).exists() );

   QFile(xmlName).remove();
}

};

QTEST_MAIN(TestSimpleBlockFile)
#include "TestSimpleBlockFile.moc"
