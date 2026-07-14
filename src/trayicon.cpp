// trayicon.cpp

#include "trayicon.h"
#include "logindialog.h"
#include "settingsdialog.h"
#include "supabasesetupdialog.h"
#include "remoteviewdialog.h"

#include <QApplication>
#include <QMessageBox>
#include <QStyle>

#include <windows.h>

extern "C" {
#include "../include/auth.h"
#include "../include/database.h"
}

TrayIcon::TrayIcon(QObject *parent) : QObject(parent)
{
    m_trayIcon.setIcon(qApp->style()->standardIcon(QStyle::SP_ComputerIcon));

    m_accountAction = m_menu.addAction(QString());
    m_accountAction->setEnabled(false);

    m_loginAction  = m_menu.addAction("Log in / Sign up...", this, &TrayIcon::onLogin);
    m_logoutAction = m_menu.addAction("Log out", this, &TrayIcon::onLogout);

    m_menu.addSeparator();

    m_pauseAction  = m_menu.addAction("Pause tracking", this, &TrayIcon::onTogglePause);
    m_reportAction = m_menu.addAction("View today's report", this, &TrayIcon::onViewReport);

    m_menu.addSeparator();

    m_settingsAction = m_menu.addAction("Settings...", this, &TrayIcon::onSettings);
    m_supabaseAction = m_menu.addAction("Supabase Setup...", this, &TrayIcon::onSupabaseSetup);
    m_remoteViewAction = m_menu.addAction("Remote View...", this, &TrayIcon::onRemoteView);

    m_debugAction = m_menu.addAction("Show debug console", this, &TrayIcon::onToggleDebugConsole);
    m_startWebSVAction = m_menu.addAction("Start Web Dashboard", this, &TrayIcon::onStartWebSVAction);
    m_debugAction->setCheckable(true);

    m_menu.addSeparator();
    m_exitAction = m_menu.addAction("Exit", this, &TrayIcon::onExit);

    m_trayIcon.setContextMenu(&m_menu);

    connect(&m_trayIcon, &QSystemTrayIcon::activated, this, &TrayIcon::onActivated);

    rebuildMenu();
}

void TrayIcon::show()
{
    m_trayIcon.show();
}

void TrayIcon::onActivated(QSystemTrayIcon::ActivationReason reason)
{
    Q_UNUSED(reason);

    /*
     * Đảm bảo trạng thái đăng nhập/tạm dừng luôn cập nhật
     * đúng trước khi hiện menu (vì có thể đổi từ dialog).
     */
    rebuildMenu();
}

void TrayIcon::rebuildMenu()
{
    AuthSession session;
    auth_get_session(&session);

    if (session.logged_in)
    {
        m_accountAction->setText(
            QString("Logged in: %1").arg(QString::fromUtf8(session.email))
        );
        m_accountAction->setVisible(true);
        m_loginAction->setVisible(false);
        m_logoutAction->setVisible(true);
    }
    else
    {
        m_accountAction->setVisible(false);
        m_loginAction->setVisible(true);
        m_logoutAction->setVisible(false);
    }

    m_pauseAction->setText(m_paused.load() ? "Resume tracking" : "Pause tracking");
    m_debugAction->setChecked(m_debugVisible);

    updateTooltip();
}

void TrayIcon::updateTooltip()
{
    AuthSession session;
    auth_get_session(&session);

    QString tip;

    if (session.logged_in)
    {
        tip = QString("JustInTime - %1%2")
                  .arg(QString::fromUtf8(session.email))
                  .arg(m_paused.load() ? " (paused)" : "");
    }
    else
    {
        tip = QString("JustInTime - not logged in%1")
                  .arg(m_paused.load() ? " (paused)" : "");
    }

    m_trayIcon.setToolTip(tip);
}

void TrayIcon::onLogin()
{
    LoginDialog dlg;

    if (dlg.exec() == QDialog::Accepted)
        rebuildMenu();
}

void TrayIcon::onLogout()
{
    if (
        QMessageBox::question(
            nullptr,
            "JustInTime",
            "Log out of the current account?"
        ) == QMessageBox::Yes
    )
    {
        auth_logout();
        rebuildMenu();

        QMessageBox::information(
            nullptr,
            "JustInTime",
            "Logged out. Data is still saved locally, but cloud sync is paused."
        );
    }
}

void TrayIcon::onTogglePause()
{
    m_paused = !m_paused.load();
    rebuildMenu();
}

void TrayIcon::onViewReport()
{
    wchar_t buffer[4096] = {0};

    db_build_daily_summary_text(buffer, 4096);

    QString text = QString("Today's summary:\n\n%1")
                       .arg(QString::fromWCharArray(buffer));

    QMessageBox::information(nullptr, "JustInTime", text);
}

void TrayIcon::onSettings()
{
    SettingsDialog dlg;
    dlg.exec();
}

void TrayIcon::onSupabaseSetup()
{
    SupabaseSetupDialog dlg;
    dlg.exec();
}

void TrayIcon::onRemoteView()
{
    RemoteViewDialog dlg;
    dlg.exec();
}

void TrayIcon::onToggleDebugConsole()
{
    m_debugVisible = !m_debugVisible;

    HWND console = GetConsoleWindow();

    if (console)
        ShowWindow(console, m_debugVisible ? SW_SHOW : SW_HIDE);

    m_debugAction->setChecked(m_debugVisible);
}

void TrayIcon::onStartWebSVAction()
{
    system("pythonw ./dashboard/app.pyw");
    // Start the WebSV server
    if (system("pythonw ./dashboard/app.pyw") == -1)
    {
        QMessageBox::critical(nullptr, "JustInTime", "Failed to start WebSV server.");
    }
}

void TrayIcon::onExit()
{
    qApp->quit();
}
