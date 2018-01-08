#include <QtTest>

#include "core/blockfile/SimpleBlockFile.h"
#include "core/DirManager.h"
#include "core/Internat.h"
#include "core/AudacityException.h"
#include "core/xml/XMLWriter.h"

#include "TestHelpers.h"

// stubs for stuff that isn't ported yet
QString DirManager::GetProjectDataDir()
{
   return ".";
}

bool DirManager::AssignFile(QString &filename, const QString &value, bool check)
{
   Q_UNUSED(filename);
   Q_UNUSED(value);
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

class XMLDummyWriter : public XMLWriter
{
public:
   QString xml;
   void Write(const QString& data) { xml += data; }
};

class TestBlockFile : public QObject
{
   Q_OBJECT
private Q_SLOTS:

//void initTestCase() {}

//void cleanupTestCase() {}

void testSimpleBlockFile()
{
   // setup a test buffer so its min/max is -/+ INT16_MAX
   const size_t numSamples = INT16_MAX*2 + 1;
   const sampleFormat fmt = int16Sample;
   QString fileName = "audio";
   
   SampleBuffer samples(numSamples, fmt);
   int16_t *samplePtr = (int16_t*)samples.ptr();
   for (size_t i = 0; i < numSamples; i++)
      samplePtr[i] = int16_t( -INT16_MAX + i );

   QCOMPARE((int)samplePtr[0], -INT16_MAX);
   QCOMPARE((int)samplePtr[numSamples-1], INT16_MAX);
   
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
      QVERIFY( bf.GetSpaceUsage() >= numSamples*SAMPLE_SIZE(fmt));
      
      QVERIFY( ! bf.IsLocked() );
      bf.Lock();
      bf.Unlock();
      QVERIFY( ! bf.IsLocked() );
      
      // GetMinMaxRMS()
      auto m = bf.GetMinMaxRMS();
      QCOMPARE( m.min, -INT16_MAX / float(1<<15) );
      QCOMPARE( m.max, INT16_MAX / float(1<<15) );
      QCOMPARE( m.RMS, 0.577341f);
      
      // try to read too much
      (void)bf.GetMinMaxRMS(0, numSamples+1, false); // must not throw
      QVERIFY_EXCEPTION_THROWN( bf.GetMinMaxRMS(0, numSamples+1), AudacityException );
      
      
      // ReadData()
      SampleBuffer dst(numSamples, fmt);
      ClearSamples(dst.ptr(), fmt, 0, numSamples);
      QVERIFY( numSamples == bf.ReadData(dst.ptr(), fmt, 0, numSamples) );
      QVERIFY( memcmp(dst.ptr(), samples.ptr(), SAMPLE_SIZE(fmt)*numSamples) == 0 );
      
      // overflow: no throw allowed, zero-fill the remainder
      size_t overLen = numSamples+10;
      SampleBuffer over(overLen, fmt);
      memset(over.ptr(), 0xff, overLen*SAMPLE_SIZE(fmt));
      QCOMPARE( numSamples, bf.ReadData(over.ptr(), fmt, 0, overLen, false) );
      QVERIFY( memcmp(over.ptr(), samples.ptr(), SAMPLE_SIZE(fmt)*numSamples) == 0 );
      for (size_t i = numSamples; i < overLen; i++)
         QVERIFY( ((int16_t*)over.ptr())[i] == 0 );
      
      // overflow: expect exception but overflow is not zero filled
      memset(over.ptr(), 0xff, overLen*SAMPLE_SIZE(fmt));
      QVERIFY_EXCEPTION_THROWN( bf.ReadData(over.ptr(), fmt, 0, overLen), AudacityException );
      QVERIFY( memcmp(over.ptr(), samples.ptr(), SAMPLE_SIZE(fmt)*numSamples) == 0 );
      for (size_t i = numSamples; i < overLen; i++)
         QVERIFY( ((uint16_t*)over.ptr())[i] == 0xffff );
      
      
      // Read256()
      {
         Q_STATIC_ASSERT( sizeof(BlockFile::MinMaxRMS) == sizeof(float)*3 );
         
         // doesn't divide evenly, get the remainder at the end
         size_t summaryLen = numSamples / 256 + 1;
         ArrayOf<BlockFile::MinMaxRMS> summaryData( summaryLen );
         
         QVERIFY( bf.Read256((float*)summaryData.get(), 0, summaryLen) );
         
         // the first/last summary block should have the same values as the
         // whole block
         QCOMPARE( summaryData.get()[0].min,           -INT16_MAX / float(1<<15) );
         QCOMPARE( summaryData.get()[summaryLen-1].max, INT16_MAX / float(1<<15) );
      }
      
      // Read64K()
      {
         // doesn't divide evenly, get the remainder at the end
         size_t summaryLen = numSamples / (64*1024) + 1;
         ArrayOf<BlockFile::MinMaxRMS> summaryData( summaryLen );
         
         QVERIFY( bf.Read64K((float*)summaryData.get(), 0, summaryLen) );
         
         // the first/last summary block should have the same values as the
         // whole block
         QCOMPARE( summaryData.get()[0].min,           -INT16_MAX / float(1<<15) );
         QCOMPARE( summaryData.get()[summaryLen-1].max, INT16_MAX / float(1<<15) );
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
         QVERIFY( bf.GetSpaceUsage() >= numSamples*SAMPLE_SIZE(fmt));
         SampleBuffer dst(numSamples, fmt);
         QVERIFY( numSamples == bf.ReadData(dst.ptr(), fmt, 0, numSamples) );
         for (size_t i = 0; i < numSamples; i++)
            QCOMPARE( ((int16_t*)dst.ptr())[i], (int16_t)0 );
         
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
      if (limit.apply( numSamples/2 * SAMPLE_SIZE(fmt)))
         QVERIFY_EXCEPTION_THROWN( 
            make_blockfile<SimpleBlockFile>("lowdisk", samples.ptr(), numSamples, fmt), AudacityException );
      else
         QWARN("Low disk simulation unsupported");
   }
   
   {
      auto bf = make_blockfile<SimpleBlockFile>(fileName, samples.ptr(), numSamples, fmt);
      XMLDummyWriter xml;
      bf.get()->SaveXML(xml);
   }
}

};

QTEST_MAIN(TestBlockFile)
#include "TestBlockFile.moc"
