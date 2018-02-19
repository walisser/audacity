//
//  NotYetAvailableException.h
//  
//
//  Created by Paul Licameli on 12/25/16.
//
//

#ifndef __AUDACITY_NOT_YET_AVAILABLE_EXCEPTION__
#define __AUDACITY_NOT_YET_AVAILABLE_EXCEPTION__

#include "../FileException.h"

// This exception can be thrown when attempting read of on-demand block files
// that have not yet completed loading.
class NotYetAvailableException final : public FileException
{
public:
   NotYetAvailableException( const QString &fileName )
      : FileException{ Cause::Read, fileName } {}
   NotYetAvailableException(NotYetAvailableException &&that)
      : FileException( std::move( that ) ) {}
   NotYetAvailableException& operator= (NotYetAvailableException&&) PROHIBITED;
   ~NotYetAvailableException();

protected:
   std::unique_ptr< AudacityException > Move() override;
   QString ErrorMessage() const override;
};

#endif
