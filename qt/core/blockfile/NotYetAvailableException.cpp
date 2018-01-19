//
//  NotYetAvailableException.cpp
//  
//
//  Created by Paul Licameli on 12/25/16.
//
//

//#include "../Audacity.h"
#include "NotYetAvailableException.h"

NotYetAvailableException::~NotYetAvailableException()
{
}

std::unique_ptr< AudacityException > NotYetAvailableException::Move()
{
   return std::unique_ptr< AudacityException >
   { safenew NotYetAvailableException{ std::move( *this ) } };
}

QString NotYetAvailableException::ErrorMessage() const
{
   return QString(
      _("This operation cannot be done until importation of %1 completes."))
      .arg(fileName);
}
