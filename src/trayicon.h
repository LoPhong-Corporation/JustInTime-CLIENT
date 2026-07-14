// trayicon.h
// Icon khay hệ thống bằng Qt (QSystemTrayIcon), gọi thẳng
// vào các hàm C lõi (auth_*, database_*, settings_*).
#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>

#include <atomic>

class TrayIcon : public QObject
{
    Q_OBJECT

public:
    explicit TrayIcon(QObject *parent = nullptr);

    void show();

    /*
     * Có đang tạm dừng theo dõi không. Được gọi từ worker
     * thread (std::atomic nên an toàn khi đọc cross-thread).
     */
    bool isPaused() const { return m_paused.load(); }

private slots:
    void onLogin();
    void onLogout();
    void onTogglePause();
    void onViewReport();
    void onSettings();
    void onSupabaseSetup();
    void onRemoteView();
    void onToggleDebugConsole();
    void onExit();
    void onStartWebSVAction();
    void onActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void rebuildMenu();
    void updateTooltip();

    QSystemTrayIcon m_trayIcon;
    QMenu           m_menu;

    QAction *m_accountAction  = nullptr;
    QAction *m_loginAction    = nullptr;
    QAction *m_logoutAction   = nullptr;
    QAction *m_pauseAction    = nullptr;
    QAction *m_reportAction   = nullptr;
    QAction *m_settingsAction = nullptr;
    QAction *m_supabaseAction = nullptr;
    QAction *m_remoteViewAction = nullptr;
    QAction *m_debugAction    = nullptr;
    QAction *m_exitAction     = nullptr;
    QAction *m_startWebSVAction = nullptr;

    std::atomic<bool> m_paused{false};
    bool m_debugVisible = false;
};
