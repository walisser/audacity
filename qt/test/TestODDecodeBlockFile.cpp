#include "TestBlockFile.h"

#include "core/blockfile/ODDecodeBlockFile.h"

#include <QtTest>
#include "TestHelpers.h"

class TestODDecodeBlockFile : public TestBlockFile
{
   Q_OBJECT

virtual void buildFromXml(DirManager& dm, const QString& tag, const QStringMap& attrs) override
{
   Q_REQUIRE(tag == "oddecodelockfile");
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
   makeTestData(44100, t);


   QString baseFileName;

   {
      ODDecodeBlockFile obf(baseFileName, t.fileName, 0, t.numSamples,
                            0, 0);
      BlockFile& bf = obf;

      QVERIFY( !QFileInfo(bf.GetFileName()).exists() );
      QVERIFY( bf.GetLength() == t.numSamples );
      QVERIFY( ! bf.IsAlias() );
      QVERIFY( bf.IsSummaryAvailable() );
      QVERIFY( bf.IsDataAvailable() );
      QVERIFY( ! bf.IsSummaryBeingComputed() );
      QVERIFY( bf.GetSpaceUsage() == 0);

      checkLockUnlock(bf);
      checkCloseLock(bf);

      checkGetMinMaxRMS(bf, t);
      checkGetMinMaxRMSOverflows(bf, t);

      checkReadData(bf, t);

      // FIXME: SimpleBlockFile will zero into the overflow, unique behavior
      //checkReadDataOverflows(bf, t);

      checkReadSummary(256, bf, t);
      checkReadSummary(64*1024, bf, t);

      checkCopy(bf, true);
      checkSetFileName(bf);

      // Recover() does nothing, there's no data file
      bf.Recover();
      QVERIFY( bf.GetLength() == t.numSamples );
   }

   QCOMPARE( (int)BlockFile::gBlockFileDestructionCount, 2 );
   QVERIFY( ! QFileInfo(baseFileName).exists() );


   /*
   // xml save/load
   QString xmlName = "silence.xml";
   {
      auto bf = make_blockfile<SilentBlockFile>(t.numSamples);
      checkSaveXML(*bf, xmlName, "SilentBlockFile");
   }

   {
      checkLoadFromXML(xmlName, fileName, t);

      auto& bf = *(_loadedBlockFile.get());

      QVERIFY( bf.GetSpaceUsage() == 0 );
      checkGetMinMaxRMS(bf, t);

      _loadedBlockFile.reset();
   }

   QFile(xmlName).remove();
   */
}

};

QTEST_MAIN(TestODDecodeBlockFile)
#include "TestODDecodeBlockFile.moc"
