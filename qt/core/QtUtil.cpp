
#include "QtUtil.h"

bool qDirIsEmpty(const QDir& dir)
{
#if QT_VERSION >= QT_VERSION_CHECK(5,9,0)
   return dir.isEmpty();
#else
   QDirIterator(dir.absolutePath(), QStringList{wxT("*")}, QDir::AllEntries|QDir::NoDotAndDotDot).hasNext();
#endif
}

bool qCopyFile(const QString& srcPath, const QString& dstPath)
{
   // copied from QFile:copy() with some modifications, namely to remove any
   // platform-optimized version of the copy, which presumeably caused the problem.
   //
   // QFile::copy() on Linux will livelock if disk space runs out (using RLIMIT_FSIZE)

   if (srcPath.isEmpty() || dstPath.isEmpty()) {
      qWarning(wxT("qCopyFile: Empty or null file name"));
      return false;
   }
   if (QFile::exists(dstPath)) {
      // ### Race condition. If a file is moved in after this, it /will/ be
      // overwritten. On Unix, the proper solution is to use hardlinks:
      // return ::link(old, new) && ::remove(old); See also rename().
      qWarning(wxT("qCopyFile: Destination exists"));
      return false;
   }

   QFile src(srcPath);

   bool error = false;
   if(!src.open(QFile::ReadOnly)) {
      qErrnoWarning(wxT("qCopyFile: Cannot open source file for reading"));
      error = true;
   } else {
      // copy to a temporary file, then rename
      QString fileTemplate = QLatin1String("%1/qt_temp.XXXXXX");
#ifdef QT_NO_TEMPORARYFILE
      QFile dst(fileTemplate.arg(QFileInfo(dstPath).path()));
      if (!dst.open(QIODevice::ReadWrite))
         error = true;
#else
      QTemporaryFile dst(fileTemplate.arg(QFileInfo(dstPath).path()));
      if (!dst.open()) {
         dst.setFileTemplate(fileTemplate.arg(QDir::tempPath()));
         if (!dst.open())
            error = true;
      }
#endif
      if (error) {
         qErrnoWarning(wxT("qCopyFile: Cannot open for destination file for writing"));
         dst.close();
         src.close();
      } else {

         char block[4096];
         qint64 totalRead = 0;
         while (!src.atEnd()) {
            qint64 in = src.read(block, sizeof(block));
            if (in <= 0)
               break;
            totalRead += in;
            if (in != dst.write(block, in)) {
               qErrnoWarning(wxT("qCopyFile: Failure to write block"));
               src.close();
               error = true;
               break;
            }
         }

         if (totalRead != src.size()) {
            // Unable to read from the source. The error string is
            // already set from read().
            error = true;
         }

         // NOTE: Qt 5.10.0 and possibly earlier have a bug
         // with file renaming on Android: https://bugreports.qt.io/browse/QTBUG-64103
         // fixed in 5.10.1
         if (!error && !dst.rename(dstPath)) {
            qErrnoWarning(wxT("qCopyFile: Cannot rename destination file"));
            error = true;
            src.close();
         }
#ifdef QT_NO_TEMPORARYFILE
         if (error)
            dst.remove();
#else
         // copy was successful, prevent temporary from being deleted
         if (!error)
            dst.setAutoRemove(false);
#endif
      }
   }
   if (!error) {
      QFile::setPermissions(dstPath, src.permissions());
      return true;
   }

   return false;
}

void qDelayedCall(std::function< void(void) > f)
{
   QObject* obj = new QObject;   
   QObject::connect(obj, &QObject::destroyed, qApp, f);
   obj->moveToThread(qApp->thread());
   obj->deleteLater();
}
