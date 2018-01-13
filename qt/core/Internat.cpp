/**********************************************************************

  Audacity: A Digital Audio Editor

  Internat.cpp

  Markus Meyer
  Dominic Mazzoni (Mac OS X code)

*******************************************************************//*!

\class Internat
\brief Internationalisation support.

This class is used to help internationalisation and in general
compatibility with different locales and character sets.
It deals mostly with converting numbers, but also has important
functions to convert to/from UTF-8, which is used in XML files
and on Mac OS X for the filesystem.

*//*******************************************************************/

//#include <wx/msgdlg.h>
//#include <wx/log.h>
//#include <wx/intl.h>
//#include <wx/filename.h>

#include <locale.h>
#include <math.h> // for pow()

#include "Internat.h"
//#include "Experimental.h"
//#include "FileNames.h"

// in order for the static member variables to exist, they must appear here
// (_outside_) the class definition, in order to be allocated some storage.
// Otherwise, you get link errors.

//wxChar Internat::mDecimalSeparator = wxT('.'); // default
//wxArrayString Internat::exclude;
//wxCharBuffer Internat::mFilename;

// DA: Use tweaked translation mechanism to replace 'Audacity' by 'DarkAudacity'.
#ifdef EXPERIMENTAL_DA
// This function allows us to replace Audacity by DarkAudacity without peppering 
// the source code with changes.  We split out this step, the customisation, as 
// it is used on its own (without translation) in the wxTS macro.
const QString& GetCustomSubstitution(const QString& str2)
{
   // If contains 'DarkAudacity, already converted.
   if( str2.Contains( "DarkAudacity" ))
      return str2;
   // If does not contain 'Audacity', nothing to do.
   if( !str2.Contains( "Audacity" ))
      return str2;
   QString str3 = str2;
   str3.Replace( "Audacity", "DarkAudacity" );
   str3.Replace( " an DarkAudacity", " a DarkAudacity" );
   // DA also renames sync-lock(ed) as time-lock(ed).
   str3.Replace( "Sync-Lock", "Time-Lock" );
   str3.Replace( "Sync-&Lock", "Time-&Lock" );
   str3.Replace( "Sync Lock", "Time Lock" );
   return wxTranslations::GetUntranslatedString(str3);
}
#else 
const QString& GetCustomSubstitution(const QString& str1)
{
   return str1 ;
}
#endif

/**PORTME
// In any translated string, we can replace the name 'Audacity' by 'DarkAudacity'
// without requiring translators to see extra strings for the two versions.
const QString& GetCustomTranslation(const QString& str1)
{
   const QString& str2 = wxGetTranslation( str1 );
   return GetCustomSubstitution( str2 );
}


void Internat::Init()
{
   // Save decimal point character
   struct lconv * localeInfo = localeconv();
   if (localeInfo)
      mDecimalSeparator = QString(wxSafeConvertMB2WX(localeInfo->decimal_point)).GetChar(0);

//   wxLogDebug(wxT("Decimal separator set to '%c'"), mDecimalSeparator);

   // Setup list of characters that aren't allowed in file names
   // Hey!  The default wxPATH_NATIVE does not do as it should.
#if defined(__WXMAC__)
   wxPathFormat format = wxPATH_MAC;
#elif defined(__WXGTK__)
   wxPathFormat format = wxPATH_UNIX;
#elif defined(__WXMSW__)
   wxPathFormat format = wxPATH_WIN;
#endif

   // This is supposed to return characters not permitted in paths to files
   // or to directories
   auto forbid = wxFileName::GetForbiddenChars(format);

   for(auto cc: forbid)
      exclude.Add(QString{ cc });

   // The path separators may not be forbidden, so add them
   auto separators = wxFileName::GetPathSeparators(format);

   for(auto cc: separators) {
      if (forbid.Find(cc) == wxNOT_FOUND)
         exclude.Add(QString{ cc });
   }
}

wxChar Internat::GetDecimalSeparator()
{
   return mDecimalSeparator;
}

bool Internat::CompatibleToDouble(const QString& stringToConvert, double* result)
{
   // Regardless of the locale, always respect comma _and_ point
   QString s = stringToConvert;
   s.Replace(wxT(","), QString(GetDecimalSeparator()));
   s.Replace(wxT("."), QString(GetDecimalSeparator()));
   return s.ToDouble(result);
}

double Internat::CompatibleToDouble(const QString& stringToConvert)
{
   double result = 0;
   Internat::CompatibleToDouble(stringToConvert, &result);
   return result;
}
PORTME**/

QString Internat::ToString(double numberToConvert,
                     int digitsAfterDecimalPoint /* = -1 */)
{
   // return number in [-]xxx.yyy format, non-localized
   return QString::number(numberToConvert, 'f', digitsAfterDecimalPoint);
   //--wxString result = ToDisplayString(
   //--   numberToConvert, digitsAfterDecimalPoint);

   //--result.Replace(QString(GetDecimalSeparator()), wxT("."));

   //--return result;
}

