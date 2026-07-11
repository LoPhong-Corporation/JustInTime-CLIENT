//
// auth.h
// Đăng nhập / đăng ký qua Supabase Auth (GoTrue REST API),
// lưu session cục bộ, mã hóa bằng Windows DPAPI (gắn với
// tài khoản Windows hiện tại của bạn, giống cách trình
// duyệt lưu mật khẩu đã lưu).
//

#ifndef AUTH_H
#define AUTH_H

#ifdef __cplusplus
extern "C" {
#endif


#define MAX_TOKEN_LEN 4096
#define MAX_EMAIL_LEN  256

typedef struct
{
    int  logged_in;

    char user_id[64];
    char email[MAX_EMAIL_LEN];

    char access_token[MAX_TOKEN_LEN];
    char refresh_token[512];

} AuthSession;

/*
 * Đăng ký tài khoản mới.
 * Trả về 1 nếu thành công, 0 nếu thất bại
 * (err_out sẽ chứa lý do, lấy từ Supabase).
 */
int auth_register(
    const char* email,
    const char* password,
    char* err_out,
    int err_out_size
);

/*
 * Đăng nhập. Nếu thành công, session được lưu lại
 * (mã hóa) và có thể lấy qua auth_get_session().
 */
int auth_login(
    const char* email,
    const char* password,
    char* err_out,
    int err_out_size
);

/*
 * Đăng xuất: xóa session khỏi bộ nhớ và file lưu.
 */
void auth_logout(void);

/*
 * Nạp lại session đã lưu từ trước (lúc khởi động app).
 * Trả về 1 nếu có session hợp lệ, 0 nếu chưa đăng nhập.
 */
int auth_load_session(void);

/*
 * Lấy bản sao session hiện tại (thread-safe).
 */
void auth_get_session(AuthSession* out);

/*
 * Có đang đăng nhập không.
 */
int auth_is_logged_in(void);


#ifdef __cplusplus
}
#endif

#endif
