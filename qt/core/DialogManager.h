#pragma once
#include "MemoryX.h"

class ProgressDialog {
public:
   enum Result {
      Success = 0
   };

   ProgressDialog(const QString& caption, const QString& message);
   Result Update(int count, int total);

private:
   QString mCaption, mMessage;
};

class DialogManager {
public:
  // construct and maybe show a progress dialog
  static std::unique_ptr<ProgressDialog> NewProgressDialog(const QString& caption, const QString& message);

  // show a dialog with one or more buttons, return index of the clicked button
  // button 0 seems to be the default button (e.g. for safe or nodestructive choice)
  // pop button click from stack if there is one available, and return its value
  static int ShowMultiDialog(const QString& msg, const QString& caption, const QStringList& buttons);

  // push a simulated button click onto stack
  static void PushButtonClick(int button);

private:
  static std::vector<int> sButtonClicks;
};
