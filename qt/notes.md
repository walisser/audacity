

TODO
----
- Test ODPCMAliasBlockFile threading
- XMLFileWriter isn't exactly the same as current implementation
   - it doesn't recover the partial file if there is any failure like low disk

- put wxT() back in that were taken out in the beginning.
- submit PR for ODDecodeFlacTask incorrect usage of memcpy() .. EXPERIMENTAL_OD_FLAC ..
- submit PR for ODPCMAliasBlockFile Read255/Read64k bug
- submit PR for ODDecodeBlockFile::BuildFromXML aliasChannel typo
- submit PR ODDecodeBlockFile Read256/Read64K doesn't clear the full buffer
- submit PR for GrowableSampleBuffer format change allocation bug
- submit PR for SilentBlockFile ReadData return > Length() (inconsistent)
- submit PR XMLReader will not fail if a child tag handler returns false,
  only if the TOP handler returns false. Perhaps it doesn't matter.

Goals
--------
- QML/Qt-based UI for iOS and Android (phone, tablet) built on the Audacity core
- 90% test coverage or better

- Port core classes with at least
   - File system (block files, xml)
   - Project model (Track, Clip, Sequence, etc)
   - Core edit operations
   - Mix and Render
   - Undo
   - Import / Export

- Add or complete Android/iOS support in portaudio
   - Latency detection for play-through support?
   - External DACs need to work, built-in audio probably not great for real work

Principles & Practices
-----------------------
- The core has no GUI requirement
   - will need to have a minimal user-interaction abstraction at some stage?
- Parallel development track
   - merge upstream changes after milestones
   - submit PR for bugs found in unit tests
   - avoid refactoring large portions if possible
- Skip dead code, deprecations and (usually) legacy code
- Remove all #includes and add back what is actually needed
- Remove #include "Audacity.h", goes in the precompiled header now
- Make classes re-entrant when trivial

Noteable Changes
---------------------
- Replaces storage of wxFileName/wxFileNameWrapper with QString
- Removes static/global summary buffer from BlockFile
- Fixes numerous bugs in ODDecodeFlacTask, but still broken and ugly
- Adds DialogManager to test code with multi-choice dialogs

Mac OS X Issues
---------------

Android Issues
--------------
- qmakespec for android/g++ needs [PCH support added](https://codereview.qt-project.org/#/c/216082/)
- qdevice.pri changed to api level 17 for malloc_usable_size

Annotations
-----------
- PORTME - needs qt version
- TESTME - needs test; or cannot test due to PORTME issue
- FIXME  - found a bug; bug needs to be fixed
- TODO   - probably should do this; not a bug but a sore point
- //--   - commented out code for old implementation
- /**-- --**//

Translation
------------
- _(x) -> QString(x)... deactivated for now
- %s,%d,etc -> %1, %2, %3 etc and use QString::arg()
- wxT() -> ()


Type Conversions
----------------
- wxArrayString -> QStringList
- wxString -> QString
- wxChar* -> QString
- wxChar** -> QStringList / QStringMap
   - QStringMap for XML attribute/value pairs
- wxFileName -> QString
- wxFileNameWrapper -> QString
- wxChar -> QChar
- wxUint32 -> uint32_t
- wxLongLong -> int64_t
   - use stdint types where apropriate
- wxCharBuffer -> ??


Logging,messages
----------------
- wxLogDebug():
   - qWarning(), if recoverable error
- wxPrintf(): qInfo()
- wxLogMessage(): qInfo()
- wxLogWarning(): qWarning()
- wxLogSysErr(): qErrnoWarning() => sends to qCritical()
- wxMessageBox -> qCritical()
   - make Audacity core free of any gui requirement
- ShowWarningDialog -> qCritical()
- AudacityMessageBox -> qCritical()


wxFileName
----------
- wxFileName(path)::GetFullName => QFileInfo(path)::fileName()
- wxFileName(path)::GetName => QFileInfo(path)::baseName()
- wxFileName(path)::GetFullPath => path


Misc
--------------
- wxASSERT() -> Q_ASSERT or Q_ASSUME or Q_UNREACHABLE
- wxASSERT_MSG -> Q_ASSERT_X or Q_ASSUME_X
   Q_ASSUME iff code that follows would be incorrect otherwise??
   Q_UNREACHABLE if the assert is to guard unreachable code
- WXUNSUED(x) -> (void)x
- wxUINT32_SWAP_ALWAYS -> qbswap
- TRUE -> true
- FALSE -> false
- int -> bool


Android Deployment Speeds
-------------------------
* * deployment speeds (tablet)
- "debug":   10 - 14s
- "bundled": 12 - 18s
- "ministro":10 - 12s

* * deployment speeds (x86 emulator)
- "debug": 6-11
- "bundled":  6 - 11s
- "ministro": 6 - 8s


Qt Notes
----------------
- QSaveFile::commit() will create an empty file if disk is full,
  and will not report an error. QSaveFile::flush() will correctly return false
- QFile::copy() seems to lock up if disk is full on Linux


Qt codereview
----------------
<ddubya> is there some quick setup guide for pushing a patch? I can't even find what repo I need to fork
<svuorela> ddubya: do you have an account ?
<ddubya> I just setup qt account and I'm logged into codereview
<sergio> ddubya: git clone ssh://<your_user>@codereview.qt-project.org:29418/qt/qtbase
<ddubya> k cool
<sergio> checkout dev branch
<svuorela> I have http://paste.debian.net/1004090/ in .gitconfig and just does git clone qt:qtbase  and then I can just git push origin HEAD:refs/for/dev
 for variaus values of qtbase and svuorela
--- threebar is now known as |||
<ddubya> so refs/for/dev is where I need to push? I don't see any documentation for that
<sergio> HEAD:refs/for/dev
<ddubya> so the first line of the commit message becomes the subject right? are there any conventions here? The patch is for mkspec
<sergio> just say what it does: android-gcc: Add support for pre-compiled headers
 also see http://wiki.qt.io/Qt_Contribution_Guidelines
 usually also git log on the files you touched, to find potential reviewers
 you should add a Task-number: QTBUG-xyz to link it to the bug


