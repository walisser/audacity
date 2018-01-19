
* TODO
- Test ODPCMAliasBlockFile threading

- put wxT() back in that were taken out, probably can't hurt...
- submit PR for ODPCMAliasBlockFile Read255/Read64k bug
- submit PR for GrowableSampleBuffer bug
- submit PR for SilentBlockFile ReadData return > Length()
-

* android
==========
qmakespec for android/g++ needs PCH support added: https://codereview.qt-project.org/#/c/216082/
qdevice.pri changed to api level 17 for malloc_usable_size

* annotations
-----------
- PORTME - needs qt version
- TESTME - needs test; or cannot test due to PORTME issue
- FIXME  - found a bug; bug needs to be fixed
- TODO   - probably should do this; not a bug but a sore point
- //--   - commented out code for old implementation
- /**-- --**//

* translation
-------------
_() -> deactivated for now
%s -> %1, %2, %3 etc and use QString::arg()
wxT() -> ()


* type conversions
----------------
wxArrayString -> QStringList
wxString -> QString
wxChar* -> QString
wxChar** -> QStringList / QStringMap
   - QStringMap for XML attribute/value pairs
wxFileName -> QString
wxFileNameWrapper -> QString
wxChar -> QChar
wxUint32 -> uint32_t
wxLongLong -> int64_t
   - use stdint types where apropriate
wxCharBuffer -> ??
wxLogDebug():
   - qWarning(), if recoverable error
wxFileName(path)::GetFullName => QFileInfo(path)::fileName()
wxFileName(path)::GetFullPath => path
wxASSERT() -> Q_ASSERT or Q_ASSUME or Q_UNREACHABLE
wxASSERT_MSG -> Q_ASSERT_X or Q_ASSUME_X
   Q_ASSUME iff code that follows would be incorrect otherwise??
   Q_UNREACHABLE if the assert is to guard unreachable code
wxUINT32_SWAP_ALWAYS -> qbswap
wxMessageBox -> qCritical()
   - make Audacity core free of any gui requirement

* misc
--------------
TRUE -> true
FALSE -> false
int -> bool


* android
---------------
* * deployment speeds (tablet)
- "debug":   10 - 14s
- "bundled": 12 - 18s
- "ministro":10 - 12s

* * deployment speeds (x86 emulator)
- "debug": 6-11
- "bundled":  6 - 11s
- "ministro": 6 - 8s


* Qt bugs
----------------
QSaveFile::commit() will create an empty file if disk is full,
and will not report an error. QSaveFile::flush() will correctly return false

* Qt codereview..
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


