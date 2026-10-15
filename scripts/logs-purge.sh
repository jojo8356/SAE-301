#!/usr/bin/env bash
# Remove regular log files older than the configured retention window.
# SPDX-License-Identifier: MIT
set -euo pipefail

usage() {
    cat <<'USAGE'
Usage: logs-purge.sh [--dry-run] [--dir DIRECTORY] [--days DAYS]

Options:
  --dry-run       List expired files without deleting them.
  --dir DIR       Log directory (default: LOG_DIR or ./logs).
  --days DAYS     Retention in whole days (default: LOG_RETENTION_DAYS or 180).
  -h, --help      Show this help.

Only regular files directly inside the selected directory whose names match
*.log* are considered. Directories and symbolic links are never followed. The
filesystem root and current working directory are refused as targets.
GNU find (Debian/Linux) is required for the -mmin age test.
USAGE
}

log_dir=${LOG_DIR:-logs}
retention_days=${LOG_RETENTION_DAYS:-180}
dry_run=0
dir_was_supplied=0

while (($# > 0)); do
    case "$1" in
        --dry-run)
            dry_run=1
            shift
            ;;
        --dir)
            if (($# < 2)) || [[ -z "$2" ]]; then
                printf 'logs-purge.sh: --dir requires a non-empty path\n' >&2
                exit 2
            fi
            log_dir=$2
            dir_was_supplied=1
            shift 2
            ;;
        --days)
            if (($# < 2)) || [[ -z "$2" ]]; then
                printf 'logs-purge.sh: --days requires a positive integer\n' >&2
                exit 2
            fi
            retention_days=$2
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        --)
            shift
            if (($# != 0)); then
                printf 'logs-purge.sh: unexpected positional arguments\n' >&2
                usage >&2
                exit 2
            fi
            ;;
        *)
            printf 'logs-purge.sh: unknown option: %s\n' "$1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

# Keep arithmetic bounded and avoid Bash's octal interpretation of leading zeroes.
if [[ ! "$retention_days" =~ ^[1-9][0-9]{0,4}$ ]] ||
   ((10#$retention_days > 36500)); then
    printf 'logs-purge.sh: --days must be between 1 and 36500\n' >&2
    exit 2
fi
if [[ -z "$log_dir" || "$log_dir" == / || "$log_dir" == . || "$log_dir" == .. ]]; then
    printf 'logs-purge.sh: refusing unsafe log directory: %s\n' "$log_dir" >&2
    exit 2
fi

if [[ ! -d "$log_dir" ]]; then
    if ((dir_was_supplied)); then
        printf 'logs-purge.sh: log directory does not exist: %s\n' "$log_dir" >&2
        exit 1
    fi
    printf 'logs-purge.sh: no log directory at %s; nothing to purge\n' "$log_dir"
    exit 0
fi

working_directory=$(pwd -P)
resolved_log_dir=$(CDPATH= cd -- "$log_dir" && pwd -P) || {
    printf 'logs-purge.sh: cannot resolve log directory: %s\n' "$log_dir" >&2
    exit 1
}
if [[ "$resolved_log_dir" == / || "$resolved_log_dir" == "$working_directory" ]]; then
    printf 'logs-purge.sh: refusing filesystem root or current working directory: %s\n' \
        "$resolved_log_dir" >&2
    exit 2
fi
log_dir=$resolved_log_dir

age_minutes=$((10#$retention_days * 24 * 60))
tmpfile=$(mktemp "${TMPDIR:-/tmp}/animetcg-logs-purge.XXXXXX") || {
    printf 'logs-purge.sh: could not create a temporary file\n' >&2
    exit 1
}
trap 'rm -f -- "$tmpfile"' EXIT

if ! find "$log_dir" -maxdepth 1 -type f -name '*.log*' \
        -mmin "+$age_minutes" -print0 >"$tmpfile"; then
    printf 'logs-purge.sh: failed while scanning %s\n' "$log_dir" >&2
    exit 1
fi

count=0
while IFS= read -r -d '' file; do
    if ((dry_run)); then
        printf 'would remove: %q\n' "$file"
    else
        if ! rm -- "$file"; then
            printf 'logs-purge.sh: failed to remove %q\n' "$file" >&2
            exit 1
        fi
        printf 'removed: %q\n' "$file"
    fi
    ((count += 1))
done <"$tmpfile"

if ((count == 0)); then
    printf 'logs-purge.sh: no log files older than %s days in %s\n' \
        "$retention_days" "$log_dir"
elif ((dry_run)); then
    printf 'logs-purge.sh: %s expired log file(s) listed; no files deleted\n' "$count"
else
    printf 'logs-purge.sh: %s expired log file(s) removed\n' "$count"
fi
