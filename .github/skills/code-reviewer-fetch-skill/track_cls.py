"""
CL Tracking & Sync Tool

Tracks Gerrit CLs and GitHub PRs, and fetches new comments on demand.

Usage:
  python track_cls.py track <url>          -- Start tracking a CL/PR
  python track_cls.py untrack <id>         -- Stop tracking a CL/PR by its change number
  python track_cls.py list                 -- List all tracked CLs/PRs
  python track_cls.py sync                 -- Sync all tracked CLs/PRs and report new comments
  python track_cls.py sync <id>            -- Sync a specific tracked CL/PR by its change number
"""

import sys
import json
import os
import re
import glob
import urllib.request
import urllib.error
from datetime import datetime, timezone

SKILL_DIR = os.path.dirname(os.path.abspath(__file__))
TRACKING_FILE = os.path.join(SKILL_DIR, "tracked_cls.json")
MODEL_DIR = os.path.join(SKILL_DIR, "model")
MANUAL_DIR = os.path.join(SKILL_DIR, "manual")

# Statuses that indicate a CL/PR is finished and should be auto-untracked.
# Gerrit: MERGED, ABANDONED  |  GitHub: closed, merged
TERMINAL_STATUSES = {"MERGED", "ABANDONED", "closed", "merged"}


# ---------------------------------------------------------------------------
# Persistence helpers
# ---------------------------------------------------------------------------

def load_tracking_data():
    """Load the tracking database from disk."""
    if os.path.exists(TRACKING_FILE):
        with open(TRACKING_FILE, "r", encoding="utf-8") as f:
            return json.load(f)
    return {"tracked": {}}


def save_tracking_data(data):
    """Persist the tracking database to disk."""
    with open(TRACKING_FILE, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)


# ---------------------------------------------------------------------------
# Network helpers (mirrors fetch_reviews.py logic)
# ---------------------------------------------------------------------------

def clean_gerrit_json(raw_data):
    prefix = ")]}'"
    data_str = raw_data.decode("utf-8").strip()
    if data_str.startswith(prefix):
        data_str = data_str[len(prefix):].strip()
    return json.loads(data_str)


def fetch_json(url, headers=None):
    if headers is None:
        headers = {}
    if "User-Agent" not in headers:
        headers["User-Agent"] = "Review-Fetcher-Agent/1.0"

    req = urllib.request.Request(url, headers=headers)
    try:
        with urllib.request.urlopen(req) as response:
            return response.read()
    except urllib.error.HTTPError as e:
        print(f"  [error] HTTP {e.code} fetching {url}")
        return None
    except urllib.error.URLError as e:
        print(f"  [error] {e.reason} fetching {url}")
        return None


# ---------------------------------------------------------------------------
# URL parsing
# ---------------------------------------------------------------------------

def parse_url(url):
    """Return (platform, host, repo, id) or raise ValueError."""
    gerrit = re.search(r"(https://[^/]+)/c/([^/]+)/\+/(\d+)", url)
    if gerrit:
        return "Gerrit", gerrit.group(1), gerrit.group(2), gerrit.group(3)

    github = re.search(r"https://github\.com/([^/]+)/([^/]+)/pull/(\d+)", url)
    if github:
        return "GitHub", "https://github.com", f"{github.group(1)}/{github.group(2)}", github.group(3)

    raise ValueError(f"Unsupported URL: {url}")


# ---------------------------------------------------------------------------
# Gerrit helpers
# ---------------------------------------------------------------------------

def _fetch_gerrit_state(host, change_id):
    """Fetch current detail + comments for a Gerrit CL. Returns (detail, comments) or (None, None)."""
    raw_detail = fetch_json(f"{host}/changes/{change_id}/detail")
    if not raw_detail:
        return None, None
    detail = clean_gerrit_json(raw_detail)

    raw_comments = fetch_json(f"{host}/changes/{change_id}/comments")
    comments = clean_gerrit_json(raw_comments) if raw_comments else {}
    return detail, comments


def _count_gerrit_comments(comments_dict):
    """Total inline comment count across all files."""
    return sum(len(v) for v in comments_dict.values())


