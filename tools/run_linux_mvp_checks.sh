#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${REPO_ROOT}"

EXECUTABLE_PATH="${1:-build/modern-video-player}"
PROBE_FILE="${2:-samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4}"
SAMPLE_MS="${3:-1800}"
ASS_SUBTITLE_FILE="${4:-samples/subtitles/opengl_ass_style_validation.ass}"
EMBEDDED_SUBTITLE_MEDIA_FILE="${5:-build/tmp/embedded-ass-validation.mkv}"
SUBTITLE_MAX_PACKETS="${6:-120}"
REQUIRE_OPTIONAL_CHECKS="${7:-${MVP_REQUIRE_OPTIONAL_CHECKS:-0}}"
EMBEDDED_SUBTITLE_BASE_MEDIA_FILE="${8:-${PROBE_FILE}}"
EMBEDDED_ASS_SUBTITLE_FILE="${9:-samples/subtitles/opengl_ass_transform_transition_validation.ass}"
REPORT_FILE="${10:-${MVP_LINUX_GATE_REPORT_FILE:-}}"
REQUIRE_VULKAN_CHECKS="${11:-${MVP_REQUIRE_VULKAN_CHECKS:-0}}"

REPORT_PATH=""
GATE_RESULT_RECORDED=0

resolve_project_path() {
  local value="$1"
  if [[ -z "${value}" ]]; then
    echo ""
    return 0
  fi
  if [[ "${value}" = /* ]]; then
    echo "${value}"
  else
    echo "${REPO_ROOT}/${value}"
  fi
}

is_truthy() {
  local value="${1:-}"
  value="${value,,}"
  [[ "${value}" == "1" || "${value}" == "true" || "${value}" == "yes" || "${value}" == "on" ]]
}

ensure_embedded_ass_sample() {
  local output_path="$1"
  local base_media_path="$2"
  local subtitle_path="$3"

  if [[ -f "${output_path}" ]]; then
    return 0
  fi

  if [[ ! -f "${base_media_path}" ]]; then
    echo "Cannot generate CP-508 media: base media missing: ${base_media_path}"
    return 1
  fi
  if [[ ! -f "${subtitle_path}" ]]; then
    echo "Cannot generate CP-508 media: ASS subtitle missing: ${subtitle_path}"
    return 1
  fi
  if ! command -v ffmpeg >/dev/null 2>&1; then
    echo "Cannot generate CP-508 media: ffmpeg not found in PATH."
    return 1
  fi

  mkdir -p "$(dirname "${output_path}")"

  echo "Generating CP-508 embedded subtitle media: ${output_path}"
  if ! ffmpeg -y -loglevel error \
    -i "${base_media_path}" \
    -i "${subtitle_path}" \
    -map 0:v \
    -map 0:a? \
    -map 1:0 \
    -c:v copy \
    -c:a copy \
    -c:s ass \
    "${output_path}"; then
    echo "Failed to generate CP-508 embedded subtitle media: ${output_path}"
    rm -f "${output_path}"
    return 1
  fi

  if [[ ! -f "${output_path}" ]]; then
    echo "Failed to generate CP-508 embedded subtitle media: output file missing after ffmpeg run."
    return 1
  fi
  return 0
}

escape_report_value() {
  local value="${1:-}"
  value="${value//$'\r'/ }"
  value="${value//$'\n'/ }"
  printf '%s' "${value}"
}

write_report_line() {
  local line="$1"
  if [[ -z "${REPORT_PATH}" ]]; then
    return 0
  fi
  printf '%s\n' "${line}" >> "${REPORT_PATH}"
}

set_gate_result_once() {
  local result="$1"
  if [[ "${GATE_RESULT_RECORDED}" -ne 0 ]]; then
    return 0
  fi
  write_report_line "gate.result=${result}"
  GATE_RESULT_RECORDED=1
}

report_fail_and_exit() {
  local reason="$1"
  set_gate_result_once "FAIL"
  write_report_line "gate.fail_reason=$(escape_report_value "${reason}")"
  echo "${reason}"
  exit 1
}

print_summary_lines() {
  local text="$1"
  local summary_regex="$2"
  if [[ -z "${summary_regex}" ]]; then
    return 0
  fi
  printf '%s\n' "${text}" | grep -E "${summary_regex}" || true
}

run_check() {
  local index="$1"
  local total="$2"
  local check_id="$3"
  local title="$4"
  local env_assignments="$5"
  local summary_regex="$6"
  shift 6

  local -a required_patterns=()
  while [[ $# -gt 0 ]]; do
    if [[ "$1" == "--" ]]; then
      shift
      break
    fi
    required_patterns+=("$1")
    shift
  done

  local -a args=("$@")
  local command_text="${EXE_PATH} ${args[*]}"
  if [[ -n "${env_assignments}" ]]; then
    command_text="env ${env_assignments} ${command_text}"
  fi

  echo
  echo "[${index}/${total}] ${title}"
  echo "Command: ${command_text}"
  write_report_line "check.${check_id}.title=$(escape_report_value "${title}")"
  write_report_line "check.${check_id}.command=$(escape_report_value "${command_text}")"

  set +e
  local output
  if [[ -n "${env_assignments}" ]]; then
    read -r -a env_parts <<< "${env_assignments}"
    output="$(env "${env_parts[@]}" "${EXE_PATH}" "${args[@]}" 2>&1)"
  else
    output="$("${EXE_PATH}" "${args[@]}" 2>&1)"
  fi
  local exit_code=$?
  set -e

  print_summary_lines "${output}" "${summary_regex}"
  echo "Exit code: ${exit_code}"

  local missing=0
  for pattern in "${required_patterns[@]}"; do
    if ! printf '%s\n' "${output}" | grep -Eq "${pattern}"; then
      echo "Missing pattern: ${pattern}"
      missing=1
    fi
  done

  if [[ ${exit_code} -ne 0 || ${missing} -ne 0 ]]; then
    write_report_line "check.${check_id}.status=FAIL"
    write_report_line "check.${check_id}.exit_code=${exit_code}"
    write_report_line "check.${check_id}.missing_patterns=${missing}"
    echo "Status: FAIL"
    echo "--- Full output start ---"
    printf '%s\n' "${output}"
    echo "--- Full output end ---"
    return 1
  fi

  write_report_line "check.${check_id}.status=PASS"
  write_report_line "check.${check_id}.exit_code=${exit_code}"
  write_report_line "check.${check_id}.missing_patterns=0"
  echo "Status: PASS"
}

probe_vulkan_check_availability() {
  local output=""
  local exit_code=0
  local supported_platform="unknown"
  local compiled_in="unknown"
  local runtime_available="unknown"

  set +e
  output="$("${EXE_PATH}" --vulkan-diagnostics 2>&1)"
  exit_code=$?
  set -e

  print_summary_lines "${output}" 'vulkan-diagnostics\.(supported_platform|compiled_in|runtime_available|result)='
  echo "Vulkan diagnostics probe exit code: ${exit_code}"

  local line=""
  line="$(printf '%s\n' "${output}" | grep -E '^vulkan-diagnostics\.supported_platform=' | tail -n1 || true)"
  if [[ -n "${line}" ]]; then
    supported_platform="${line#*=}"
  fi

  line="$(printf '%s\n' "${output}" | grep -E '^vulkan-diagnostics\.compiled_in=' | tail -n1 || true)"
  if [[ -n "${line}" ]]; then
    compiled_in="${line#*=}"
  fi

  line="$(printf '%s\n' "${output}" | grep -E '^vulkan-diagnostics\.runtime_available=' | tail -n1 || true)"
  if [[ -n "${line}" ]]; then
    runtime_available="${line#*=}"
  fi

  write_report_line "gate.vulkan_probe_exit_code=${exit_code}"
  write_report_line "gate.vulkan_supported_platform=$(escape_report_value "${supported_platform}")"
  write_report_line "gate.vulkan_compiled_in=$(escape_report_value "${compiled_in}")"
  write_report_line "gate.vulkan_runtime_available=$(escape_report_value "${runtime_available}")"

  if [[ "${supported_platform}" == "true" && "${compiled_in}" == "true" && "${runtime_available}" == "true" && ${exit_code} -eq 0 ]]; then
    HAS_VK010=1
    VULKAN_SKIP_REASON="none"
    return 0
  fi

  HAS_VK010=0
  VULKAN_SKIP_REASON="supported_platform=${supported_platform}, compiled_in=${compiled_in}, runtime_available=${runtime_available}, exit_code=${exit_code}"
  if is_truthy "${REQUIRE_VULKAN_CHECKS}"; then
    report_fail_and_exit "VK-010 check is required but unavailable (${VULKAN_SKIP_REASON})."
  fi
  return 0
}

if [[ "$(uname -s)" != "Linux" ]]; then
  echo "This gate script only supports Linux."
  exit 1
fi

EXE_PATH="$(resolve_project_path "${EXECUTABLE_PATH}")"
MEDIA_PATH="$(resolve_project_path "${PROBE_FILE}")"
ASS_SUBTITLE_PATH="$(resolve_project_path "${ASS_SUBTITLE_FILE}")"
EMBEDDED_SUBTITLE_MEDIA_PATH="$(resolve_project_path "${EMBEDDED_SUBTITLE_MEDIA_FILE}")"
EMBEDDED_SUBTITLE_BASE_MEDIA_PATH="$(resolve_project_path "${EMBEDDED_SUBTITLE_BASE_MEDIA_FILE}")"
EMBEDDED_ASS_SUBTITLE_PATH="$(resolve_project_path "${EMBEDDED_ASS_SUBTITLE_FILE}")"
REPORT_PATH="$(resolve_project_path "${REPORT_FILE}")"

if [[ -n "${REPORT_PATH}" ]]; then
  mkdir -p "$(dirname "${REPORT_PATH}")"
  : > "${REPORT_PATH}"
  write_report_line "gate.script=tools/run_linux_mvp_checks.sh"
  write_report_line "gate.executable=$(escape_report_value "${EXE_PATH}")"
  write_report_line "gate.probe_media=$(escape_report_value "${MEDIA_PATH}")"
  write_report_line "gate.sample_ms=${SAMPLE_MS}"
  write_report_line "gate.require_optional_checks=$(escape_report_value "${REQUIRE_OPTIONAL_CHECKS}")"
  write_report_line "gate.require_vulkan_checks=$(escape_report_value "${REQUIRE_VULKAN_CHECKS}")"
  write_report_line "gate.ass_subtitle_file=$(escape_report_value "${ASS_SUBTITLE_PATH}")"
  write_report_line "gate.embedded_subtitle_media_file=$(escape_report_value "${EMBEDDED_SUBTITLE_MEDIA_PATH}")"
  write_report_line "gate.embedded_subtitle_base_media_file=$(escape_report_value "${EMBEDDED_SUBTITLE_BASE_MEDIA_PATH}")"
  write_report_line "gate.embedded_ass_subtitle_file=$(escape_report_value "${EMBEDDED_ASS_SUBTITLE_PATH}")"
  trap 'exit_code=$?; if [[ ${exit_code} -ne 0 ]]; then set_gate_result_once "FAIL"; fi' EXIT
fi

if [[ ! -f "${EXE_PATH}" ]]; then
  report_fail_and_exit "Executable not found: ${EXE_PATH}"
fi
if [[ ! -f "${MEDIA_PATH}" ]]; then
  report_fail_and_exit "Probe media not found: ${MEDIA_PATH}"
fi

if [[ ! -f "${EMBEDDED_SUBTITLE_MEDIA_PATH}" ]]; then
  ensure_embedded_ass_sample "${EMBEDDED_SUBTITLE_MEDIA_PATH}" "${EMBEDDED_SUBTITLE_BASE_MEDIA_PATH}" "${EMBEDDED_ASS_SUBTITLE_PATH}" || true
fi

HAS_CP507=0
if [[ -f "${ASS_SUBTITLE_PATH}" ]]; then
  HAS_CP507=1
fi

HAS_CP508=0
if [[ -f "${EMBEDDED_SUBTITLE_MEDIA_PATH}" ]]; then
  HAS_CP508=1
fi

if is_truthy "${REQUIRE_OPTIONAL_CHECKS}"; then
  if [[ ${HAS_CP507} -eq 0 ]]; then
    report_fail_and_exit "CP-507 check is required but subtitle file is missing: ${ASS_SUBTITLE_PATH}"
  fi
  if [[ ${HAS_CP508} -eq 0 ]]; then
    report_fail_and_exit "CP-508 check is required but embedded subtitle media is unavailable: ${EMBEDDED_SUBTITLE_MEDIA_PATH}"
  fi
fi

HAS_VK010=0
VULKAN_SKIP_REASON="not-probed"
probe_vulkan_check_availability

EXTRA_CHECKS=$((HAS_CP507 + HAS_CP508 + HAS_VK010))
write_report_line "gate.has_cp507=${HAS_CP507}"
write_report_line "gate.has_cp508=${HAS_CP508}"
write_report_line "gate.has_vk010=${HAS_VK010}"
write_report_line "gate.vulkan_skip_reason=$(escape_report_value "${VULKAN_SKIP_REASON}")"

TOTAL=$((8 + EXTRA_CHECKS))
INDEX=1
write_report_line "gate.total_checks=${TOTAL}"

run_check "${INDEX}" "${TOTAL}" "cp902_observability_baseline" "CP-902 observability baseline" "MVP_RENDERER_BACKEND=opengl" \
  'performance-log-check\.(renderer_backend|decoder_backend|scheduler_render_wait_ms|runtime_drop_total|result)=' \
  'performance-log-check\.renderer_backend=OpenGL' \
  'performance-log-check\.scheduler_render_wait_ms=[0-9]+' \
  'performance-log-check\.runtime_drop_total=[0-9]+' \
  'performance-log-check\.result=PASS' \
  -- --performance-log-check "${MEDIA_PATH}" "${SAMPLE_MS}"
INDEX=$((INDEX + 1))
run_check "${INDEX}" "${TOTAL}" "cp401_software_decode_audio" "CP-401 Linux software decode + SDL audio" "" \
  'linux-software-audio-check\.(renderer_backend|decoder_backend|active_audio_driver|audio_init_strategy|result)=' \
  'linux-software-audio-check\.platform_ok=true' \
  'linux-software-audio-check\.renderer_backend=SoftwareSDL' \
  'linux-software-audio-check\.decoder_backend=Software' \
  'linux-software-audio-check\.active_audio_driver=.+' \
  'linux-software-audio-check\.result=PASS' \
  -- --linux-software-audio-check "${MEDIA_PATH}" "${SAMPLE_MS}"
INDEX=$((INDEX + 1))
run_check "${INDEX}" "${TOTAL}" "cp402_opengl_playback" "CP-402 Linux OpenGL playback" "" \
  'linux-opengl-playback-check\.(renderer_backend|decoder_backend|render_frames|result)=' \
  'linux-opengl-playback-check\.platform_ok=true' \
  'linux-opengl-playback-check\.renderer_backend=OpenGL' \
  'linux-opengl-playback-check\.result=PASS' \
  -- --linux-opengl-playback-check "${MEDIA_PATH}" "${SAMPLE_MS}"
INDEX=$((INDEX + 1))
run_check "${INDEX}" "${TOTAL}" "cp403_opengl_to_software_fallback" "CP-403 OpenGL -> SoftwareSDL fallback" "" \
  'linux-opengl-fallback-check\.(renderer_backend|startup_renderer_fallback_reason|fallback_to_software|result)=' \
  'linux-opengl-fallback-check\.platform_ok=true' \
  'linux-opengl-fallback-check\.fallback_to_software=true' \
  'linux-opengl-fallback-check\.fallback_reason_recorded=true' \
  'linux-opengl-fallback-check\.result=PASS' \
  -- --linux-opengl-fallback-check "${MEDIA_PATH}" "${SAMPLE_MS}"
INDEX=$((INDEX + 1))
run_check "${INDEX}" "${TOTAL}" "cp703_cp704_vaapi_fallback_copyback" "CP-703/CP-704 Linux VAAPI fallback + copy-back baseline" "" \
  'linux-vaapi-fallback-check\.(decoder_backend|capability_available|copy_back_frames|path_ok|result)=' \
  'linux-vaapi-fallback-check\.platform_ok=true' \
  'linux-vaapi-fallback-check\.capability_probe_completed=true' \
  'linux-vaapi-fallback-check\.runtime_ok=true' \
  'linux-vaapi-fallback-check\.result=PASS' \
  -- --linux-vaapi-fallback-check "${MEDIA_PATH}" "${SAMPLE_MS}"
INDEX=$((INDEX + 1))
run_check "${INDEX}" "${TOTAL}" "cp404_audio_backend_smoke" "CP-404 Linux audio backend smoke" "" \
  'linux-audio-backend-smoke\.(available_target_count|pass_target_count|result)=' \
  'linux-audio-backend-smoke\.available_target_count=[1-9][0-9]*' \
  'linux-audio-backend-smoke\.pass_target_count=[1-9][0-9]*' \
  'linux-audio-backend-smoke\.result=PASS' \
  -- --linux-audio-backend-smoke "${MEDIA_PATH}" "${SAMPLE_MS}"
INDEX=$((INDEX + 1))
run_check "${INDEX}" "${TOTAL}" "cp405_core_playback_behavior" "CP-405 Core playback behavior" "" \
  'core-playback-behavior-check\.(renderer_backend|decoder_backend|progress_ok|runtime_ok|result)=' \
  'core-playback-behavior-check\.progress_ok=true' \
  'core-playback-behavior-check\.runtime_ok=true' \
  'core-playback-behavior-check\.result=PASS' \
  -- --core-playback-behavior-check "${MEDIA_PATH}" "${SAMPLE_MS}"
INDEX=$((INDEX + 1))
run_check "${INDEX}" "${TOTAL}" "cp406_ui_interaction_stability" "CP-406 UI interaction stability" "" \
  'ui-interaction-check\.(renderer_backend|decoder_backend|render_progress_ok|stability_ok|result)=' \
  'ui-interaction-check\.render_progress_ok=true' \
  'ui-interaction-check\.stability_ok=true' \
  'ui-interaction-check\.result=PASS' \
  -- --ui-interaction-check "${MEDIA_PATH}" "${SAMPLE_MS}"

if [[ ${HAS_VK010} -eq 1 ]]; then
  INDEX=$((INDEX + 1))
  run_check "${INDEX}" "${TOTAL}" "vk010_vulkan_diagnostics" "VK-010 Vulkan diagnostics baseline" "" \
    'vulkan-diagnostics\.(supported_platform|compiled_in|runtime_available|startup_renderer_candidates|startup_renderer_plan_reason|selected_renderer|fallback_target|result)=' \
    'vulkan-diagnostics\.supported_platform=true' \
    'vulkan-diagnostics\.compiled_in=true' \
    'vulkan-diagnostics\.runtime_available=true' \
    'vulkan-diagnostics\.startup_renderer_candidates=Vulkan -> OpenGL -> SoftwareSDL' \
    'vulkan-diagnostics\.startup_renderer_plan_reason=.*linux-vulkan-fallback-chain.*' \
    'vulkan-diagnostics\.selected_renderer=Vulkan' \
    'vulkan-diagnostics\.fallback_target=OpenGL' \
    'vulkan-diagnostics\.result=PASS' \
    -- --vulkan-diagnostics
else
  echo
  echo "Skipping VK-010 check: ${VULKAN_SKIP_REASON}"
  write_report_line "check.vk010_vulkan_diagnostics.status=SKIPPED"
  write_report_line "check.vk010_vulkan_diagnostics.reason=$(escape_report_value "${VULKAN_SKIP_REASON}")"
fi

if [[ -f "${ASS_SUBTITLE_PATH}" ]]; then
  INDEX=$((INDEX + 1))
  run_check "${INDEX}" "${TOTAL}" "cp507_libass_shaping_layout_probe" "CP-507 libass shaping/layout probe" "" \
    'libass-shaping-check\.(platform_ok|library_initialized|events_loaded|rendered_output|result)=' \
    'libass-shaping-check\.platform_ok=true' \
    'libass-shaping-check\.library_initialized=true' \
    'libass-shaping-check\.events_loaded=true' \
    'libass-shaping-check\.rendered_output=true' \
    'libass-shaping-check\.result=PASS' \
    -- --libass-shaping-check "${ASS_SUBTITLE_PATH}"
else
  echo
  echo "Skipping CP-507 check: subtitle file not found: ${ASS_SUBTITLE_PATH}"
  write_report_line "check.cp507_libass_shaping_layout_probe.status=SKIPPED"
  write_report_line "check.cp507_libass_shaping_layout_probe.reason=subtitle file not found"
fi

if [[ -f "${EMBEDDED_SUBTITLE_MEDIA_PATH}" ]]; then
  INDEX=$((INDEX + 1))
  run_check "${INDEX}" "${TOTAL}" "cp508_embedded_subtitle_live_packet_probe" "CP-508 embedded subtitle live packet probe" "" \
    'embedded-subtitle-live-packet-check\.(subtitle_stream_found|supported_stream_found|subtitle_packets_read|produced_output|result)=' \
    'embedded-subtitle-live-packet-check\.subtitle_stream_found=true' \
    'embedded-subtitle-live-packet-check\.supported_stream_found=true' \
    'embedded-subtitle-live-packet-check\.subtitle_packets_read=[1-9][0-9]*' \
    'embedded-subtitle-live-packet-check\.produced_output=true' \
    'embedded-subtitle-live-packet-check\.result=PASS' \
    -- --embedded-subtitle-live-packet-check "${EMBEDDED_SUBTITLE_MEDIA_PATH}" -1 "${SUBTITLE_MAX_PACKETS}"
else
  echo
  echo "Skipping CP-508 check: embedded subtitle media not found: ${EMBEDDED_SUBTITLE_MEDIA_PATH}"
  write_report_line "check.cp508_embedded_subtitle_live_packet_probe.status=SKIPPED"
  write_report_line "check.cp508_embedded_subtitle_live_packet_probe.reason=embedded subtitle media not found"
fi

echo
echo "Linux MVP gate result: PASS"
set_gate_result_once "PASS"
