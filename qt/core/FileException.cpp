//
//  FileException.cpp
//  
//
//  Created by Paul Licameli on 11/22/16.
//
//

//--#include "Audacity.h"
#include "FileException.h"
//--#include "Internat.h"
//--#include "Prefs.h"

FileException::~FileException()
{
}

std::unique_ptr< AudacityException > FileException::Move()
{
   return std::unique_ptr< AudacityException >
      { safenew FileException{ std::move( *this ) } };
}

QString FileException::ErrorMessage() const
{
   QString format;
   switch (cause) {
      case Cause::Open:
         format = _("Audacity failed to open a file in %1.");
         break;
      case Cause::Read:
         format = _("Audacity failed to read from a file in %1.");
         break;
      case Cause::Write: {
         //--auto lang = gPrefs->Read("/Locale/Language", "");
            format =
_("Audacity failed to write to a file.\n"
  "Perhaps %1 is not writable or the disk is full.");
         break;
      }
      case Cause::Rename:
         format =
_("Audacity successfully wrote a file in %1 but failed to rename it as %2.");
      default:
         break;
   }
   QString target;

#ifdef __WXMSW__

   // Drive letter plus colon
   target = fileName.GetVolume() + wxT(":");

#else

   // Shorten the path, arbitrarily to 3 components
   auto path = fileName;
   //--path.SetFullName(wxString{});
   //--while(path.GetDirCount() > 3)
   //--   path.RemoveLastDir();
   //--target = path.GetFullPath();
   auto parts = path.split("/");
   if (parts.count() > 5)
   {
      auto name = parts.last();
      do {
         parts.removeLast();
      } while (parts.count() > 4);
      parts.append("..."); // indicate the removal
      parts.append(name);
   }
   target = parts.join("/");
#endif

   //--return wxString::Format(
   //--   format, target, renameTarget.GetFullName() );
   format = format.arg(target);
   if (format.contains("%2"))
      format = format.arg(renameTarget);

   return format;
}

