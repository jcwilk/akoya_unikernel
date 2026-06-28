#!/usr/bin/env bash
# Shared inference-endpoint pre-flight for automated verification entry points.
# Aborts with non-zero exit when the configured chat/inference host is unreachable.

inference_preflight() {
    local host="${AKOYA_CHAT_HOST_IP:-192.168.1.110}"
    local port="${AKOYA_CHAT_PORT:-11435}"
    local label="${1:-inference pre-flight}"

    if inference_endpoint_reachable "${host}" "${port}"; then
        return 0
    fi

    echo "${label}: configured inference endpoint ${host}:${port} is unreachable from this workstation." >&2
    echo "${label}: restore inference availability before running automated verification." >&2
    echo "${label}: emulation was not started." >&2
    return 1
}

inference_endpoint_reachable() {
    local host="$1"
    local port="$2"

    if command -v nc >/dev/null 2>&1; then
        nc -z -w 2 "${host}" "${port}" >/dev/null 2>&1
        return $?
    fi
    if command -v timeout >/dev/null 2>&1 && command -v bash >/dev/null 2>&1; then
        timeout 2 bash -c "echo >/dev/tcp/${host}/${port}" >/dev/null 2>&1
        return $?
    fi
    return 1
}