def _gerrit_snapshot(detail, comments):
    """Build a compact snapshot dict from Gerrit API responses."""
    messages = detail.get("messages", [])
    seen_messages = [msg.get("id") for msg in messages if msg.get("id")]
    
    seen_inline = []
    for file_comments in comments.values():
        for c in file_comments:
            if c.get("id"):
                seen_inline.append(c["id"])

    return {
        "status": detail.get("status", "UNKNOWN"),
        "subject": detail.get("subject", ""),
        "message_count": len(messages),
        "inline_comment_count": _count_gerrit_comments(comments),
        "latest_message_date": messages[-1].get("date", "").split(".")[0] if messages else "",
        "seen_messages": seen_messages,
        "seen_inline": seen_inline,
    }


# ---------------------------------------------------------------------------
# GitHub helpers
# ---------------------------------------------------------------------------

def _fetch_github_state(repo, pr_number):
    headers = {"Accept": "application/vnd.github.v3+json"}
    gh_token = os.environ.get("GITHUB_TOKEN")
    if gh_token:
        headers["Authorization"] = f"token {gh_token}"

    api_base = f"https://api.github.com/repos/{repo}"
    raw_pr = fetch_json(f"{api_base}/pulls/{pr_number}", headers)
    if not raw_pr:
        return None, None, None
    pr = json.loads(raw_pr)

    raw_comments = fetch_json(f"{api_base}/pulls/{pr_number}/comments", headers)
    inline_comments = json.loads(raw_comments) if raw_comments else []

    raw_issue = fetch_json(f"{api_base}/issues/{pr_number}/comments", headers)
    issue_comments = json.loads(raw_issue) if raw_issue else []

    return pr, inline_comments, issue_comments


def _github_snapshot(pr, inline_comments, issue_comments):
    seen_messages = [str(c.get("id")) for c in issue_comments if c.get("id")]
    seen_inline = [str(c.get("id")) for c in inline_comments if c.get("id")]
    return {
        "status": pr.get("state", "unknown"),
        "subject": pr.get("title", ""),
        "message_count": len(issue_comments),
        "inline_comment_count": len(inline_comments),
        "latest_message_date": issue_comments[-1].get("created_at", "") if issue_comments else "",
        "seen_messages": seen_messages,
        "seen_inline": seen_inline,
    }


# ---------------------------------------------------------------------------
# Sub-commands
# ---------------------------------------------------------------------------

def _find_existing_review_file(change_id):
    """Check if a review JSON file already exists locally for this change ID."""
    pattern = os.path.join(MODEL_DIR, f"Review_{change_id}_*.json")
    matches = glob.glob(pattern)
    return matches[0] if matches else None


def _save_full_review(data):
    """Save full review data to model/ (JSON) and manual/ (MD), mirroring fetch_reviews.py."""
    # Import generate_markdown from the sibling script
    import importlib.util
    spec = importlib.util.spec_from_file_location(
        "fetch_reviews", os.path.join(SKILL_DIR, "fetch_reviews.py"))
    fetch_mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(fetch_mod)

    os.makedirs(MODEL_DIR, exist_ok=True)
    os.makedirs(MANUAL_DIR, exist_ok=True)

    safe_subject = re.sub(r'[^a-zA-Z0-9_\-]', '_', data["metadata"]["title"])[:40]
    filename = f"Review_{data['metadata']['id']}_{safe_subject}"

    model_path = os.path.join(MODEL_DIR, f"{filename}.json")
    with open(model_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)

    manual_path = os.path.join(MANUAL_DIR, f"{filename}.md")
    with open(manual_path, "w", encoding="utf-8") as f:
        f.write(fetch_mod.generate_markdown(data))

    return model_path, manual_path


