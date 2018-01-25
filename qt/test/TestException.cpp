#include <QtTest>

#include "core/FileException.h"

class TestException : public QObject
{
Q_OBJECT
private Q_SLOTS:
//void initTestCase() {}
//void cleanupTestCase() {}

void testDelayedCallFromMainThread()
{
   int argc=1;
   char* argv[] = { strdup("app") };
   QCoreApplication app(argc, argv);

   bool didInvoke=false;

   qDelayedCall([&didInvoke]() {
      qDebug("delayed call from main thread");
      QVERIFY(QThread::currentThread() == qApp->thread());
      didInvoke=true;
      qApp->exit(0);
   });

   QVERIFY(!didInvoke);
   app.exec();
   QVERIFY(didInvoke);
}

void testDelayedCallFromWorkerThread()
{
   int argc=1;
   char* argv[] = { strdup("app") };

   volatile bool didInvoke = false;

   QCoreApplication app(argc, argv);

   QtConcurrent::run([&didInvoke]() {

      QVERIFY(QThread::currentThread() != qApp->thread());
      qDelayedCall([&didInvoke]() {
         qDebug("delayed call from worker thread");
         QVERIFY(QThread::currentThread() == qApp->thread());
         didInvoke = true;
         qApp->exit(0);
      });
   });

   // delayed calls can't fire until event loop runs
   QVERIFY(!didInvoke);
   app.exec();
   QVERIFY(didInvoke);
}

void testMainThreadException()
{
   QString msg = "Ruh-Roh Raggie";
   testGuardedCall(false, msg, [=]() {
      GuardedCall<void>([=] {
         throw std::move(SimpleMessageBoxException(msg, "Main Thread Exception"));
      });
      qDelayedCall([]() { qApp->exit(0); });
   });
}

void testWorkerThreadException()
{
   QString msg = "Ruh-Roh Raggie";
   testGuardedCall(true, msg, [=]() {
      GuardedCall<void>([=] {
         throw GetException(msg);
      });
      qDelayedCall([]() { qApp->exit(0); });
   });
}

void testFileException()
{
   // test the path gets truncated to 3 dirs
   QString filePath    = "/path/to/some/directory/foo.xyz";
   QString checkString = "/path/to/some/.../foo.xyz";
   testGuardedCall(false, checkString, [=]() {
      GuardedCall<void>([=] {
         throw FileException(FileException::Cause::Open, filePath);
      });
      qDelayedCall([]() { qApp->exit(0); });
   });

   // check all causes produce an error message with file name
   // use a switch block to get warning when new stuff comes up
   for (int cause = 0; cause <= (int)FileException::Cause::Rename; cause++)
      switch ((FileException::Cause)cause)
      {
         case FileException::Cause::Open:
         case FileException::Cause::Read:
         case FileException::Cause::Write:
         case FileException::Cause::Rename:

         QString filePath = "/path/to/some/foo.xyz";
         QString checkString = filePath;
         testGuardedCall(false, checkString, [=]() {
            GuardedCall<void>([=] {
               throw FileException((FileException::Cause)cause, filePath);
            });
            qDelayedCall([]() { qApp->exit(0); });
         });
      }
}

private:

// call a function that throws an AudacityException,
// if fromThread, call from a new thread
// look for checkString in the resulting log message
void testGuardedCall(bool fromThread, const QString& checkString,
                     std::function<void(void)> function)
{
   int argc=1;
   char* argv[] = { strdup("app") };

   QCoreApplication app(argc, argv);

   // Install our own message handler, replacing the one
   // of QTest framework, so we can see if the exception
   // successfully logged a message.
   _loggedMessages.clear();
   _defaultMessageHandler = qInstallMessageHandler(messageHandler);

   // MessageBoxException should discard all but the last exception -
   // *between idle times* of the event loop.
   QString message = "This exception shall not be presented to the user";
   GuardedCall<void>([=] {
      throw SimpleMessageBoxException(message, "Suppressed Exception");
   });

   if (fromThread)
   {
      auto future = QtConcurrent::run(function);

      // wait for thread to finish; to ensure first exception is ignored
      future.waitForFinished();
   }
   else
      function();

   // nothing logged yet, event loop hasn't run yet
   QVERIFY(_loggedMessages.count() == 0);

   // enter event loop
   app.exec();

   // only one exception logged, it should contain our message
   QVERIFY(_loggedMessages.count() == 1);
   QVERIFY(_loggedMessages[0].contains(checkString));

   (void)qInstallMessageHandler(_defaultMessageHandler);
}


static QtMessageHandler _defaultMessageHandler;
static QStringList _loggedMessages;
static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
   if (type == QtCriticalMsg)
      _loggedMessages.append(msg);

   _defaultMessageHandler(type, context, msg);
}

static SimpleMessageBoxException GetException(const QString& msg)
{
   return SimpleMessageBoxException(msg, "Worker Thread Exception");
}

};

QtMessageHandler TestException::_defaultMessageHandler;
QStringList      TestException::_loggedMessages;

QTEST_MAIN(TestException)
#include "TestException.moc"
