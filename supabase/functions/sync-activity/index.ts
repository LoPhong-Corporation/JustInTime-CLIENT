// supabase/functions/sync-activity/index.ts
//
// Edge Function nhận dữ liệu activity từ agent JustInTime,
// validate, rồi ghi vào bảng activity_logs bằng service_role
// key (chỉ tồn tại trên server Supabase, KHÔNG bao giờ lộ ra
// client). Nhờ vậy client (.exe) chỉ cần cầm publishable key
// (vốn dĩ công khai), không cần và không nên cầm service_role.
//
// Deploy: supabase functions deploy sync-activity

import { createClient } from "https://esm.sh/@supabase/supabase-js@2";

const SUPABASE_URL = Deno.env.get("SUPABASE_URL")!;
const SERVICE_ROLE_KEY = Deno.env.get("SUPABASE_SERVICE_ROLE_KEY")!;

const supabase = createClient(SUPABASE_URL, SERVICE_ROLE_KEY, {
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

  const { error } = await supabase
    .from("activity_logs")
    .upsert(
      {
        device_id,
        process_name,
        window_title,
        duration_seconds,
        start_time,
        end_time,
      },
      { onConflict: "device_id,start_time" }
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
