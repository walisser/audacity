/**********************************************************************

  Audacity: A Digital Audio Editor

  AudacityException

  Paul Licameli

********************************************************************//**

\class AudacityException
\brief root of a hierarchy of classes that are thrown and caught
 by Audacity.

\class MessageBoxException
\brief an AudacityException that pops up a single message box even if
there were multiple exceptions of the same kind before it actually 
got to show.

*//********************************************************************/

//--#include "Audacity.h"
#include "AudacityException.h"

//--#include <wx/atomic.h>
//--#include "widgets/ErrorDialog.h"

AudacityException::~AudacityException()
{
}

QAtomicInt sOutstandingMessages;

MessageBoxException::MessageBoxException( const QString &caption_ )
   : caption{ caption_ }
{
   if (!caption.isEmpty())
      sOutstandingMessages.ref();//--wxAtomicInc( sOutstandingMessages );
   else
      // invalidate me
      moved = true;
}

// The class needs a copy constructor to be throwable
// (or will need it, by C++14 rules).  But the copy
// needs to act like a move constructor.  There must be unique responsibility
// for each exception thrown to decrease the global count when it is handled.
MessageBoxException::MessageBoxException( const MessageBoxException& that )
   : AudacityException(that)
{
   caption = that.caption;
   moved = that.moved;
   that.moved = true;
}

MessageBoxException &MessageBoxException::operator = ( MessageBoxException &&that )
{
   caption = that.caption;
   if ( this != &that ) {
      AudacityException::operator=( std::move(that) );
      if (!moved)
         sOutstandingMessages.deref();//--wxAtomicDec( sOutstandingMessages );

      moved = that.moved;
      that.moved = true;
   }

   return *this;
}

MessageBoxException::~MessageBoxException()
{
   if (!moved)
      // If exceptions are used properly, you should never reach this,
      // because moved should become true earlier in the object's lifetime.
      sOutstandingMessages.deref();//--wxAtomicDec( sOutstandingMessages );
}

SimpleMessageBoxException::~SimpleMessageBoxException()
{
}

QString SimpleMessageBoxException::ErrorMessage() const
{
   return message;
}

std::unique_ptr< AudacityException > SimpleMessageBoxException::Move()
{
   return std::unique_ptr< AudacityException >
   { safenew SimpleMessageBoxException{ std::move( *this ) } };
}

// This is meant to be invoked via wxEvtHandler::CallAfter
void MessageBoxException::DelayedHandlerAction()
{
   if (!moved) {
      // This test prevents accumulation of multiple messages between idle
      // times of the main even loop.  Only the last queued exception
      // displays its message.  We assume that multiple messages have a
      // common cause such as exhaustion of disk space so that the others
      // give the user no useful added information.
      //--if ( wxAtomicDec( sOutstandingMessages ) == 0 )
      QString msg = QString("%1: %2").arg(caption).arg(ErrorMessage());
      
      if ( !sOutstandingMessages.deref() )
         //--::AudacityMessageBox(
         //--   ErrorMessage(),
         //--   caption.IsEmpty() ? wxMessageBoxCaptionStr : caption,
         //--   wxICON_ERROR
         //--);
         qCritical("%s", qPrintable(msg));
      else
         qWarning("%s", qPrintable(msg));
      
      moved = true;
   }
}