def _append_review_updates(change_id, full_data, new_messages, new_inline, timestamp):
    """Save full review data to model/ (JSON) and append new comments to manual/ (MD)."""
    os.makedirs(MODEL_DIR, exist_ok=True)
    os.makedirs(MANUAL_DIR, exist_ok=True)

    safe_subject = re.sub(r'[^a-zA-Z0-9_\-]', '_', full_data["metadata"]["title"])[:40]
    filename = f"Review_{change_id}_{safe_subject}"

    # Overwrite JSON with full correct state
    model_path = os.path.join(MODEL_DIR, f"{filename}.json")
    with open(model_path, "w", encoding="utf-8") as f:
        json.dump(full_data, f, indent=2)

    manual_path = os.path.join(MANUAL_DIR, f"{filename}.md")
    
    # If the markdown doesn't exist or we have no new messages/inline, fallback/skip append
    if not os.path.exists(manual_path):
        import importlib.util
        spec = importlib.util.spec_from_file_location(
            "fetch_reviews", os.path.join(SKILL_DIR, "fetch_reviews.py"))
        fetch_mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(fetch_mod)
        with open(manual_path, "w", encoding="utf-8") as f:
            f.write(fetch_mod.generate_markdown(full_data))
        return model_path, manual_path

    if not new_messages and not new_inline:
        # Just update JSON if there were other changes (e.g. status)
        return model_path, manual_path

    # Append to existing markdown with a clear delimiter
    md = []
    md.append(f"\n\n---\n## Sync Update: {timestamp}")
    if new_messages:
        md.append("\n### New General Comments")
        for c in new_messages:
            rnd = f" (Round {c['round']})" if "round" in c else ""
            md.append(f"**@{c['author']}**{rnd} ({c['date']}):")
            indented = "\n".join([f"> {line}" for line in c["message"].split("\n")])
            md.append(indented)
            md.append("")

    if new_inline:
        md.append("\n### New Inline Comments")
        for c in new_inline:
            rnd = f" (Round {c['round']})" if "round" in c else ""
            md.append(f"**[{c.get('file', 'Unknown')}:{c['line']}]{rnd}** - @{c['author']} ({c['date']}):")
            indented = "\n".join([f"> {line}" for line in c["message"].split("\n")])
            md.append(indented)
            md.append("")

    with open(manual_path, "a", encoding="utf-8") as f:
        f.write("\n".join(md))

    return model_path, manual_path


def _build_full_gerrit_data(url, host, repo, change_id, detail, comments):
    """Build the full review data dict (same structure as fetch_reviews.py process_gerrit)."""
    messages = detail.get("messages", [])
    max_round = max([msg.get("_revision_number", 1) for msg in messages]) if messages else 1

    data = {
        "metadata": {
            "platform": "Gerrit",
            "repo": repo,
            "url": url,
            "id": change_id,
            "title": detail.get("subject", "Untitled"),
            "author": detail.get("owner", {}).get("name", "Unknown"),
            "status": detail.get("status", "NEW"),
            "rounds": max_round,
        },
        "general_comments": [],
        "inline_comments": {},
    }

    for msg in messages:
        author = msg.get("author", {}).get("name", "Unknown")
        message_text = msg.get("message", "").strip()
        date = msg.get("date", "").split(".")[0]
        rnd = msg.get("_revision_number", "?")
        if message_text:
            data["general_comments"].append({
                "author": author, "date": date,
                "round": str(rnd), "message": message_text,
            })

    for file_path, file_comments in sorted(comments.items()):
        data["inline_comments"][file_path] = []
        file_comments.sort(key=lambda x: x.get("line", 0))
        for c in file_comments:
            data["inline_comments"][file_path].append({
                "line": str(c.get("line", "File-level")),
                "round": str(c.get("patch_set", "?")),
                "author": c.get("author", {}).get("name", "Unknown"),
                "date": c.get("updated", "").split(".")[0],
                "message": c.get("message", "").strip(),
            })

    return data


def _build_full_github_data(url, repo, pr, inline_comments, issue_comments):
    """Build the full review data dict (same structure as fetch_reviews.py process_github)."""
    owner_repo = repo  # already "owner/repo"
    data = {
        "metadata": {
            "platform": "GitHub",
            "repo": owner_repo,
            "url": url,
            "id": str(pr.get("number", "?")),
            "title": pr.get("title", "Untitled"),
            "author": pr.get("user", {}).get("login", "Unknown"),
            "status": pr.get("state", "Unknown"),
        },
        "general_comments": [],
        "inline_comments": {},
    }

    for c in issue_comments:
        data["general_comments"].append({
            "author": c.get("user", {}).get("login", "Unknown"),
            "date": c.get("created_at", "").replace("T", " ").replace("Z", ""),
            "message": c.get("body", "").strip(),
        })

    for c in inline_comments:
        path = c.get("path", "Unknown")
        if path not in data["inline_comments"]:
            data["inline_comments"][path] = []
        data["inline_comments"][path].append({
            "line": str(c.get("line") or c.get("original_line") or "outdated"),
            "author": c.get("user", {}).get("login", "Unknown"),
            "date": c.get("created_at", "").replace("T", " ").replace("Z", ""),
            "message": c.get("body", "").strip(),
        })

    return data


