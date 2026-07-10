// supabase/functions/sync-activity/index.ts
//
// Edge Function nhận dữ liệu activity từ agent JustInTime,
// validate, xác thực NGƯỜI DÙNG THẬT qua access_token trong
// header Authorization (không phải publishable key), rồi ghi
// vào bảng activity_logs bằng service_role key (chỉ tồn tại
// trên server Supabase, KHÔNG bao giờ lộ ra client).
//
// Nhờ vậy: mỗi record được gắn đúng user_id của người đăng
// nhập, và client (.exe) không bao giờ cầm service_role.
//
// Deploy: supabase functions deploy sync-activity

import { createClient } from "https://esm.sh/@supabase/supabase-js@2";

const SUPABASE_URL = Deno.env.get("SUPABASE_URL")!;
const SERVICE_ROLE_KEY = Deno.env.get("SUPABASE_SERVICE_ROLE_KEY")!;

/*
 * Client dùng service_role để GHI dữ liệu (bỏ qua RLS).
 */
const adminClient = createClient(SUPABASE_URL, SERVICE_ROLE_KEY, {
  auth: { persistSession: false },
});

const MAX_ID_LEN = 128;
const MAX_PROCESS_LEN = 512;
const MAX_TITLE_LEN = 4000;

function isNonEmptyString(v: unknown, maxLen: number): v is string {
  return typeof v === "string" && v.length > 0 && v.length <= maxLen;
}

function isFiniteNumber(v: unknown): v is number {
  return typeof v === "number" && Number.isFinite(v);
}

Deno.serve(async (req: Request) => {
  if (req.method !== "POST") {
    return new Response(
      JSON.stringify({ error: "Method not allowed" }),
      { status: 405, headers: { "Content-Type": "application/json" } }
    );
  }

  /*
   * Bắt buộc phải có access_token thật của user đăng nhập
   * (không phải publishable key) trong header Authorization.
   * Dùng chính access_token này để xác thực -> lấy user_id,
   * đảm bảo mỗi client chỉ ghi được dữ liệu gắn với TÀI KHOẢN
   * CỦA CHÍNH HỌ, không thể giả mạo user_id trong body.
   */
  const authHeader = req.headers.get("Authorization") ?? "";
  const accessToken = authHeader.replace(/^Bearer\s+/i, "");

  if (!accessToken) {
    return new Response(
      JSON.stringify({ error: "Missing access token" }),
      { status: 401, headers: { "Content-Type": "application/json" } }
    );
  }

  const { data: userData, error: userError } =
    await adminClient.auth.getUser(accessToken);

  if (userError || !userData?.user) {
    return new Response(
      JSON.stringify({ error: "Invalid or expired session, please login again" }),
      { status: 401, headers: { "Content-Type": "application/json" } }
    );
  }

  const user_id = userData.user.id;

  let payload: unknown;

  try {
    payload = await req.json();
  } catch {
    return new Response(
      JSON.stringify({ error: "Invalid JSON" }),
      { status: 400, headers: { "Content-Type": "application/json" } }
    );
  }

  const body = payload as Record<string, unknown>;

  const device_id = body?.device_id;
  const process_name = body?.process_name;
  const window_title = body?.window_title;
  const duration_seconds = body?.duration_seconds;
  const start_time = body?.start_time;
  const end_time = body?.end_time;

  if (
    !isNonEmptyString(device_id, MAX_ID_LEN) ||
    !isNonEmptyString(process_name, MAX_PROCESS_LEN) ||
    typeof window_title !== "string" ||
    window_title.length > MAX_TITLE_LEN ||
    !isFiniteNumber(duration_seconds) ||
    !isFiniteNumber(start_time) ||
    !isFiniteNumber(end_time) ||
    duration_seconds < 0 ||
    end_time < start_time
  ) {
    return new Response(
      JSON.stringify({ error: "Invalid payload" }),
      { status: 400, headers: { "Content-Type": "application/json" } }
    );
  }

  const { error } = await adminClient
    .from("activity_logs")
    .upsert(
      {
        user_id,
        device_id,
        process_name,
        window_title,
        duration_seconds,
        start_time,
        end_time,
      },
      { onConflict: "user_id,device_id,start_time" }
    );

  if (error) {
    console.error(error);

    return new Response(
      JSON.stringify({ error: "Insert failed" }),
      { status: 500, headers: { "Content-Type": "application/json" } }
    );
  }

  return new Response(
    JSON.stringify({ ok: true }),
    { status: 200, headers: { "Content-Type": "application/json" } }
  );
});
