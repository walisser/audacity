#include "TestBlockFile.h"

#include "core/AudacityException.h"
#include "core/xml/XMLWriter.h"
#include "core/xml/XMLFileReader.h"
#include "core/DirManager.h"
#include "core/FileFormats.h"

#include <QtTest>
#include "TestHelpers.h"


TestBlockFile* _implementation = nullptr;

// stubs
#ifndef TESTING_DIRMANAGER
DirManager::DirManager() {}
DirManager::~DirManager() {}

bool DirManager::HandleXMLTag(const QString &tag, const QStringMap &attrs)
{
   // subclass must assign this in initTestcase() slot
   Q_REQUIRE(_implementation != nullptr);
   _implementation->buildFromXml(*this, tag, attrs);

   /*
   if (tag == "pcmaliasblockfile")
      _loadedBlockFile = PCMAliasBlockFile::BuildFromXML(*this, attrs);
   else
   if (tag == "odpcmaliasblockfile")
      _loadedBlockFile = ODPCMAliasBlockFile::BuildFromXML(*this, attrs);
   else
   if (tag == "silentblockfile")
      _loadedBlockFile = SilentBlockFile::BuildFromXML(*this, attrs);
   else
   if (tag == "simpleblockfile")
      _loadedBlockFile = SimpleBlockFile::BuildFromXML(*this, attrs);
   else
      qFatal("DirManager::HandleXMLTag stub: unknown tag: %s", qPrintable(tag));
   */

   return true;
}

bool DirManager::AssignFile(QString &fileName, const QString &value, bool check)
{
   (void)check;
   fileName = value;
   return true;
}
// end stubs
#endif

// zeroed test data (for SilentBlockFile)
void TestBlockFile::makeTestData(size_t numSamples, TestData& t) const
{
   t.numSamples = numSamples;
   t.fmt = int16Sample; // don't care
   t.samples.Allocate(t.numSamples, t.fmt);
   setZero(t.samples, t.fmt, 0, t.numSamples);
   t.minSample=t.maxSample=t.rms = 0.0f;
}

void TestBlockFile::writeTestFile(const TestData& t) const
{
   // write our test buffer to a .wav file using libsndfile
   SF_INFO sfInfo;
   memset(&sfInfo, 0, sizeof(sfInfo));
   sfInfo.channels = 1;
   int format = 0;
   if (t.fileName.endsWith(".wav"))
      format |= SF_FORMAT_WAV;
   else if (t.fileName.endsWith(".flac"))
      format |= SF_FORMAT_FLAC;
   else
   {
      if (!t.fileName.isEmpty())
         QWARN("Not writing test file");
      return;
   }

   switch (t.fmt)
   {
   case int16Sample: format |= SF_FORMAT_PCM_16; break;
   case int24Sample: format |= SF_FORMAT_PCM_24; break;
   case floatSample: format |= SF_FORMAT_FLOAT;  break;
   default:
      QFAIL("Unsupported sample format");
   }

   sfInfo.format = format;
   sfInfo.samplerate = 44100;

   // if this fails the format combination is invalid
   QVERIFY( sf_format_check(&sfInfo) );

   QFile(t.fileName).remove();

   {
   SFFile file;
   file.reset( SFCall<SNDFILE*>(::sf_open, qPrintable(t.fileName), SFM_WRITE, &sfInfo) );
   QVERIFY(file != nullptr);

   if (t.fmt == int16Sample)
      QVERIFY( (sf_count_t)t.numSamples == SFCall<sf_count_t>(sf_write_short, file.get(), (const short*)t.samples.ptr(), t.numSamples) );
   else if (t.fmt == int24Sample)
      QVERIFY( (sf_count_t)t.numSamples == SFCall<sf_count_t>(sf_write_int, file.get(), (const int*)t.samples.ptr(), t.numSamples) );
   else
      QVERIFY( (sf_count_t)t.numSamples == SFCall<sf_count_t>(sf_write_float, file.get(), (const float*)t.samples.ptr(), t.numSamples) );
   }

   QVERIFY( QFileInfo(t.fileName).exists() );
}

// sequential samples from -/+ INT16_MAX
void TestBlockFile::makeTestData(const QString& fileName_, TestData& t) const
{
   t.fileName = fileName_;
   t.numSamples = INT16_MAX*2 + 1; // +1 to include 0 sample
   t.fmt = int16Sample;
   t.minSample = -INT16_MAX/float(1<<15),
   t.maxSample=INT16_MAX/float(1<<15),
   t.rms=0.577341f;

   t.samples.Allocate(t.numSamples, t.fmt);
   int16_t *samplePtr = (int16_t*)t.samples.ptr();
   for (size_t i = 0; i < t.numSamples; i++)
      samplePtr[i] = int16_t( -INT16_MAX + i );

   QCOMPARE((int)samplePtr[0], -INT16_MAX);
   QCOMPARE((int)samplePtr[t.numSamples-1], INT16_MAX);

   writeTestFile(t);
}