def cmd_track(url):
    """Add a CL/PR to the tracking list."""
    platform, host, repo, change_id = parse_url(url)
    data = load_tracking_data()

    if change_id in data["tracked"]:
        print(f"CL/PR {change_id} is already being tracked.")
        return

    now = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    # 1. Check if we already have a local review file from a previous fetch
    existing = _find_existing_review_file(change_id)
    if existing:
        print(f"Found existing local review file: {existing}")
        with open(existing, "r", encoding="utf-8") as f:
            review_data = json.load(f)
        # Build baseline snapshot from the existing file
        meta = review_data.get("metadata", {})
        gc = review_data.get("general_comments", [])
        ic = review_data.get("inline_comments", {})
        snapshot = {
            "status": meta.get("status", "UNKNOWN"),
            "subject": meta.get("title", ""),
            "message_count": len(gc),
            "inline_comment_count": sum(len(v) for v in ic.values()),
            "latest_message_date": gc[-1].get("date", "") if gc else "",
        }
        print(f"  Loaded baseline from existing file ({snapshot['message_count']} general, "
              f"{snapshot['inline_comment_count']} inline comments).")
    else:
        # 2. Fetch full review data from the API and save locally
        print(f"Fetching full review data for {platform} {change_id}...")
        if platform == "Gerrit":
            detail, comments = _fetch_gerrit_state(host, change_id)
            if not detail:
                print("Failed to fetch CL state. Tracking entry saved without baseline.")
                snapshot = {}
            else:
                snapshot = _gerrit_snapshot(detail, comments)
                review_data = _build_full_gerrit_data(url, host, repo, change_id, detail, comments)
                model_path, manual_path = _save_full_review(review_data)
                print(f"  Saved review data:")
                print(f"    Model JSON : {model_path}")
                print(f"    Manual MD  : {manual_path}")
        else:
            pr, inline_comments, issue_comments = _fetch_github_state(repo, change_id)
            if not pr:
                print("Failed to fetch PR state. Tracking entry saved without baseline.")
                snapshot = {}
            else:
                snapshot = _github_snapshot(pr, inline_comments, issue_comments)
                review_data = _build_full_github_data(url, repo, pr, inline_comments, issue_comments)
                model_path, manual_path = _save_full_review(review_data)
                print(f"  Saved review data:")
                print(f"    Model JSON : {model_path}")
                print(f"    Manual MD  : {manual_path}")

    entry = {
        "url": url,
        "platform": platform,
        "host": host,
        "repo": repo,
        "title": snapshot.get("subject", "Unknown"),
        "added_at": now,
        "last_synced": now,
        "baseline": snapshot,
    }

    data["tracked"][change_id] = entry
    save_tracking_data(data)
    print(f"Now tracking {platform} {change_id}: {entry['title']}")


def cmd_untrack(change_id):
    """Remove a CL/PR from the tracking list."""
    data = load_tracking_data()
    if change_id not in data["tracked"]:
        print(f"CL/PR {change_id} is not currently tracked.")
        return
    title = data["tracked"][change_id].get("title", change_id)
    del data["tracked"][change_id]
    save_tracking_data(data)
    print(f"Stopped tracking {change_id}: {title}")


