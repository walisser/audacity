#include <QtTest>

#include "core/xml/XMLTagHandler.h"
#include "core/Track.h"
#include "core/SampleFormat.h"

class TestXMLValueChecker : public QObject
{
   Q_OBJECT
private Q_SLOTS:
//void initTestCase() {}
//void cleanupTestCase() {}

void testIsGoodString()
{   
   QVERIFY(XMLValueChecker::IsGoodString(""));
   QVERIFY(XMLValueChecker::IsGoodString(QString()));
   
   // fail: string is the null character
   QVERIFY(!XMLValueChecker::IsGoodString(QChar()));

   // fail: extra null at the front
   QString badString1 = QString(QChar()) + "a";
   QVERIFY(!XMLValueChecker::IsGoodString(badString1));
   
   // fail: null in the middle
   QString badString2 = QString("a") + QChar() + "b";
   QVERIFY(!XMLValueChecker::IsGoodString(badString2));
   
   QVERIFY(XMLValueChecker::IsGoodString(makeString(PLATFORM_MAX_PATH)));
   QVERIFY(!XMLValueChecker::IsGoodString(makeString(PLATFORM_MAX_PATH + 1)));
}

void testIsGoodPathString()
{
   QVERIFY(!XMLValueChecker::IsGoodPathString(""));
   QVERIFY(XMLValueChecker::IsGoodPathString(makeString(PLATFORM_MAX_PATH)));
   QVERIFY(!XMLValueChecker::IsGoodPathString(makeString(PLATFORM_MAX_PATH + 1)));
}

void testIsGoodFileString()
{
   QVERIFY(!XMLValueChecker::IsGoodFileString(""));   
   QVERIFY(!XMLValueChecker::IsGoodFileString(PLATFORM_PATH_SEPARATOR));
   QVERIFY(XMLValueChecker::IsGoodFileString(makeString(1)));
   QVERIFY(XMLValueChecker::IsGoodFileString(makeString(260)));
   QVERIFY(!XMLValueChecker::IsGoodFileString(makeString(261)));   
}

void testIsGoodFileName()
{
   QVERIFY(!XMLValueChecker::IsGoodFileName("",""));
   
   // fixme: not testing exceeding max path
   // make a test file
   QString fileName = makeString(1);
   QString dirName = makeString(1);

   if (!QDir(dirName).exists())
      QVERIFY(QDir().mkdir(dirName));
   
   QVERIFY(QFile(dirName+PLATFORM_PATH_SEPARATOR+fileName).open(QFile::WriteOnly));
   
   QVERIFY(XMLValueChecker::IsGoodFileName(fileName, dirName));

   // clean up
   QVERIFY(QDir(dirName).remove(fileName));
   QVERIFY(QDir().rmdir(dirName));
}

void testIsGoodSubdirName()
{
   QVERIFY(!XMLValueChecker::IsGoodSubdirName("",""));
   QVERIFY(!XMLValueChecker::IsGoodSubdirName(".",""));
   QVERIFY(!XMLValueChecker::IsGoodSubdirName("..",""));
   
   QString childName = "child";
   QString parentName = "parent";
   
   // fixme: not testing exceeding max path
   QDir parent(parentName);
   if (!parent.exists())
      QVERIFY(QDir().mkdir(parentName));
   
   QDir child(parentName+PLATFORM_PATH_SEPARATOR+childName);
   if (!child.exists())
      QVERIFY(parent.mkdir(childName));
   
   QVERIFY(XMLValueChecker::IsGoodSubdirName(childName, parentName));

   // clean up
   QVERIFY(parent.rmdir(childName));
   QVERIFY(QDir().rmdir(parentName));
}

void testIsGoodPathName()
{
   QVERIFY(XMLValueChecker::IsGoodFileName(__FILE__));
   QVERIFY(!XMLValueChecker::IsGoodFileName("random-parent/random-child"));
}

void testIsGoodInt()
{
   QVERIFY(XMLValueChecker::IsGoodInt( QString::number(INT_MAX) ));
   QVERIFY(XMLValueChecker::IsGoodInt( QString::number(INT_MIN) ));
   
   QVERIFY(XMLValueChecker::IsGoodInt( "2147483647" ));
   QVERIFY(XMLValueChecker::IsGoodInt( "-2147483648" ));
   
   QVERIFY(!XMLValueChecker::IsGoodInt( "2147483648" ));
   QVERIFY(!XMLValueChecker::IsGoodInt( "-2147483649" ));
}

void testIsGoodInt64()
{
   QVERIFY(XMLValueChecker::IsGoodInt64( QString::number(INT64_MAX) ));
   QVERIFY(XMLValueChecker::IsGoodInt64( QString::number(INT64_MIN) ));
   
   QVERIFY(XMLValueChecker::IsGoodInt64( "9223372036854775807" ));
   QVERIFY(XMLValueChecker::IsGoodInt64( "-9223372036854775808" ));
   
   QVERIFY(!XMLValueChecker::IsGoodInt64( "9223372036854775808" ));
   QVERIFY(!XMLValueChecker::IsGoodInt64( "-9223372036854775809" ));
}

void testIsValidChannel()
{
   QVERIFY(!XMLValueChecker::IsValidChannel(Track::LeftChannel-1));
   QVERIFY(XMLValueChecker::IsValidChannel(Track::LeftChannel));
   QVERIFY(XMLValueChecker::IsValidChannel(Track::RightChannel));
   QVERIFY(XMLValueChecker::IsValidChannel(Track::MonoChannel));
   QVERIFY(!XMLValueChecker::IsValidChannel(Track::MonoChannel+1));   
}

void testIsValidVisibleChannels()
{
   QVERIFY(!XMLValueChecker::IsValidVisibleChannels(-1));
   QVERIFY(XMLValueChecker::IsValidVisibleChannels(0));
   QVERIFY(XMLValueChecker::IsValidVisibleChannels( (1<<16) - 1));
   QVERIFY(!XMLValueChecker::IsValidVisibleChannels(1<<16));
}

void testIsValidSampleFormat()
{
   QVERIFY(!XMLValueChecker::IsValidSampleFormat(int16Sample-1));
   QVERIFY(XMLValueChecker::IsValidSampleFormat(int16Sample));
   QVERIFY(XMLValueChecker::IsValidSampleFormat(int24Sample));
   QVERIFY(XMLValueChecker::IsValidSampleFormat(floatSample));
   QVERIFY(!XMLValueChecker::IsValidSampleFormat(floatSample+1));
}

private:
   QString makeString(int num)
   {
      QString str;
      for (int i = 0; i < num; i++)
         str += "a";
      return str;
   }
};

QTEST_MAIN(TestXMLValueChecker)
#include "TestXMLValueChecker.moc"
