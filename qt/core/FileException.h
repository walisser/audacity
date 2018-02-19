//
//  FileException.h
//  
//
//  Created by Paul Licameli on 11/22/16.
//
//

#ifndef __AUDACITY_FILE_EXCEPTION__
#define __AUDACITY_FILE_EXCEPTION__

#include "AudacityException.h"

class FileException /* not final */ : public MessageBoxException
{
public:
   enum class Cause { Open, Read, Write, Rename };

   explicit FileException
      ( Cause cause_, const QString &fileName_,
        const QString &caption = _("File Error"),
        const QString &renameTarget_ = {})
   : MessageBoxException{ caption }
   , cause{ cause_ }, fileName{ fileName_ }, renameTarget{ renameTarget_ }
   {}

   FileException(FileException&& that)
      : MessageBoxException(std::move(that))
      , cause{ that.cause }
      , fileName{ that.fileName }
      , renameTarget{ that.renameTarget }
   {}

   FileException& operator= (FileException&&) PROHIBITED;

   ~FileException() override;

protected:
   std::unique_ptr< AudacityException > Move() override;

   // Format a default, internationalized error message for this exception.
   QString ErrorMessage() const override;

public:
   Cause cause;
   QString fileName;
   QString renameTarget;
};

#endif