def cmd_list():
    """Print all tracked CLs/PRs."""
    data = load_tracking_data()
    tracked = data.get("tracked", {})
    if not tracked:
        print("No CLs/PRs are currently being tracked.")
        return

    print(f"{'ID':<10} {'Platform':<8} {'Status':<12} {'Last Synced':<22} Title")
    print("-" * 90)
    for cid, entry in tracked.items():
        status = entry.get("baseline", {}).get("status", "?")
        synced = entry.get("last_synced", "?")
        title = entry.get("title", "?")[:50]
        plat = entry.get("platform", "?")
        print(f"{cid:<10} {plat:<8} {status:<12} {synced:<22} {title}")


def cmd_sync(target_id=None):
    """
    Sync tracked CLs/PRs. For each one, fetch current state and diff against
    the stored baseline. Report new comments/messages, status changes, etc.
    Then update the baseline.

    If target_id is given, only sync that one CL/PR.
    """
    data = load_tracking_data()
    tracked = data.get("tracked", {})
    if not tracked:
        print("No CLs/PRs are currently being tracked.")
        return

    ids_to_sync = [target_id] if target_id else list(tracked.keys())
    missing = [i for i in ids_to_sync if i not in tracked]
    if missing:
        print(f"Not tracked: {', '.join(missing)}")
        return

    now = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    sync_report = []  # list of dicts for JSON output

    for cid in ids_to_sync:
        entry = tracked[cid]
        platform = entry["platform"]
        host = entry["host"]
        repo = entry["repo"]
        url = entry["url"]
        old = entry.get("baseline", {})

        print(f"\n--- Syncing {platform} {cid}: {entry.get('title', '')} ---")

        if platform == "Gerrit":
            detail, comments = _fetch_gerrit_state(host, cid)
            if not detail:
                print(f"  [skip] Could not fetch CL {cid}.")
                continue
            new = _gerrit_snapshot(detail, comments)
            new_messages = _extract_new_gerrit_messages(detail, old)
            new_inline = _extract_new_gerrit_inline(comments, old)
        else:
            pr, inline_comments, issue_comments = _fetch_github_state(repo, cid)
            if not pr:
                print(f"  [skip] Could not fetch PR {cid}.")
                continue
            new = _github_snapshot(pr, inline_comments, issue_comments)
            new_messages = _extract_new_github_messages(issue_comments, old)
            new_inline = _extract_new_github_inline(inline_comments, old)

        # Diff
        changes = []
        if old.get("status") != new.get("status"):
            changes.append(f"Status changed: {old.get('status', '?')} -> {new['status']}")
        new_msg_count = new["message_count"] - old.get("message_count", 0)
        new_inl_count = new["inline_comment_count"] - old.get("inline_comment_count", 0)
        if new_msg_count > 0:
            changes.append(f"{new_msg_count} new general comment(s)")
        if new_inl_count > 0:
            changes.append(f"{new_inl_count} new inline comment(s)")

        if changes:
            print(f"  Updating main review file...")
            if platform == "Gerrit":
                full_data = _build_full_gerrit_data(url, host, repo, cid, detail, comments)
            else:
                full_data = _build_full_github_data(url, repo, pr, inline_comments, issue_comments)
            _append_review_updates(cid, full_data, new_messages, new_inline, now)

        if not changes:
            print("  No changes since last sync.")
        else:
            for ch in changes:
                print(f"  * {ch}")

        # Build per-CL report
        cl_report = {
            "id": cid,
            "url": url,
            "platform": platform,
            "title": new.get("subject", entry.get("title", "")),
            "previous_status": old.get("status", "?"),
            "current_status": new["status"],
            "new_general_comment_count": max(new_msg_count, 0),
            "new_inline_comment_count": max(new_inl_count, 0),
            "changes_summary": changes,
            "new_general_comments": new_messages,
            "new_inline_comments": new_inline,
        }
        sync_report.append(cl_report)

        # Update baseline
        entry["baseline"] = new
        entry["title"] = new.get("subject", entry.get("title", ""))
        entry["last_synced"] = now

    # Auto-untrack CLs/PRs that have reached a terminal status
    auto_untracked = []
    for rpt in sync_report:
        cid = rpt["id"]
        current = rpt["current_status"]
        if current in TERMINAL_STATUSES and cid in tracked:
            reason = (f"Auto-untracked {rpt['platform']} {cid} "
                      f"(\"{rpt['title']}\") because its status is "
                      f"{current}. No further updates are expected.")
            print(f"  >> {reason}")
            rpt["auto_untracked"] = True
            rpt["auto_untrack_reason"] = reason
            auto_untracked.append(cid)

    for cid in auto_untracked:
        del tracked[cid]

    save_tracking_data(data)

    # Write sync report to model/ and manual/ for agent/human consumption
    if sync_report:
        _write_sync_report(sync_report, now)


