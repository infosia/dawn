import sys
import json
import urllib.request
import urllib.error
import os
import re

def clean_gerrit_json(raw_data):
    """Gerrit REST API prefixes JSON responses with )]}' to prevent CSRF. Strip it."""
    prefix = ")]}'"
    data_str = raw_data.decode('utf-8').strip()
    if data_str.startswith(prefix):
        data_str = data_str[len(prefix):].strip()
    return json.loads(data_str)

def fetch_json(url, headers=None):
    if headers is None:
        headers = {}
    
    # Generic User-Agent to avoid API blocking
    if 'User-Agent' not in headers:
            headers['User-Agent'] = 'Review-Fetcher-Agent/1.0'

    print(f"Fetching {url}...")
    req = urllib.request.Request(url, headers=headers)
    try:
        with urllib.request.urlopen(req) as response:
            return response.read()
    except urllib.error.HTTPError as e:
        print(f"Failed to fetch {url}: HTTP {e.code}")
        if e.code in (401, 403) and "api.github.com" in url:
            print("Note: GitHub API might be rate-limiting you. Provide a GITHUB_TOKEN environment variable.")
        return None
    except urllib.error.URLError as e:
        print(f"Failed to fetch {url}: {e.reason}")
        return None

def process_gerrit(url):
    match = re.search(r'(https://[^/]+)/c/([^/]+)/\+/(\d+)', url)
    if not match:
        raise ValueError("Invalid Gerrit URL format.")
        
    host, repo, change_number = match.group(1), match.group(2), match.group(3)
    
    raw_detail = fetch_json(f"{host}/changes/{change_number}/detail")
    if not raw_detail: return None
    detail = clean_gerrit_json(raw_detail)
    
    raw_comments = fetch_json(f"{host}/changes/{change_number}/comments")
    comments = clean_gerrit_json(raw_comments) if raw_comments else {}

    messages = detail.get("messages", [])
    max_round = max([msg.get("_revision_number", 1) for msg in messages]) if messages else 1

    data = {
        "metadata": {
            "platform": "Gerrit",
            "repo": repo,
            "url": url,
            "id": change_number,
            "title": detail.get('subject', 'Untitled'),
            "author": detail.get('owner', {}).get('name', 'Unknown'),
            "status": detail.get('status', 'NEW'),
            "rounds": max_round
        },
        "general_comments": [],
        "inline_comments": {}
    }

    # Extract general comments/messages (LGTMs, CI results, etc.)
    for msg in messages:
        author = msg.get("author", {}).get("name", "Unknown")
        message_text = msg.get("message", "").strip()
        date = msg.get("date", "").split(".")[0]
        rnd = msg.get("_revision_number", "?")
        
        # Include if message is somewhat meaningful
        if message_text:
            data["general_comments"].append({
                "author": author,
                "date": date,
                "round": str(rnd),
                "message": message_text
            })

    # Extract inline code comments
    for file_path, file_comments in sorted(comments.items()):
        data["inline_comments"][file_path] = []
        file_comments.sort(key=lambda x: x.get("line", 0))
        for c in file_comments:
            data["inline_comments"][file_path].append({
                "line": str(c.get("line", "File-level")),
                "round": str(c.get("patch_set", "?")),
                "author": c.get("author", {}).get("name", "Unknown"),
                "date": c.get("updated", "").split(".")[0],
                "message": c.get("message", "").strip()
            })

    return data

def process_github(url):
    match = re.search(r'https://github\.com/([^/]+)/([^/]+)/pull/(\d+)', url)
    if not match:
        raise ValueError("Invalid GitHub PR URL format.")
        
    owner, repo, pr_number = match.group(1), match.group(2), match.group(3)
    
    headers = {'Accept': 'application/vnd.github.v3+json'}
    gh_token = os.environ.get('GITHUB_TOKEN')
    if gh_token: headers['Authorization'] = f'token {gh_token}'
    
    api_base = f"https://api.github.com/repos/{owner}/{repo}"
    
    raw_pr = fetch_json(f"{api_base}/pulls/{pr_number}", headers)
    if not raw_pr: return None
    pr = json.loads(raw_pr)
    
    raw_comments = fetch_json(f"{api_base}/pulls/{pr_number}/comments", headers)
    comments = json.loads(raw_comments) if raw_comments else []
    
    raw_issue_comments = fetch_json(f"{api_base}/issues/{pr_number}/comments", headers)
    issue_comments = json.loads(raw_issue_comments) if raw_issue_comments else []
    
    data = {
        "metadata": {
            "platform": "GitHub",
            "repo": f"{owner}/{repo}",
            "url": url,
            "id": pr_number,
            "title": pr.get('title', 'Untitled'),
            "author": pr.get('user', {}).get('login', 'Unknown'),
            "status": pr.get('state', 'Unknown'),
        },
        "general_comments": [],
        "inline_comments": {}
    }

    # Extract general issue comments (The PR timeline conversation)
    for c in issue_comments:
        data["general_comments"].append({
            "author": c.get('user', {}).get('login', 'Unknown'),
            "date": c.get('created_at', '').replace('T', ' ').replace('Z', ''),
            "message": c.get('body', '').strip()
        })

    # Extract inline code comments
    for c in comments:
        path = c.get('path', 'Unknown')
        if path not in data["inline_comments"]:
            data["inline_comments"][path] = []
        
        data["inline_comments"][path].append({
            "line": str(c.get('line') or c.get('original_line') or 'outdated'),
            "author": c.get('user', {}).get('login', 'Unknown'),
            "date": c.get('created_at', '').replace('T', ' ').replace('Z', ''),
            "message": c.get('body', '').strip()
        })
        
    return data

