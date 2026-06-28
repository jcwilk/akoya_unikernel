#!/usr/bin/env bash
# Line-oriented chat script parser and assertion engine for headless QEMU runs.
# Script format (*.akoya-script):
#   # comment
#   delay MS
#   type "literal text"
#   expect "substring or /regex/"
#   expect_line reply
#   expect_not "pattern"

chat_script_parse_file() {
    local script_file="$1"
    CHAT_SCRIPT_STEPS=()
    CHAT_SCRIPT_STEP_KINDS=()
    CHAT_SCRIPT_STEP_ARGS=()

    local line trimmed kind arg
    while IFS= read -r line || [[ -n "${line}" ]]; do
        trimmed="${line#"${line%%[![:space:]]*}"}"
        trimmed="${trimmed%"${trimmed##*[![:space:]]}"}"
        [[ -z "${trimmed}" ]] && continue
        [[ "${trimmed}" == \#* ]] && continue

        kind="${trimmed%% *}"
        arg="${trimmed#"${kind}"}"
        arg="${arg#"${arg%%[![:space:]]*}"}"

        case "${kind}" in
            delay)
                CHAT_SCRIPT_STEPS+=("delay")
                CHAT_SCRIPT_STEP_KINDS+=("delay")
                CHAT_SCRIPT_STEP_ARGS+=("${arg}")
                ;;
            type)
                arg="${arg#\"}"
                arg="${arg%\"}"
                CHAT_SCRIPT_STEPS+=("type")
                CHAT_SCRIPT_STEP_KINDS+=("type")
                CHAT_SCRIPT_STEP_ARGS+=("${arg}")
                ;;
            expect)
                arg="${arg#\"}"
                arg="${arg%\"}"
                CHAT_SCRIPT_STEPS+=("expect")
                CHAT_SCRIPT_STEP_KINDS+=("expect")
                CHAT_SCRIPT_STEP_ARGS+=("${arg}")
                ;;
            expect_not)
                arg="${arg#\"}"
                arg="${arg%\"}"
                CHAT_SCRIPT_STEPS+=("expect_not")
                CHAT_SCRIPT_STEP_KINDS+=("expect_not")
                CHAT_SCRIPT_STEP_ARGS+=("${arg}")
                ;;
            expect_line)
                CHAT_SCRIPT_STEPS+=("expect_line")
                CHAT_SCRIPT_STEP_KINDS+=("expect_line")
                CHAT_SCRIPT_STEP_ARGS+=("${arg}")
                ;;
            *)
                echo "chat-script-harness: unsupported script directive '${kind}' in ${script_file}" >&2
                return 1
                ;;
        esac
    done < "${script_file}"
}

chat_script_assert_pattern() {
    local pattern="$1"
    local log_file="$2"

    if [[ "${pattern}" == /* && "${pattern}" == */ ]]; then
        local regex="${pattern:1:${#pattern}-2}"
        grep -Ev '^> ' "${log_file}" | grep -Eq "${regex}"
        return $?
    fi

    grep -Ev '^> ' "${log_file}" | grep -Fiq "${pattern}"
}

chat_script_snippet() {
    local log_file="$1"
    tail -n 8 "${log_file}" 2>/dev/null | sed 's/^/  /'
}

chat_script_fail_step() {
    local step_idx="$1"
    local message="$2"
    local log_file="$3"

    echo "chat-script-harness: step ${step_idx} failed: ${message}" >&2
    echo "chat-script-harness: recent log snippet:" >&2
    chat_script_snippet "${log_file}" >&2
    return 1
}

chat_script_assert_expect() {
    local step_idx="$1"
    local pattern="$2"
    local log_file="$3"
    local max_wait="${4:-240}"

    local waited=0
    while [[ ${waited} -lt ${max_wait} ]]; do
        if chat_script_assert_pattern "${pattern}" "${log_file}"; then
            return 0
        fi
        sleep 0.5
        waited=$((waited + 1))
    done

    chat_script_fail_step "${step_idx}" "expected pattern not found: ${pattern}" "${log_file}"
}

