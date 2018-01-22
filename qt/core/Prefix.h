#include <QtCore/QtCore>
#include <QtConcurrent/QtConcurrent>

#include "core/QtUtil.h"

typedef QMap<QString,QString> QStringMap;

// always-enabled assertions
#define Q_REQUIRE(cond) ((cond) ? static_cast<void>(0) : qt_assert(#cond, __FILE__, __LINE__))
#define Q_REQUIRE_X(cond, where, what) ((cond) ? static_cast<void>(0) : qt_assert_x(where, what, __FILE__, __LINE__))

// QString is always included, so must be wxT()
#define wxT(s) (s)