// sequential sampels from -/+ INT24_MAX
void TestBlockFile::makeTestData24Bit(const QString& fileName_, TestData& t) const
{
   const int INT24_MAX = (1<<23)-1;

   t.fileName = fileName_;
   t.numSamples = (INT24_MAX>>7)+1; // 64k samples
   t.fmt = int24Sample;
   t.minSample = -INT24_MAX/float(1<<23);
   t.maxSample = INT24_MAX/float(1<<23);
   t.rms = 0.57735f;

   t.samples.Allocate(t.numSamples, t.fmt);
   int32_t *samplePtr = (int32_t*)t.samples.ptr();
   for (size_t i = 0; i < t.numSamples; i++)
      samplePtr[i] = int32_t( -INT24_MAX + i*256 );

   // sequence doesn't work out...
   samplePtr[t.numSamples-1] = INT24_MAX;

   QCOMPARE((int)samplePtr[0], -INT24_MAX);
   QCOMPARE((int)samplePtr[t.numSamples-1], INT24_MAX);

   writeTestFile(t);
}

// test buffer with min/max of -/+ 1.0f
void TestBlockFile::makeTestDataFloat(const QString& filename_, TestData& t) const
{
   t.fileName = filename_;
   t.numSamples = 40000 + 1;
   t.fmt = floatSample;
   t.minSample = -1.0f;
   t.maxSample = 1.0f;
   t.rms = 0.577365f;

   t.samples.Allocate(t.numSamples, t.fmt);
   float *samplePtr = (float*)t.samples.ptr();
   for (size_t i = 0; i < t.numSamples; i++)
      samplePtr[i] = -1.0f + i*2.0f/40000;

   QCOMPARE(samplePtr[0], -1.0f);
   QCOMPARE(samplePtr[t.numSamples-1], 1.0f);

   writeTestFile(t);
}

void TestBlockFile::checkLockUnlock(BlockFile& bf, ExpectFailure fails)
{
   QVERIFY( ! bf.IsLocked() );
   bf.Lock();
   if (!fails)
      QVERIFY( bf.IsLocked() );
   bf.Unlock();
   QVERIFY( ! bf.IsLocked() );
}

void TestBlockFile::checkCloseLock(BlockFile& bf, ExpectFailure fails)
{
   QVERIFY( ! bf.IsLocked() );
   bf.CloseLock();
   if (!fails)
      QVERIFY( bf.IsLocked() );
   bf.Unlock();
   QVERIFY( ! bf.IsLocked() );
}

void TestBlockFile::checkGetMinMaxRMS(BlockFile& bf, TestData&t, ExpectDefault defaults, ExpectThrow throws)
{
   if (!throws)
   {
      if (defaults)
      {
         // values should be filled with sane defaults
         auto m = bf.GetMinMaxRMS();
         QCOMPARE( m.min, -JUST_BELOW_MAX_AUDIO );
         QCOMPARE( m.max, JUST_BELOW_MAX_AUDIO );
         QCOMPARE( m.RMS, 0.707f);
      }
      else
      {
         // values should be good
         auto m = bf.GetMinMaxRMS();
         QCOMPARE( m.min, t.minSample );
         QCOMPARE( m.max, t.maxSample );
         QCOMPARE( m.RMS, t.rms);
      }
   }
   else
   {
      // not required to throw, but might depends on situation/implementation
      QVERIFY_EXCEPTION_THROWN( bf.GetMinMaxRMS(), AudacityException );
   }
}

void TestBlockFile::checkGetMinMaxRMSOverflows(BlockFile& bf, TestData& t, ExpectThrow throws)
{
   // overflow, must not throw
   (void)bf.GetMinMaxRMS(0, t.numSamples+1, false);

   // overflow, may or may not throw, depends on situation/implementation
   if (throws)
      QVERIFY_EXCEPTION_THROWN( bf.GetMinMaxRMS(0, t.numSamples+1), AudacityException );
}

void TestBlockFile::checkReadData(BlockFile& bf, TestData& t, ExpectThrow throws)
{
   SampleBuffer dst(t.numSamples, t.fmt);
   setOne(dst, t.fmt, 0, t.numSamples);

   if (!throws)
   {
      QVERIFY( t.numSamples == bf.ReadData(dst.ptr(), t.fmt, 0, t.numSamples) );
      QVERIFY( isEqual(t.samples, dst, t.fmt, t.numSamples) );
   }
   else
   {
      QVERIFY_EXCEPTION_THROWN( bf.ReadData(dst.ptr(), t.fmt, 0, t.numSamples), AudacityException );

      // exception thrown, destination untouched
      QVERIFY( isOne(dst, t.fmt, 0, t.numSamples) );

      // not allowed to throw
      QVERIFY( 0 == bf.ReadData(dst.ptr(), t.fmt, 0, t.numSamples, false) );

      // destination should be zeroed
      QVERIFY( isZero(dst, t.fmt, 0, t.numSamples) );
   }
}

