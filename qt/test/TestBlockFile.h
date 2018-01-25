
//
// Common code for BlockFile tests
//
// This file should be #included from a test SOURCE file TextXXX.cpp
// Then subclass TestBlockFile
//
#pragma once
#include "core/BlockFile.h"

// container for a test file/buffer and its properties
class TestData
{
public:
   QString fileName;
   size_t numSamples;
   sampleFormat fmt;
   float minSample;
   float maxSample;
   float rms;
   SampleBuffer samples;
};

class DirManager;
class TestBlockFile;
extern TestBlockFile* _implementation; // must be assigned to "this" in initTestcase()

// common tests for all subclasses of BlockFile to use
class TestBlockFile : public QObject
{
   friend class DirManager;

protected:
   BlockFilePtr _loadedBlockFile;

   // DirManager mockup: subclasses implement to call ::BuildFromXML(dm, attr) and store to _loadedBlockFile
   virtual void buildFromXml(DirManager& dm, const QString& tag, const QStringMap& attrs) = 0;

   // zeroed test data (for SilentBlockFile)
   void makeTestData(size_t numSamples, TestData& t);

   // sequential samples from -INT16_MAX to INT16_MAX
   // also writes test buffer to a file
   void makeTestData(const QString& fileName_, TestData& t);


   // setup a test buffer so its min/max is -/+ INT24_MAX
   void makeTestData24Bit(const QString& fileName_, TestData& t);


   // setup a test buffer so its min/max is -/+ 1.0f
   void makeTestDataFloat(const QString& filename_, TestData& t);


   // use typedefs for checks that have multiple modes
   // - to guard incorrect use of test functions
   // - to annotate what condition is tested
   typedef enum {
      EXPECT_DEFAULTS = true,     // default values are expected (e.g. on failure)
      EXPECT_NO_DEFAULTS = false, // default values are not expected
   } ExpectDefault;

   typedef enum {
      EXPECT_THROW = true,
      EXPECT_NO_THROW = false
   } ExpectThrow;

   typedef enum {
      EXPECT_FAILURE = true,
      EXPECT_NO_FAILURE = false
   } ExpectFailure;


   // Note: methods named checkXXX instead of testXXX because they
   // are not callable by the test driver (due to taking arguments)
   void checkLockUnlock(BlockFile& bf, ExpectFailure fails=EXPECT_NO_FAILURE);

   void checkCloseLock(BlockFile& bf, ExpectFailure fails=EXPECT_NO_FAILURE);

   void checkGetMinMaxRMS(BlockFile& bf, TestData&t,
                          ExpectDefault defaults = EXPECT_NO_DEFAULTS,
                          ExpectThrow throws = EXPECT_NO_THROW);

   void checkGetMinMaxRMSOverflows(BlockFile& bf, TestData& t,
                                   ExpectThrow throws = EXPECT_NO_THROW);

   void checkReadData(BlockFile& bf, TestData& t);


   void checkReadDataOverflows(BlockFile& bf, TestData& t,
                               ExpectThrow throws = EXPECT_NO_THROW);

   void checkReadSummary(int chunkSize, BlockFile& bf, TestData& t,
                         ExpectDefault defaults = EXPECT_NO_DEFAULTS);

   void checkCopy(BlockFile& bf, bool isSilentBlockFile=false);

   void checkSetFileName(BlockFile& bf);

   void checkRecover(BlockFile& bf);

   void checkSaveXML(BlockFile& bf, const QString& xmlName, const QString& caption);

   void checkLoadFromXML(const QString& xmlName, const QString& fileName, TestData& t);
};