chat_script_assert_expect_after_line() {
    local step_idx="$1"
    local pattern="$2"
    local log_file="$3"
    local after_line="$4"
    local max_wait="${5:-240}"
    local waited=0

    while [[ ${waited} -lt ${max_wait} ]]; do
        if tail -n +"$((after_line + 1))" "${log_file}" | grep -Ev '^> ' | grep -Fiq "${pattern}"; then
            return 0
        fi
        sleep 0.5
        waited=$((waited + 1))
    done

    chat_script_fail_step "${step_idx}" "expected pattern not found after typing: ${pattern}" "${log_file}"
}

chat_script_assert_expect_not() {
    local step_idx="$1"
    local pattern="$2"
    local log_file="$3"

    if chat_script_assert_pattern "${pattern}" "${log_file}"; then
        chat_script_fail_step "${step_idx}" "unexpected pattern present: ${pattern}" "${log_file}"
        return 1
    fi
    return 0
}

chat_script_count_reply_lines() {
    local log_file="$1"
    local host="${AKOYA_CHAT_HOST_IP:-192.168.1.110}"

    awk -v host="${host}" '
        $0 ~ ("^" host " reachable$") { found = 1; next }
        !found { next }
        /^> / { next }
        /^chat_session=/ { next }
        /^chat failed:/ { next }
        /^net_/ { next }
        /^run-qemu\.sh:/ { next }
        /^Running QEMU/ { next }
        /^Boot image:/ { next }
        /^LAN:/ { next }
        /^QEMU smoke test/ { next }
        /^chat-script-harness:/ { next }
        /^$/ { next }
        { count++ }
        END { print count + 0 }
    ' "${log_file}"
}

chat_script_assert_expect_line_reply() {
    local step_idx="$1"
    local log_file="$2"
    local min_count="${3:-1}"
    local max_wait="${4:-240}"
    local waited=0

    while [[ ${waited} -lt ${max_wait} ]]; do
        if [[ "$(chat_script_count_reply_lines "${log_file}")" -ge "${min_count}" ]]; then
            return 0
        fi
        sleep 0.5
        waited=$((waited + 1))
    done

    chat_script_fail_step "${step_idx}" "expected plain assistant reply line not found (need ${min_count})" "${log_file}"
}

chat_script_text_to_sendkeys() {
    local text="$1"
    local -a keys=()
    local i ch lower

    for ((i = 0; i < ${#text}; i++)); do
        ch="${text:i:1}"
        case "${ch}" in
            $'\n'|'\\')
                keys+=("ret")
                ;;
            ' ')
                keys+=("spc")
                ;;
            '/')
                keys+=("slash")
                ;;
            '"')
                keys+=("apostrophe")
                ;;
            *)
                lower="$(printf '%s' "${ch}" | tr '[:upper:]' '[:lower:]')"
                if [[ "${lower}" =~ ^[a-z0-9]$ ]]; then
                    keys+=("${lower}")
                else
                    echo "chat-script-harness: unsupported character in type text: ${ch}" >&2
                    return 1
                fi
                ;;
        esac
    done
    keys+=("ret")
    printf '%s\n' "${keys[@]}"
}

chat_script_run_assertions() {
    local log_file="$1"
    local step_idx=0

    local i kind arg
    for i in "${!CHAT_SCRIPT_STEP_KINDS[@]}"; do
        kind="${CHAT_SCRIPT_STEP_KINDS[$i]}"
        arg="${CHAT_SCRIPT_STEP_ARGS[$i]}"
        step_idx=$((step_idx + 1))

        case "${kind}" in
            expect)
                chat_script_assert_expect "${step_idx}" "${arg}" "${log_file}" || return 1
                ;;
            expect_not)
                chat_script_assert_expect_not "${step_idx}" "${arg}" "${log_file}" || return 1
                ;;
            expect_line)
                if [[ "${arg}" == "reply" ]]; then
                    chat_script_assert_expect_line_reply "${step_idx}" "${log_file}" 1 || return 1
                else
                    chat_script_fail_step "${step_idx}" "unsupported expect_line variant: ${arg}" "${log_file}"
                    return 1
                fi
                ;;
        esac
    done
    return 0
}

