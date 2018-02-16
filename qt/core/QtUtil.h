
// similar to QFile::copy() but without the bugs
bool qCopyFile(const QString& srcPath, const QString& dstPath);

// run function on the main event loop
void qDelayedCall(std::function< void(void) > f);