void TestBlockFile::checkReadDataOverflows(BlockFile& bf, TestData& t, ExpectThrow throws)
{
   size_t overLen = t.numSamples+10;
   SampleBuffer over(overLen, t.fmt);
   setOne(over, t.fmt, 0, overLen);

   // overflow: no throw allowed, overflow is zero-filled
   QCOMPARE( t.numSamples, bf.ReadData(over.ptr(), t.fmt, 0, overLen, false) );
   QVERIFY( isEqual(t.samples, over, t.fmt, t.numSamples) );
   QVERIFY( isZero(over, t.fmt, t.numSamples, overLen-t.numSamples) );

   setOne(over, t.fmt, 0, overLen);
   if (throws)
   {
      // throw allowed and expected
      QVERIFY_EXCEPTION_THROWN( bf.ReadData(over.ptr(), t.fmt, 0, overLen), AudacityException );
      QVERIFY( memcmp(over.ptr(), t.samples.ptr(), SAMPLE_SIZE(t.fmt)*t.numSamples) == 0 );
      QVERIFY( isOne(over, t.fmt, t.numSamples, overLen-t.numSamples) );
   }
   else
   {
      // throw allowed, not expected
      (void)bf.ReadData(over.ptr(), t.fmt, 0, overLen);
      QVERIFY( memcmp(over.ptr(), t.samples.ptr(), SAMPLE_SIZE(t.fmt)*t.numSamples) == 0 );
      QVERIFY( isOne(over, t.fmt, t.numSamples, overLen-t.numSamples) );
   }
}

void TestBlockFile::checkReadSummary(int chunkSize, BlockFile& bf, TestData& t,
                      ExpectDefault defaults)
{
   // if it doesn't divide evenly, get one more
   size_t summaryLen = t.numSamples/chunkSize;
   if (t.numSamples % chunkSize != 0)
      summaryLen++;

   ArrayOf<BlockFile::MinMaxRMS> summaryData( summaryLen );
   size_t byteLen = summaryLen*sizeof(BlockFile::MinMaxRMS);
   memset(summaryData.get(), 0xff, byteLen);
   QVERIFY( isOne(summaryData.get(), byteLen) );

   bool readOk = false;

   if (chunkSize == 256)
      readOk = bf.Read256((float*)summaryData.get(), 0, summaryLen);
   else
   if (chunkSize == 64*1024)
      readOk = bf.Read64K((float*)summaryData.get(), 0, summaryLen);
   else
      QFAIL("Unsupported chunk size");

   // the min/max of first/last summary respectively should
   // be the same as min/max of the test file.
   // only because test file was specially crafted
   if (defaults)
   {
      // only way to get defaults is if read fails
      QVERIFY( !readOk );
      QCOMPARE( summaryData.get()[0].min, 0.0f );
      QCOMPARE( summaryData.get()[summaryLen-1].max, 0.0f );
      QVERIFY( isZero(summaryData.get(), byteLen) );
   }
   else
   {
      QVERIFY( readOk );
      QCOMPARE( summaryData.get()[0].min, t.minSample );
      QCOMPARE( summaryData.get()[summaryLen-1].max, t.maxSample );
   }
}

void TestBlockFile::checkCopy(BlockFile& bf, bool isSilentBlockFile)
{
   QString copyName = "copy.auf";
   BlockFilePtr copy = bf.Copy(copyName);
   BlockFile& cp = *(copy.get());

   // silent block file has no filename
   if (isSilentBlockFile)
      QCOMPARE( cp.GetFileName(), QString("") );
   else
      QCOMPARE( cp.GetLength(), bf.GetLength() );

   // this may fail under some conditions,
   // like OD*BlockFile in limbo, so don't allow throw
   // the values are still required to be copied!
   auto rms1 = cp.GetMinMaxRMS(false);
   auto rms2 = bf.GetMinMaxRMS(false);

   QCOMPARE( rms1.min, rms2.min );
   QCOMPARE( rms1.max, rms2.max );
   QCOMPARE( rms1.RMS, rms2.RMS );

   // should not exist yet on disk
   QVERIFY( ! QFileInfo(copyName).exists() );
}

void TestBlockFile::checkSetFileName(BlockFile& bf)
{
   QString oldName = bf.GetFileName();
   QString newName = oldName+".new";
   bf.SetFileName(newName);
   QVERIFY(bf.GetFileName() == newName);
   bf.SetFileName(oldName);
   QVERIFY(bf.GetFileName() == oldName);
}

void TestBlockFile::checkRecover(BlockFile& bf)
{
   QString fileName = bf.GetFileName();
   QVERIFY( QFileInfo(fileName).exists() );
   QFile(fileName).remove();
   bf.Recover();
   QVERIFY( QFileInfo(fileName).exists() );
}

void TestBlockFile::checkSaveXML(BlockFile& bf, const QString& xmlName, const QString& caption)
{
   XMLFileWriter writer(xmlName, caption);
   bf.SaveXML(writer);
   writer.Commit();
   QVERIFY( QFileInfo(xmlName).exists() );
}

void TestBlockFile::checkLoadFromXML(const QString& xmlName, const QString& fileName, TestData& t)
{
   DirManager handler;
   XMLFileReader reader;
   QVERIFY( reader.Parse(&handler, xmlName) );

   auto& bf = *(_loadedBlockFile.get());

   QCOMPARE( bf.GetFileName(), fileName );
   QCOMPARE( bf.GetLength(), t.numSamples );
}
