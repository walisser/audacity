/**********************************************************************

  Audacity: A Digital Audio Editor

  Internat.h

  Markus Meyer
  Dominic Mazzoni (Mac OS X code)

**********************************************************************/

#ifndef __AUDACITY_INTERNAT__
#define __AUDACITY_INTERNAT__

//--#include <wx/arrstr.h>
//--#include <wx/string.h>
//--#include <wx/longlong.h>
#ifndef IN_RC

class QString;

extern const QString& GetCustomTranslation(const QString& str1 );
extern const QString& GetCustomSubstitution(const QString& str1 );

// Marks string for substitution only.
#define _TS( s ) GetCustomSubstitution( s )

// Marks strings for extraction only...must use wxGetTranslation() to translate.
#define XO(s)  wxT(s)

#if defined( __WXDEBUG__ )
   // Force a crash if you misuse _ in a static initializer, so that translation
   // is looked up too early and not found.

   #ifdef _MSC_VER

   #define _(s) ((wxTranslations::Get() || (DebugBreak(), true)), \
                GetCustomTranslation((s)))

   #else

   #include <signal.h>
   // Raise a signal because it's even too early to use wxASSERT for this.
   #define _(s) ((wxTranslations::Get() || raise(SIGTRAP)), \
                GetCustomTranslation((s)))

   #endif

#else
   #define _(s) GetCustomTranslation((s))
#endif

// TODO: translations
#undef _TS
#undef _
#define _TS(s) (s)
#define _(s)   QString(s)

// The two string arugments will go to the .pot file, as
// msgid sing
// msgid_plural plural
//
// (You must use plain string literals.  Do not use _() or wxT() or L prefix,
//  which (intentionally) will fail to compile.  The macro inserts wxT).
//
// Note too:  it seems an i18n-hint comment is not extracted if it precedes
// wxPLURAL directly.  A workaround:  after the comment, insert a line
// _("dummyStringXXXX");
// where for XXXX subsitute something making this dummy string unique in the
// program.  Then check in your generated audacity.pot that the dummy is
// immediately before the singular/plural entry.
//
// Your i18n-comment should therefore say something like,
// "In the string after this one, ..."
#define wxPLURAL(sing, plur, n)  wxGetTranslation( wxT(sing), wxT(plur), n)

#endif

class Internat
{
public:
   /** \brief Initialize internationalisation support. Call this once at
    * program start. */
   static void Init();

   /** \brief Get the decimal separator for the current locale.
    *
    * Normally, this is a decimal point ('.'), but e.g. Germany uses a
    * comma (',').*/
   static QChar GetDecimalSeparator();

   /** \brief Convert a string to a number.
    *
    * This function will accept BOTH point and comma as a decimal separator,
    * regardless of the current locale.
    * Returns 'true' on success, and 'false' if an error occurs. */
   static bool CompatibleToDouble(const QString& stringToConvert, double* result);

   // Function version of above.
   static double CompatibleToDouble(const QString& stringToConvert);

   /** \brief Convert a number to a string, always uses the dot as decimal
    * separator*/
   static QString ToString(double numberToConvert,
                     int digitsAfterDecimalPoint = -1);

   /** \brief Convert a number to a string, uses the user's locale's decimal
    * separator */
   static QString ToDisplayString(double numberToConvert,
                     int digitsAfterDecimalPoint = -1);

   /** \brief Convert a number to a string while formatting it in bytes, KB,
    * MB, GB */
   static QString FormatSize(int64_t size);
   static QString FormatSize(double size);

   /** \brief Protect against Unicode to multi-byte conversion failures
    * on Windows */
#if defined(__WXMSW__)
   static char *VerifyFilename(const QString &s, bool input = true);
#endif

   /** \brief Check a proposed file name string for illegal characters and
    * remove them
    * return true iff name is "visibly" changed (not necessarily equivalent to
    * character-wise changed)
    */
   static bool SanitiseFilename(QString &name, const QString &sub);

   /** \brief Remove accelerator charactors from strings
    *
    * Utility function - takes a translatable string to be used as a menu item,
    * for example _("&Splash...\tAlt+S"), and strips all of the menu
    * accelerator stuff from it, to make "Splash".  That way the same
    * translatable string can be used both when accelerators are needed and
    * when they aren't, saving translators effort. */
   static QString StripAccelerators(const QString& str);

   static const QStringList &GetExcludedCharacters()
   { return exclude; }

private:
   static QChar mDecimalSeparator;

   // stuff for file name sanitisation
   static QStringList exclude;

   static QByteArray mFilename;
};

#define _NoAcc(X) Internat::StripAccelerators(_(X))

// Use this macro to wrap all filenames and pathnames that get
// passed directly to a system call, like opening a file, creating
// a directory, checking to see that a file exists, etc...
#if defined(__WXMSW__)
// Note, on Windows we don't define an OSFILENAME() to prevent accidental use.
// See VerifyFilename() for an explanation.
#define OSINPUT(X) Internat::VerifyFilename(X, true)
#define OSOUTPUT(X) Internat::VerifyFilename(X, false)
#elif defined(__WXMAC__)
#define OSFILENAME(X) ((char *) (const char *)(X).fn_str())
#define OSINPUT(X) OSFILENAME(X)
#define OSOUTPUT(X) OSFILENAME(X)
#else
#define OSFILENAME(X) ((char *) (const char *)(X).mb_str())
#define OSINPUT(X) OSFILENAME(X)
#define OSOUTPUT(X) OSFILENAME(X)
#endif

// Convert C strings to wxString
#define UTF8CTOWX(X) wxString((X), wxConvUTF8)
#define LAT1CTOWX(X) wxString((X), wxConvISO8859_1)

#endif
