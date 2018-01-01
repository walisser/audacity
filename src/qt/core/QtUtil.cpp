
#include "QtUtil.h"

void qDelayedCall(std::function< void(void) > f)
{
   QObject* obj = new QObject;   
   QObject::connect(obj, &QObject::destroyed, qApp, f);
   obj->moveToThread(qApp->thread());
   obj->deleteLater();
}
