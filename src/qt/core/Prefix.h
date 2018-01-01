#include <QtCore/QtCore>
#include <QtConcurrent/QtConcurrent>

#include "core/QtUtil.h"

typedef QMap<QString,QString> QStringMap;

// FIXME: configuration test
#define HAVE_GNU_MALLOC 1
#define HAVE_MALLOC_H 1