# ---------------------------------------------------------------------------
# Extract *new* comments/messages since last baseline
# ---------------------------------------------------------------------------

def _extract_new_gerrit_messages(detail, old_baseline):
    """Return general messages that are new since the old baseline."""
    messages = detail.get("messages", [])
    seen_messages = set(old_baseline.get("seen_messages", []))
    
    result = []
    for msg in messages:
        msg_id = msg.get("id")
        # Fallback to count logic if old baseline doesn't have IDs
        if not seen_messages and result == [] and old_baseline.get("message_count", 0) > 0:
             # Legacy logic fallback happens outside this loop since it's hard to mix
             pass
             
        if msg_id and msg_id not in seen_messages:
            result.append({
                "author": msg.get("author", {}).get("name", "Unknown"),
                "date": msg.get("date", "").split(".")[0],
                "round": str(msg.get("_revision_number", "?")),
                "message": msg.get("message", "").strip(),
            })
            
    # Legacy fallback
    if not seen_messages and result == []:
        old_count = old_baseline.get("message_count", 0)
        new_msgs = messages[old_count:]
        for msg in new_msgs:
            result.append({
                "author": msg.get("author", {}).get("name", "Unknown"),
                "date": msg.get("date", "").split(".")[0],
                "round": str(msg.get("_revision_number", "?")),
                "message": msg.get("message", "").strip(),
            })
            
    return result


def _extract_new_gerrit_inline(comments_dict, old_baseline):
    """Return inline comments that are new since the old baseline."""
    seen_inline = set(old_baseline.get("seen_inline", []))
    
    all_inline = []
    for file_path, file_comments in sorted(comments_dict.items()):
        for c in file_comments:
            c_id = c.get("id")
            if seen_inline and c_id in seen_inline:
                continue
                
            all_inline.append({
                "id": c_id,
                "file": file_path,
                "line": str(c.get("line", "File-level")),
                "round": str(c.get("patch_set", "?")),
                "author": c.get("author", {}).get("name", "Unknown"),
                "date": c.get("updated", "").split(".")[0],
                "message": c.get("message", "").strip(),
            })
            
    # Sort by date ascending so newest are at the end
    all_inline.sort(key=lambda x: x["date"])
    
    # Legacy fallback
    if not seen_inline:
        old_count = old_baseline.get("inline_comment_count", 0)
        new_count = len(all_inline) - old_count
        if new_count <= 0:
            return []
        return all_inline[-new_count:]
        
    return all_inline


def _extract_new_github_messages(issue_comments, old_baseline):
    seen_messages = set(old_baseline.get("seen_messages", []))
    
    result = []
    for c in issue_comments:
        c_id = str(c.get("id"))
        if seen_messages and c_id in seen_messages:
            continue
            
        result.append({
            "author": c.get("user", {}).get("login", "Unknown"),
            "date": c.get("created_at", "").replace("T", " ").replace("Z", ""),
            "message": c.get("body", "").strip(),
        })
        
    if not seen_messages and result == []:
        old_count = old_baseline.get("message_count", 0)
        new_comments = issue_comments[old_count:]
        for c in new_comments:
            result.append({
                "author": c.get("user", {}).get("login", "Unknown"),
                "date": c.get("created_at", "").replace("T", " ").replace("Z", ""),
                "message": c.get("body", "").strip(),
            })
    return result


