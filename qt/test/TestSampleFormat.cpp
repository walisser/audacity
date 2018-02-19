#include <QtTest>

#include "core/SampleFormat.h"
#include "TestHelpers.h"

class TestSampleFormat : public QObject
{
   Q_OBJECT
private Q_SLOTS:

void initTestCase()
{
   InitDitherers();
}

//void cleanupTestCase() {}

void testGetSampleFormatStr()
{
   // use switch to get warned when formats are added
   QVector<sampleFormat> formats = { int16Sample, int24Sample, floatSample };
   for (auto fmt : formats)
      switch(fmt)
      {
         case int16Sample:
         case int24Sample:
         case floatSample:
            QVERIFY(GetSampleFormatStr(fmt) != "");
      }
}

void testSampleBuffer()
{
   const int count=100;

   SampleBuffer samples(count, int16Sample);

   QVERIFY(samples.ptr());
   QVERIFY( MALLOC_SIZE(samples.ptr()) >= SAMPLE_SIZE(int16Sample)*count );
   samples.Free();
   QVERIFY(samples.ptr() == nullptr);
}

void testGrowableSampleBuffer()
{
   const int count=1000;

   GrowableSampleBuffer samples(count, int16Sample);
   QVERIFY(samples.ptr());
   QVERIFY( MALLOC_SIZE(samples.ptr()) >= SAMPLE_SIZE(int16Sample)*count );
   samples.Free();
   QVERIFY(samples.ptr() == nullptr);

   samples.Resize(count, int16Sample);
   QVERIFY( MALLOC_SIZE(samples.ptr()) >= SAMPLE_SIZE(int16Sample)*count );

   samples.Resize(count*2, int16Sample);
   QVERIFY( MALLOC_SIZE(samples.ptr()) >= SAMPLE_SIZE(int16Sample)*count*2 );

   // FIXME: GrowableSampleBuffer doesn't resize buffers if format change requires it
   samples.Resize(count*2, floatSample);
   QEXPECT_FAIL("", "FIXME: Buffer Resizing", Continue);
   QVERIFY( MALLOC_SIZE(samples.ptr()) >= SAMPLE_SIZE(floatSample)*count*2 );
}

void testCopySamples_int16_to_int16()
{
   copySamples<int16_t>(int16Sample, 100, 1);
   copySamples<int16_t>(int16Sample, 100, 2);
   copySamples<int16_t>(int16Sample, 99, 3);
   copySamples<int16_t>(int16Sample, 100, 4);
}

void testCopySamples_int24_to_int24()
{
   copySamples<int32_t>(int24Sample, 100, 1);
   copySamples<int32_t>(int24Sample, 100, 2);
   copySamples<int32_t>(int24Sample, 99, 3);
   copySamples<int32_t>(int24Sample, 100, 4);
}

void testCopySamples_float_to_float()
{
   copySamples<float>(floatSample, 100, 1);
   copySamples<float>(floatSample, 100, 2);
   copySamples<float>(floatSample, 99, 3);
   copySamples<float>(floatSample, 100, 4);
}

void testCopySamples_int16_to_float()
{
   copySamples<int16_t,float>(0, 1, 1, int16Sample, floatSample, 100, 1);
   copySamples<int16_t,float>(0, 1, 1, int16Sample, floatSample, 100, 2);
   copySamples<int16_t,float>(0, 1, 1, int16Sample, floatSample, 99, 3);
   copySamples<int16_t,float>(0, 1, 1, int16Sample, floatSample, 100, 4);
}

void testCopySamples_int24_to_float()
{
   copySamples<int32_t,float>(0, 1, 1, int24Sample, floatSample, 100, 1);
   copySamples<int32_t,float>(0, 1, 1, int24Sample, floatSample, 100, 2);
   copySamples<int32_t,float>(0, 1, 1, int24Sample, floatSample, 99, 3);
   copySamples<int32_t,float>(0, 1, 1, int24Sample, floatSample, 100, 4);
}

void testCopySamples_int16_to_int24()
{
   copySamples<int16_t,int32_t>(0, 1, 1, int16Sample, int24Sample, 100, 1);
   copySamples<int16_t,int32_t>(0, 1, 1, int16Sample, int24Sample, 100, 2);
   copySamples<int16_t,int32_t>(0, 1, 1, int16Sample, int24Sample, 99, 3);
   copySamples<int16_t,int32_t>(0, 1, 1, int16Sample, int24Sample, 100, 4);
}

void testCopySamples_float_to_int16()
{
   copySamplesDithered<float,int16_t>(-1, 0, 0.01, floatSample, int16Sample, 100, 1);
}

void testCopySamples_float_to_int24()
{
   copySamplesDithered<float,int32_t>(-1, 0, 0.01, floatSample, int24Sample, 100, 1);
}

// TESTME: different dither code paths; must use gPrefs to set the ditherer,
// the use InitDitherers to apply the setting

void testCopySamplesNoDither()
{
}

void testClearSamples()
{
   // ClearSamples depends on 0.0f == 0x0
   union {
      int32_t i;
      float f;
   } u;
   u.i = 0;
   QVERIFY(u.f == 0.0f);
   u.f = 0.0f;
   QVERIFY(u.i == 0);

   const int count = 100;
   SampleBuffer s1(count, int16Sample);

   int16_t* src=(int16_t*)s1.ptr();
   for (int i = 0; i < count; i++)
      src[i] = i;

   ClearSamples(s1.ptr(), int16Sample, 0, count);

   for (int i = 0; i < count; i++)
      QVERIFY(src[i] == 0);
}

void testReverseSamples()
{
   const int count = 100;
   SampleBuffer s1(count, int16Sample);

   int16_t* src=(int16_t*)s1.ptr();
   for (int i = 0; i < count; i++)
      src[i] = i;

   ReverseSamples(s1.ptr(), int16Sample, 0, count);

   for (int i = 0; i < count; i++)
      QVERIFY(src[i] == count-i-1);
}

private:

// sample formats are the same
template<typename T >
void copySamples(sampleFormat fmt, const int count, int stride)
{
   SampleBuffer s1(count, fmt);
   SampleBuffer s2(count, fmt);

   T* src=(T*)s1.ptr();
   T* dst=(T*)s2.ptr();

   for (int i = 0; i < count; i++)
   {
      src[i] = i;
      dst[i] = 0;
   }

   // needs to be divisible for check to work
   QVERIFY(count % stride == 0);
   int len = count / stride;

   CopySamplesNoDither(s1.ptr(), fmt, s2.ptr(), fmt, len, stride, stride);
   for (int i = 0; i < count; i++)
   {
      //qDebug("%d,%d,%d", i, (int)src[i], (int)dst[i]);
      if (i % stride == 0)
         QCOMPARE( dst[i], (T)i );
      else
         QCOMPARE( dst[i], (T)0 );
   }
}

// sample formats differ, no dithering
template<typename srcType, typename dstType>
void copySamples(
   srcType start, srcType step,
   dstType emptyValue, sampleFormat srcFormat, sampleFormat dstFormat, const int count, int stride)
{
   SampleBuffer s1(count, srcFormat);
   SampleBuffer s2(count, dstFormat);

   srcType* src=(srcType*)s1.ptr();
   dstType* dst=(dstType*)s2.ptr();

   for (int i = 0; i < count; i++)
   {
      src[i] = start + i*step;
      dst[i] = emptyValue;
   }

   // needs to be divisible for check to work
   QVERIFY(count % stride == 0);
   int len = count / stride;

   CopySamplesNoDither(s1.ptr(), srcFormat, s2.ptr(), dstFormat, len, stride, stride);

   dstType lastValue;
   for (int i = 0; i < count; i++)
   {
      //qDebug("%d,%d,%d", i, (int)src[i], (int)dst[i]);
      if (i % stride == 0)
      {
         QVERIFY(dst[i] != emptyValue); // the value changed

         if (i > 0)
            QVERIFY(dst[i] >= lastValue);  // shouldn't be the same value repeated
         lastValue = dst[i];

         //QCOMPARE( dst[i], (dstType)i ); // this won't hold due to conversion
      }
      else
         QCOMPARE( dst[i], emptyValue );
   }
}

// sample format differ, dithered
template<typename srcType, typename dstType>
void copySamplesDithered(
   srcType start, srcType step,
   dstType emptyValue, sampleFormat srcFormat, sampleFormat dstFormat, const int count, int stride)
{
   SampleBuffer s1(count, srcFormat);
   SampleBuffer s2(count, dstFormat);

   srcType* src=(srcType*)s1.ptr();
   dstType* dst=(dstType*)s2.ptr();

   for (int i = 0; i < count; i++)
   {
      src[i] = start + i*step;
      dst[i] = emptyValue;
   }

   // needs to be divisible for check to work
   QVERIFY(count % stride == 0);
   int len = count / stride;

   CopySamples(s1.ptr(), srcFormat, s2.ptr(), dstFormat, len, true, stride, stride);

   for (int i = 0; i < count; i++)
   {
      //qDebug("%d,%d,%d", i, (int)src[i], (int)dst[i]);
      if (i % stride == 0)
         QVERIFY(dst[i] != emptyValue); // the value changed
      else
         QCOMPARE( dst[i], emptyValue );
   }
}


};

QTEST_MAIN(TestSampleFormat)
#include "TestSampleFormat.moc"
