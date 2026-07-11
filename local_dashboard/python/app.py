"""
JustInTime Local Dashboard
--------------------------
Flask Entry Point

Dùng local dashboard để hiển thị dữ liệu từ SQLite và cấu hình máy. 
Chưa share dữ liệu ra ngoài, chỉ dùng cho local.
"""

from flask import Flask, render_template

# Khởi tạo Flask
app = Flask(
    __name__,
    template_folder="../html/templates",
    static_folder="../html"
)


@app.route("/")
def dashboard():
    """
    Trang Dashboard.
    """
    return render_template("dashboard.html")


if __name__ == "__main__":
    app.run(
        host="127.0.0.1",
        port=5000,
        debug=True
    )