def _extract_new_github_inline(inline_comments, old_baseline):
    seen_inline = set(old_baseline.get("seen_inline", []))
    inline_comments_sorted = sorted(inline_comments, key=lambda c: c.get("created_at", ""))
    
    result = []
    for c in inline_comments_sorted:
        c_id = str(c.get("id"))
        if seen_inline and c_id in seen_inline:
            continue
            
        result.append({
            "file": c.get("path", "Unknown"),
            "line": str(c.get("line") or c.get("original_line") or "outdated"),
            "author": c.get("user", {}).get("login", "Unknown"),
            "date": c.get("created_at", "").replace("T", " ").replace("Z", ""),
            "message": c.get("body", "").strip(),
        })
        
    if not seen_inline and result == []:
        old_count = old_baseline.get("inline_comment_count", 0)
        new_comments = inline_comments_sorted[old_count:]
        for c in new_comments:
            result.append({
                "file": c.get("path", "Unknown"),
                "line": str(c.get("line") or c.get("original_line") or "outdated"),
                "author": c.get("user", {}).get("login", "Unknown"),
                "date": c.get("created_at", "").replace("T", " ").replace("Z", ""),
                "message": c.get("body", "").strip(),
            })
    return result


# ---------------------------------------------------------------------------
# Sync report output
# ---------------------------------------------------------------------------

def _write_sync_report(sync_report, timestamp):
    """Write a human-friendly Markdown sync report to manual/sync_reports/."""
    sync_dir = os.path.join(MANUAL_DIR, "sync_reports")
    os.makedirs(sync_dir, exist_ok=True)

    safe_ts = timestamp.replace(":", "-").replace("T", "_").rstrip("Z")
    base_name = f"SyncReport_{safe_ts}"

    # --- Markdown for human consumption ---
    md = []
    md.append(f"# CL/PR Sync Report")
    md.append(f"**Synced at**: {timestamp}\n")

    for rpt in sync_report:
        md.append(f"## [{rpt['platform']}] {rpt['id']}: {rpt['title']}")
        md.append(f"- **URL**: {rpt['url']}")
        md.append(f"- **Status**: {rpt['previous_status']} -> {rpt['current_status']}")
        md.append(f"- **New general comments**: {rpt['new_general_comment_count']}")
        md.append(f"- **New inline comments**: {rpt['new_inline_comment_count']}")

        if rpt["changes_summary"]:
            md.append("\n### Changes")
            for ch in rpt["changes_summary"]:
                md.append(f"- {ch}")

        if rpt["new_general_comments"]:
            md.append("\n### New General Comments")
            for c in rpt["new_general_comments"]:
                rnd = f" (Round {c['round']})" if "round" in c else ""
                md.append(f"**@{c['author']}**{rnd} ({c['date']}):")
                indented = "\n".join([f"> {line}" for line in c["message"].split("\n")])
                md.append(indented)
                md.append("")

        if rpt["new_inline_comments"]:
            md.append("\n### New Inline Comments")
            for c in rpt["new_inline_comments"]:
                rnd = f" (Round {c['round']})" if "round" in c else ""
                md.append(f"**[{c['file']}:{c['line']}]{rnd}** - @{c['author']} ({c['date']}):")
                indented = "\n".join([f"> {line}" for line in c["message"].split("\n")])
                md.append(indented)
                md.append("")

        if rpt.get("auto_untracked"):
            md.append(f"\n> **Auto-untracked**: {rpt['auto_untrack_reason']}")

        if not rpt["changes_summary"] and not rpt.get("auto_untracked"):
            md.append("\n*No changes since last sync.*")
        md.append("")

    report_path = os.path.join(sync_dir, f"{base_name}.md")
    with open(report_path, "w", encoding="utf-8") as f:
        f.write("\n".join(md))

    print(f"\nSync report written:")
    print(f"  {report_path}")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    command = sys.argv[1].lower()

    if command == "track":
        if len(sys.argv) < 3:
            print("Usage: python track_cls.py track <url>")
            sys.exit(1)
        cmd_track(sys.argv[2])

    elif command == "untrack":
        if len(sys.argv) < 3:
            print("Usage: python track_cls.py untrack <change_id>")
            sys.exit(1)
        cmd_untrack(sys.argv[2])

    elif command == "list":
        cmd_list()

    elif command == "sync":
        target = sys.argv[2] if len(sys.argv) >= 3 else None
        cmd_sync(target)

    else:
        print(f"Unknown command: {command}")
        print(__doc__)
        sys.exit(1)


if __name__ == "__main__":
    main()
