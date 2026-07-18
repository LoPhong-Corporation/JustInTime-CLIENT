// trayicon.cpp

#include "trayicon.h"
#include "logindialog.h"
#include "settingsdialog.h"
#include "supabasesetupdialog.h"
#include "remoteviewdialog.h"

#include <QApplication>
#include <QMessageBox>
#include <QStyle>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>

#include <windows.h>

extern "C" {
#include "../include/auth.h"
#include "../include/database.h"
}

namespace {

/*
 * Cả "dashboard" (Python) và "dashboard-go" đều không được CMake copy
 * theo exe (chưa có bước cài đặt chính thức), nên tìm chúng bằng cách
 * dò vài cấp thư mục cha kể từ nơi JustInTime.exe đang chạy — bản dev
 * build thường nằm ở build/<Config>/JustInTime.exe, tức thư mục gốc
 * repo cách đó 2 cấp.
 */
QString findResourceDir(const QString &relativeName)
{
    const QString exeDir = QCoreApplication::applicationDirPath();

    const QStringList candidates = {
        exeDir + "/" + relativeName,
        exeDir + "/../" + relativeName,
        exeDir + "/../../" + relativeName,
        exeDir + "/../../../" + relativeName,
    };

    for (const QString &candidate : candidates)
    {
        QDir dir(candidate);
        if (dir.exists())
            return dir.absolutePath();
    }

    // Không tìm thấy đâu cả — trả về ứng viên "cạnh exe" để thông báo
    // lỗi phía sau còn có đường dẫn cụ thể để hiển thị cho người dùng.
    return QDir(exeDir + "/" + relativeName).absolutePath();
}

bool isPortOpen(quint16 port)
{
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, port);
    return socket.waitForConnected(150);
}

} // namespace

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

    m_menu.addSeparator();

    m_dashboardMenu = m_menu.addMenu("Dashboards");
    m_pythonDashboardAction = m_dashboardMenu->addAction(
        "Open Web Dashboard (Python)", this, &TrayIcon::onOpenPythonDashboard
    );
    m_goDashboardAction = m_dashboardMenu->addAction(
        "Open Local Dashboard (Go)", this, &TrayIcon::onOpenGoDashboard
    );

    const QString dashboardTip =
        "Both dashboards show the same data on http://127.0.0.1:5000 "
        "(they're two implementations of the same dashboard, not meant "
        "to run at the same time). Whichever is already running will "
        "just be opened again.";
    m_pythonDashboardAction->setToolTip(dashboardTip);
    m_goDashboardAction->setToolTip(dashboardTip);

    m_debugAction = m_menu.addAction("Show debug console", this, &TrayIcon::onToggleDebugConsole);
    //m_startWebSVAction = m_menu.addAction("Start Web Dashboard", this, &TrayIcon::onStartWebSVAction);
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

/*void TrayIcon::onStartWebSVAction()
{
    system("pythonw ./dashboard/app.pyw");
    // Start the WebSV server
    if (system("pythonw ./dashboard/app.pyw") == -1)
    {
        QMessageBox::critical(nullptr, "JustInTime", "Failed to start WebSV server.");
    }
}*/

void TrayIcon::launchDashboard(
    const QString &label,
    quint16 port,
    const QString &program,
    const QStringList &arguments,
    const QString &workingDir
)
{
    if (isPortOpen(port))
    {
        /*
         * Đã có tiến trình lắng nghe cổng này rồi (Python và Go dùng
         * chung cổng 5000 vì cả hai là 2 cách triển khai của CÙNG một
         * dashboard, không nhằm chạy song song) - chỉ cần mở trình
         * duyệt tới, không cần spawn thêm tiến trình mới.
         */
        QDesktopServices::openUrl(QUrl(QString("http://127.0.0.1:%1/").arg(port)));
        return;
    }

    qint64 pid = 0;
    const bool started = QProcess::startDetached(program, arguments, workingDir, &pid);

    if (!started)
    {
        QMessageBox::warning(
            nullptr,
            "JustInTime",
            QString("Could not start %1.\n\nTried to run:\n%2 %3\n\nWorking directory:\n%4")
                .arg(label)
                .arg(program)
                .arg(arguments.join(' '))
                .arg(workingDir)
        );
        return;
    }

    /*
     * Chờ một chút cho server bind cổng + render lần đầu rồi mới mở
     * trình duyệt, thay vì mở ngay lập tức và dễ gặp "connection
     * refused" nếu server chưa kịp sẵn sàng.
     */
    QTimer::singleShot(1200, [port]() {
        QDesktopServices::openUrl(QUrl(QString("http://127.0.0.1:%1/").arg(port)));
    });
}

void TrayIcon::onOpenPythonDashboard()
{
    const QString dashboardDir = findResourceDir("dashboard");
    const QString scriptPath = dashboardDir + "/app.pyw";

    if (!QFile::exists(scriptPath))
    {
        QMessageBox::warning(
            nullptr,
            "JustInTime",
            QString(
                "Could not find the Python dashboard at:\n%1\n\n"
                "Make sure the 'dashboard' folder sits next to JustInTime.exe "
                "(or in the project root, for dev builds), and that Python "
                "with the packages in dashboard/requirements.txt is installed."
            ).arg(scriptPath)
        );
        return;
    }

    launchDashboard("the Python dashboard", 5000, "pythonw", { scriptPath }, dashboardDir);
}

void TrayIcon::onOpenGoDashboard()
{
    const QString goDir = findResourceDir("dashboard-go");
    const QString exePath = goDir + "/dashboard.exe";

    if (!QFile::exists(exePath))
    {
        QMessageBox::warning(
            nullptr,
            "JustInTime",
            QString(
                "Could not find the Go dashboard at:\n%1\n\n"
                "Build it first with:\n"
                "  cd dashboard-go\n"
                "  go build -o dashboard.exe ./cmd/dashboard"
            ).arg(exePath)
        );
        return;
    }

    /*
     * Không truyền --tray ở đây: cái tray bạn đang thấy CHÍNH LÀ
     * tray rồi. dashboard.exe khi chạy không có --tray chỉ start
     * server + tự mở trình duyệt rồi chạy nền, không tạo thêm icon
     * khay hệ thống thứ hai (xem cmd/dashboard/main.go).
     */
    launchDashboard("the Go dashboard", 5000, exePath, {}, goDir);
}

void TrayIcon::onExit()
{
    qApp->quit();
}
