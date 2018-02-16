#include <QtCore/QtCore>
#include <QtConcurrent/QtConcurrent>

// uncomment to find Q_UNUSED, replace with (void)x
// however must keep it for moc
//#undef Q_UNUSED

#include "core/QtUtil.h"

typedef QMap<QString,QString> QStringMap;

// always-enabled assertions
#define Q_REQUIRE(cond) ((cond) ? static_cast<void>(0) : qt_assert(#cond, __FILE__, __LINE__))
#define Q_REQUIRE_X(cond, where, what) ((cond) ? static_cast<void>(0) : qt_assert_x(where, what, __FILE__, __LINE__))

// wxT() indicates a string that we do not want to translate
// can be changed to something qt-ish like notr() 
#define wxT(s) (s)

// include global macros and configuration in everything
#include "core/Audacity.h"
