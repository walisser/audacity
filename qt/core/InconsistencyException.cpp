//
//  InconsistencyException.cpp
//  
//
//  Created by Paul Licameli on 11/27/16.
//
//

#include "InconsistencyException.h"
#include "Internat.h"

InconsistencyException::~InconsistencyException()
{
}

std::unique_ptr< AudacityException > InconsistencyException::Move()
{
   return std::unique_ptr< AudacityException >
   { safenew InconsistencyException{ std::move( *this ) } };
}

QString InconsistencyException::ErrorMessage() const
{
   // Shorten the path
   QString path { file };
   auto sub = QString{ PLATFORM_PATH_SEPARATOR } + "src" + PLATFORM_PATH_SEPARATOR;
   auto index = path.indexOf(sub);
   if (index >= 0)
      path = path.mid(index + sub.length());

#ifdef Q_FUNC_INFO
   return _("Internal error in %1 at %2 line %3.\nPlease inform the Audacity team at https://forum.audacityteam.org/.")
      .arg(Q_FUNC_INFO).arg(path).arg(line);
#else
   return _("Internal error at %1 line %2.\nPlease inform the Audacity team at https://forum.audacityteam.org/.")
      .arg(path).arg(line);
#endif
}
