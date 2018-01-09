#include <QtTest>

#include "core/xml/XMLWriter.h"
#include "core/xml/XMLTagHandler.h"
#include "core/xml/XMLFileReader.h"
#include "core/AudacityException.h"

#include <float.h>

#include "TestHelpers.h"

class TestFailXMLTagHandler : public XMLTagHandler
{
   bool HandleXMLTag(const QString& tag, const QStringMap& attrs) override
   {
      (void)tag; (void)attrs;
      return false;
   }

   XMLTagHandler* HandleXMLChild(const QString& tag) override
   {
      (void)tag;
      return this;
   }
};

class TestXMLWriter : public QObject, public XMLTagHandler
{
   Q_OBJECT
private Q_SLOTS:
//void initTestCase() {}
//void cleanupTestCase() {}

void testEscape()
{
   QCOMPARE(QString("&apos;&quot;&amp;&lt;&gt;"),
            XMLWriter::XMLEsc("\'\"&<>"));

   QString surrogates;
   surrogates.append(QChar(0xd800));
   surrogates.append(QChar(0xdfff));
   QCOMPARE(surrogates, XMLWriter::XMLEsc(surrogates));

   // strip both surrogates
   QString badSurrogates;
   badSurrogates.append(QChar(0xdfff));
   badSurrogates.append(QChar(0xd800));
   QCOMPARE(QString(""), XMLWriter::XMLEsc(badSurrogates));

   // strip the unmatched high surrogate
   badSurrogates = QChar(0xd800) + (QChar(0xd800) + badSurrogates);
   QCOMPARE(surrogates, XMLWriter::XMLEsc(badSurrogates));

   // expat nukeage
   QCOMPARE( QString(""), XMLWriter::XMLEsc(QChar(0x04)) );
   QCOMPARE( QString(""), XMLWriter::XMLEsc(QChar(0x02)) );

   // noncharacters
   QCOMPARE( QString(""), XMLWriter::XMLEsc(QChar(0xFFFE)) );
   QCOMPARE( QString(""), XMLWriter::XMLEsc(QChar(0xFFFF)) );

   // unicode escape
   QCOMPARE( QString("&#x007f;"), XMLWriter::XMLEsc(QChar(0x007f)) );
   QCOMPARE( QString("&#x0080;"), XMLWriter::XMLEsc(QChar(0x0080)) );
}

void testStringWriter()
{
   XMLStringWriter w;
   writeTestData(w);

   //qDebug("%s", w.Get().data());

   QTemporaryFile xmlFile("testStringWriter.xml");
   QVERIFY( xmlFile.open() );
   QVERIFY( xmlFile.write(w.Get()) == w.Get().size() );
   xmlFile.close();
   verifyTestData(xmlFile.fileName());
}

void testFileWriter()
{
   const QString fileName = "testFileWriter.xml";
   const QString caption = "Test Caption";
   QFile(fileName).remove();

   XMLFileWriter w(fileName, caption);

   writeTestData(w);
   QVERIFY( !QFileInfo(fileName).exists() ); // not committed yet

   w.Commit();
   QVERIFY( QFileInfo(fileName).exists() );
   verifyTestData(fileName);
   QFile(fileName).remove();
}

void testFileWriterException()
{
   QVERIFY_EXCEPTION_THROWN(
      XMLFileWriter("bad/file/name.xml", "Test Bad File Name"), AudacityException );

   // TESTME: ensure existing file is not touched, if there was a problem...
   {
      FileSizeLimiter limit;
      if (limit.apply(10))
      {
         QString fileName="full-disk.xml";
         QFile(fileName).remove();
         XMLFileWriter w(fileName, "Test Full Disk");
         writeTestData(w);
         QVERIFY_EXCEPTION_THROWN( w.Commit(), AudacityException );
         QVERIFY( !QFileInfo(fileName).exists() );
      }
   }

   {
      FileSizeLimiter limit;
      if (limit.apply(10))
      {
         QString fileName="almost-full-disk.xml";
         QFile(fileName).remove();
         XMLFileWriter w(fileName, "Test Full Disk");
         writeTestData(w);
         QVERIFY_EXCEPTION_THROWN( w.Commit(), AudacityException );
         QVERIFY( !QFileInfo(fileName).exists() );
      }
   }
}

void testFileReader()
{
   // file doesn't exist
   {
      XMLFileReader reader;
      QVERIFY( reader.GetErrorStr() == "" );
      QVERIFY( ! reader.Parse(nullptr, "bogus/filename.xml") );
      QVERIFY( reader.GetErrorStr().contains("bogus/filename.xml") );
   }

   // parse error
   {
      XMLFileReader reader;
      QTemporaryFile f("testFileReader-parse-error.xml");
      QVERIFY( f.open() );
      f.write(QByteArray("<malformed>xml<malformed>"));
      f.close();

      QVERIFY( !reader.Parse(nullptr, f.fileName()) );
      QVERIFY( reader.GetErrorStr() != "" ); // localized string
   }

   // parse OK but handler rejected it
   {
      XMLFileReader reader;
      TestFailXMLTagHandler handler;

      QTemporaryFile f("testFileReader-handler.xml");
      QVERIFY( f.open() );
      f.write(QByteArray("<tag>foo</tag>\n"));
      f.close();

      QVERIFY( ! reader.Parse(&handler, f.fileName()) );
      qDebug("%s", qPrintable(reader.GetErrorStr()) );
      QVERIFY( reader.GetErrorStr().contains(f.fileName()) );
   }
}

private:
void writeTestData(XMLWriter& w)
{
   w.StartTag("document");
   w.StartTag("entity");
   w.WriteAttr("cstring", "astring");
   w.WriteAttr("qstring", QString("astring"));
   w.WriteAttr("qchar", QChar(0x01b1));
   w.WriteAttr("unprintable", QChar(0x007f));
   w.WriteAttr("surrogates", QString(QChar(0xd802)) + QChar(0xdc3f));

   w.WriteAttr("false", false);
   w.WriteAttr("true", true);
   w.WriteAttr("char", (char)'x');
   w.WriteAttr("uchar", (unsigned char)0xff);

   w.WriteAttr("short", (short)SHRT_MAX);
   w.WriteAttr("ushort", (unsigned short)USHRT_MAX);

   w.WriteAttr("int", (int)INT_MAX);
   // FIXME: shouldn't this work?
   //w.WriteAttr("uint", (unsigned int)UINT_MAX);

   w.WriteAttr("long", (long)LONG_MAX);
   w.WriteAttr("ulong", (unsigned long)ULONG_MAX);

   w.WriteAttr("longlong", (long long)LLONG_MAX);
   //w.WriteAttr("ulonglong", (unsigned long long)ULLONG_MAX);

   w.WriteAttr("size_t", (size_t)UINT64_MAX);
   w.WriteAttr("float1", (float)1/9);
   w.WriteAttr("float2", (float)1/9, 3);
   w.WriteAttr("float3", FLT_MAX);

   w.WriteAttr("double1", (double)1/9);
   w.WriteAttr("double2", (double)1/9, 3);
   w.WriteAttr("double3", DBL_MAX);

   //TESTME what is WriteData supposed to do?
   //w.WriteData("WriteData");

   w.WriteSubTree("WriteSubTree");

   w.EndTag("entity");
   w.EndTag("document");
}

void verifyTestData(const QString& fileName)
{
   XMLFileReader reader;

   if (!reader.Parse(this, fileName))
      qFatal("%s", qPrintable(reader.GetErrorStr()) );
}

void verifyTestAttrs(const QStringMap& attrs)
{
   QCOMPARE( attrs["cstring"], QString("astring") );
   QCOMPARE( attrs["qstring"], QString("astring") );
   QCOMPARE( attrs["qchar"],   QString(QChar(0x01b1)) );
   QCOMPARE( attrs["unprintable"], QString(QChar(0x007f)) );
   QCOMPARE( attrs["surrogates"], QString(QChar(0xd802)) + QChar(0xdc3f));

// QCOMPARE( (bool)(attrs["false"].toInt()), false );
#define CMP( key, func, type, value ) \
   QCOMPARE( (type)(attrs[key].func()), (type)value )

   CMP("false", toInt, bool, false);
   CMP("true", toInt, bool, true);
   CMP("char", toInt, char, 'x');
   CMP("uchar", toInt, unsigned char, 0xff);
   CMP("short", toShort, short, SHRT_MAX);
   CMP("ushort", toUShort, unsigned short, USHRT_MAX);
   CMP("int", toInt, int, INT_MAX);
   CMP("long", toLong, long, LONG_MAX);
   CMP("ulong", toULong, unsigned long, ULONG_MAX);
   CMP("longlong", toLongLong, long long, LLONG_MAX);
   CMP("size_t", toULongLong, size_t, UINT64_MAX);
   CMP("float1",  toFloat, float, 0.111111f);
   CMP("float2",  toFloat, float, 0.111f);
   CMP("float3",  toFloat, float, FLT_MAX);
   CMP("double1", toDouble, double, 0.111111);
   CMP("double2", toDouble, double, 0.111);
   CMP("double3", toDouble, double, DBL_MAX);

#undef CMP
}

bool HandleXMLTag(const QString& tag, const QStringMap& attrs) override
{
   //qDebug("tag: %s", qPrintable(tag));
   if (tag != "entity")
      return true;

   verifyTestAttrs(attrs);

   return true;
}

XMLTagHandler* HandleXMLChild(const QString& tag) override
{
   //qDebug("child: %s", qPrintable(tag));
   (void)tag;
   return this;
}

};

QTEST_MAIN(TestXMLWriter)
#include "TestXMLWriter.moc"
