#include <QtTest>

#include "core/MyClass.h"

class TestMyClass : public QObject
{
   Q_OBJECT
private Q_SLOTS:
//void initTestCase() {}
//void cleanupTestCase() {}

void testSomeMethod()
{   
   QVERIFY(MyClass::someMethodReturnsTrue());
}

};

QTEST_MAIN(TestMyClass)
#include "TestMyClass.moc"
