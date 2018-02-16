#include "DialogManager.h"

std::vector<int> DialogManager::sButtonClicks;

ProgressDialog::ProgressDialog(const QString &caption, const QString &message)
 : mCaption(caption), mMessage(message)
{

}

ProgressDialog::Result ProgressDialog::Update(int count, int total)
{
   qInfo("%s: %s: %d/%d", qPrintable(mCaption), qPrintable(mMessage), count, total);
   return Success;
}

std::unique_ptr<ProgressDialog> DialogManager::NewProgressDialog(const QString& caption, const QString& message)
{
   return std::make_unique<ProgressDialog>(caption, message);
}

int DialogManager::ShowMultiDialog(const QString& msg, const QString& caption, const QStringList& buttons)
{
   (void)msg;

   qDebug() << wxT("ShowMultiDialog: ") << caption;
   qDebug() << wxT("               : ") << msg;
   for (int i = 0; i < buttons.count(); i++)
      qDebug() << QString(wxT("%1: %2")).arg(i).arg(buttons[i]);

   int button = 0;
   if (sButtonClicks.size() > 0)
   {
      button = sButtonClicks.back();
      if (button >= buttons.count())
      {
         qWarning() << wxT("Invalid button click: ") << button;
         button = 0;
      }
      sButtonClicks.pop_back();
   }
   qDebug() << wxT("ShowMultiDialog: click button") << button;
   return button;
}

void DialogManager::PushButtonClick(int button)
{
   sButtonClicks.push_back(button);
}