def generate_markdown(data):
    m = data["metadata"]
    md = []
    md.append(f"# [{m['platform']}] CL/PR {m['id']}: {m['title']}")
    md.append(f"\n## Metadata Summary")
    md.append(f"- **Author**: {m['author']}")
    md.append(f"- **Status**: {m['status']}")
    if 'rounds' in m:
        md.append(f"- **Total Rounds (Gerrit)**: {m['rounds']}")
    md.append(f"- **URL**: {m['url']}")
    
    md.append("\n## General PR Comments")
    if not data["general_comments"]:
        md.append("\n*No general comments found.*")
    else:
        for c in data["general_comments"]:
            rnd = f" (Round {c['round']})" if "round" in c else ""
            md.append(f"**General Comment** - @{c['author']}{rnd} ({c['date']}):")
            indented_message = "\n".join([f"> {line}" for line in c["message"].split("\n")])
            md.append(indented_message)
            md.append("")
            
    md.append("\n## Inline Review Comments")
    if not data["inline_comments"]:
        md.append("\n*No inline comments found.*")
    else:
        for file_path, file_comments in sorted(data["inline_comments"].items()):
            md.append(f"\n### File: `{file_path}`")
            # Sort by line (if numeric)
            def line_sort_key(c):
                try: return int(c["line"])
                except ValueError: return 0
            
            file_comments.sort(key=line_sort_key)
            
            for c in file_comments:
                rnd = f" (Round {c['round']})" if "round" in c else ""
                md.append(f"**[{file_path}:{c['line']}]{rnd}** - @{c['author']} ({c['date']}):")
                indented_message = "\n".join([f"> {line}" for line in c["message"].split("\n")])
                md.append(indented_message)
                md.append("")

    return "\n".join(md)

def main():
    if len(sys.argv) < 2:
        print("Usage: python fetch_reviews.py <url>")
        sys.exit(1)

    url = sys.argv[1]
    
    try:
        if "github.com" in url and "/pull/" in url:
            data = process_github(url)
        elif "/c/" in url and "/+/" in url:
            data = process_gerrit(url)
        else:
            print("Error: Unsupported URL format. Please provide a GitHub PR or Gerrit CL URL.")
            sys.exit(1)
    except Exception as e:
        print(f"Error parsing URL or fetching data: {e}")
        sys.exit(1)

    if not data:
        print("Failed to fully process the requested review data.")
        sys.exit(1)

    # Save outputs
    base_dir = os.path.dirname(os.path.abspath(__file__))
    model_dir = os.path.join(base_dir, "model")
    manual_dir = os.path.join(base_dir, "manual")
    
    os.makedirs(model_dir, exist_ok=True)
    os.makedirs(manual_dir, exist_ok=True)
    
    safe_subject = re.sub(r'[^a-zA-Z0-9_\-]', '_', data["metadata"]["title"])[:40]
    filename = f"Review_{data['metadata']['id']}_{safe_subject}"
    
    # 1. Output LLM-Friendly JSON representation
    model_path = os.path.join(model_dir, f"{filename}.json")
    with open(model_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
        
    # 2. Output Human-Friendly Markdown representation
    manual_path = os.path.join(manual_dir, f"{filename}.md")
    with open(manual_path, "w", encoding="utf-8") as f:
        f.write(generate_markdown(data))
        
    print(f"\nSuccessfully generated outputs:")
    print(f"1. Model Agent JSON: {model_path}")
    print(f"2. Manual Check Markdown: {manual_path}")

if __name__ == "__main__":
    main()
