
// return true if dir is empty
// use this instead of QDir::isEmpty(), added in Qt 5.9
bool qDirIsEmpty(const QDir& dir);

// similar to QFile::copy() but without the bugs
bool qCopyFile(const QString& srcPath, const QString& dstPath);

// run function on the main event loop
void qDelayedCall(std::function< void(void) > f);