chat_script_inject_type() {
    local text="$1"
    local send_monitor_command_fn="$2"
    local log_file="$3"
    local -a keys=()

    mapfile -t keys < <(chat_script_text_to_sendkeys "${text}")

    local key
    for key in "${keys[@]}"; do
        "${send_monitor_command_fn}" "sendkey ${key}" || return 1
        if [[ "${key}" == "ret" ]]; then
            local start_lines
            start_lines="$(wc -l < "${log_file}")"
            local turn_waited=0
            while [[ ${turn_waited} -lt 240 ]]; do
                if grep -Fq "chat_session=exit" "${log_file}"; then
                    break
                fi
                local now_lines
                now_lines="$(wc -l < "${log_file}")"
                if [[ ${now_lines} -gt ${start_lines} ]]; then
                    sleep 0.5
                    break
                fi
                sleep 0.25
                turn_waited=$((turn_waited + 1))
            done
            sleep 0.5
        else
            sleep 0.05
        fi
    done
    return 0
}

chat_script_execute_interactive_steps() {
    local log_file="$1"
    local send_monitor_command_fn="$2"
    local step_idx=0
    local waited=0
    local chat_turns_submitted=0
    CHAT_SCRIPT_LAST_TYPE_LINE=0

    local i kind arg
    for i in "${!CHAT_SCRIPT_STEP_KINDS[@]}"; do
        kind="${CHAT_SCRIPT_STEP_KINDS[$i]}"
        arg="${CHAT_SCRIPT_STEP_ARGS[$i]}"
        step_idx=$((step_idx + 1))

        case "${kind}" in
            delay)
                sleep "$(awk "BEGIN { printf \"%.3f\", ${arg}/1000 }")"
                ;;
            expect|expect_not|expect_line)
                case "${kind}" in
                    expect)
                        if [[ "${CHAT_SCRIPT_LAST_TYPE_LINE}" -gt 0 ]]; then
                            chat_script_assert_expect_after_line "${step_idx}" "${arg}" "${log_file}" "${CHAT_SCRIPT_LAST_TYPE_LINE}" || return 1
                            CHAT_SCRIPT_LAST_TYPE_LINE=0
                        else
                            chat_script_assert_expect "${step_idx}" "${arg}" "${log_file}" || return 1
                        fi
                        ;;
                    expect_not)
                        chat_script_assert_expect_not "${step_idx}" "${arg}" "${log_file}" || return 1
                        ;;
                    expect_line)
                        if [[ "${arg}" == "reply" ]]; then
                            chat_script_assert_expect_line_reply "${step_idx}" "${log_file}" "${chat_turns_submitted}" || return 1
                            CHAT_SCRIPT_LAST_TYPE_LINE=0
                        else
                            chat_script_fail_step "${step_idx}" "unsupported expect_line variant: ${arg}" "${log_file}"
                            return 1
                        fi
                        ;;
                esac
                ;;
            type)
                waited=0
                while [[ ${waited} -lt 600 ]]; do
                    if tail -n 1 "${log_file}" | grep -Eq '^> $|^> [^[:space:]]'; then
                        break
                    fi
                    sleep 0.5
                    waited=$((waited + 1))
                done
                if ! tail -n 1 "${log_file}" | grep -Eq '^> $|^> [^[:space:]]'; then
                    chat_script_fail_step "${step_idx}" "chat prompt not observed before typing" "${log_file}"
                    return 1
                fi
                local log_lines_before
                log_lines_before="$(wc -l < "${log_file}")"
                if ! chat_script_inject_type "${arg}" "${send_monitor_command_fn}" "${log_file}"; then
                    return 1
                fi
                CHAT_SCRIPT_LAST_TYPE_LINE="${log_lines_before}"
                if [[ "${arg}" != "quit" && "${arg}" != "exit" ]]; then
                    chat_turns_submitted=$((chat_turns_submitted + 1))
                fi
                ;;
        esac
    done

    if grep -q '^chat failed:' "${log_file}"; then
        chat_script_fail_step "${step_idx}" "connection failure line detected during script" "${log_file}"
        return 1
    fi

    return 0
}

chat_script_wait_for_reachability() {
    local log_file="$1"
    local chat_host="${AKOYA_CHAT_HOST_IP:-192.168.1.110}"
    local waited=0

    while ! grep -Fq "${chat_host} reachable" "${log_file}" && [[ ${waited} -lt 600 ]]; do
        sleep 0.5
        waited=$((waited + 1))
    done

    if ! grep -Fq "${chat_host} reachable" "${log_file}"; then
        echo "chat-script-harness: reachability line not observed before script steps" >&2
        chat_script_snippet "${log_file}" >&2
        return 1
    fi
    return 0
}