/**PORTME
QString Internat::ToDisplayString(double numberToConvert,
                                    int digitsAfterDecimalPoint)
{
   QString decSep = QString(GetDecimalSeparator());
   QString result;

   if (digitsAfterDecimalPoint == -1)
   {
      result.Printf(wxT("%f"), numberToConvert);

      // Not all libcs respect the decimal separator, so always convert
      // any dots found to the decimal separator.
      result.Replace(wxT("."), decSep);

      if (result.Find(decSep) != -1)
      {
         // Strip trailing zeros, but leave one, and decimal separator.
         int pos = result.Length() - 1;
         while ((pos > 1) &&
                  (result.GetChar(pos) == wxT('0')) &&
                  (result.GetChar(pos - 1) != decSep))
            pos--;
         // (Previous code removed all of them and decimal separator.)
         //    if (result.GetChar(pos) == decSep)
         //       pos--; // strip point before empty fractional part
         result = result.Left(pos+1);
      }
   }
   else
   {
      QString format;
      format.Printf(wxT("%%.%if"), digitsAfterDecimalPoint);
      result.Printf(format, numberToConvert);

      // Not all libcs respect the decimal separator, so always convert
      // any dots found to the decimal separator
      result.Replace(wxT("."), decSep);
   }

   return result;
}

QString Internat::FormatSize(wxLongLong size)
{
   // wxLongLong contains no built-in conversion to double
   double dSize = size.GetHi() * pow(2.0, 32);  // 2 ^ 32
   dSize += size.GetLo();

   return FormatSize(dSize);
}

QString Internat::FormatSize(double size)
{
   QString sizeStr;

   if (size == -1)
      sizeStr = _("Unable to determine");
   else {
      // make it look nice, by formatting into k, MB, etc
      if (size < 1024.0)
         sizeStr = ToDisplayString(size) + wxT(" ") + _("bytes");
      else if (size < 1024.0 * 1024.0) {
         // i18n-hint: Abbreviation for Kilo bytes
         sizeStr = ToDisplayString(size / 1024.0, 1) + wxT(" ") + _("KB");
      }
      else if (size < 1024.0 * 1024.0 * 1024.0) {
         // i18n-hint: Abbreviation for Mega bytes
         sizeStr = ToDisplayString(size / (1024.0 * 1024.0), 1) + wxT(" ") + _("MB");
      }
      else {
         // i18n-hint: Abbreviation for Giga bytes
         sizeStr = ToDisplayString(size / (1024.0 * 1024.0 * 1024.0), 1) + wxT(" ") + _("GB");
      }
   }

   return sizeStr;
}

#if defined(__WXMSW__)
//
// On Windows, QString::mb_str() can return a NULL pointer if the
// conversion to multi-byte fails.  So, based on direction intent,
// returns a pointer to an empty string or prompts for a NEW name.
//
char *Internat::VerifyFilename(const QString &s, bool input)
{
   static wxCharBuffer buf;
   QString name = s;

   if (input) {
      if ((char *) (const char *)name.mb_str() == NULL) {
         name = wxEmptyString;
      }
   }
   else {
      wxFileName ff(name);
      QString ext;
      while ((char *) (const char *)name.mb_str() == NULL) {
         wxMessageBox(_("The specified filename could not be converted due to Unicode character use."));

         ext = ff.GetExt();
         name = FileNames::SelectFile(FileNames::Operation::_None,
                             _("Specify New Filename:"),
                             wxEmptyString,
                             name,
                             ext,
                             wxT("*.") + ext,
                             wxFD_SAVE | wxRESIZE_BORDER,
                             wxGetTopLevelParent(NULL));
      }
   }

   mFilename = name.mb_str();

   return (char *) (const char *) mFilename;
}
#endif

bool Internat::SanitiseFilename(QString &name, const QString &sub)
{
   bool result = false;
   for(const auto &item : exclude)
   {
      if(name.Contains(item))
      {
         name.Replace(item, sub);
         result = true;
      }
   }

#ifdef __WXMAC__
   // Special Mac stuff
   // '/' is permitted in file names as seen in dialogs, even though it is
   // the path separator.  The "real" filename as seen in the terminal has ':'.
   // Do NOT return true if this is the only subsitution.
   name.Replace(wxT("/"), wxT(":"));
#endif

   return result;
}

QString Internat::StripAccelerators(const QString &s)
{
   QString result;
   result.Alloc(s.Length());
   for(size_t i = 0; i < s.Length(); i++) {
      if (s[i] == '\t')
         break;
      if (s[i] != '&' && s[i] != '.')
         result += s[i];
   }
   return result;
}

PORTME**/
