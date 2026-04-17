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

def extract_gerrit_info(url):
    """Parse Gerrit URL to get host, repo, and change_number."""
    # Matches formats like: https://dawn-review.googlesource.com/c/dawn/+/224154
    match = re.search(r'(https://[^/]+)/c/([^/]+)/\+/(\d+)', url)
    if not match:
        raise ValueError("Invalid Gerrit URL. Expected format: https://<host>/c/<repo>/+/<change_number>")
    return match.group(1), match.group(2), match.group(3)

def fetch_gerrit_api(host, endpoint):
    """Helper to fetch data from the Gerrit REST API."""
    url = f"{host}/{endpoint}"
    print(f"Fetching {url}...")
    req = urllib.request.Request(url)
    try:
        with urllib.request.urlopen(req) as response:
            return clean_gerrit_json(response.read())
    except urllib.error.URLError as e:
        print(f"Failed to fetch {url}: {e}")
        return None

def main():
    if len(sys.argv) < 2:
        print("Usage: python fetch_cl_reviews.py <gerrit_url>")
        print("Example: python fetch_cl_reviews.py https://dawn-review.googlesource.com/c/dawn/+/224154")
        sys.exit(1)

    url = sys.argv[1]
    try:
        host, repo, change_number = extract_gerrit_info(url)
    except ValueError as e:
        print(e)
        sys.exit(1)

    # 1. Fetch CL metadata
    # The detail endpoint contains messages, owner, subject, etc.
    detail = fetch_gerrit_api(host, f"changes/{change_number}/detail")
    if not detail:
        print("Failed to get CL metadata.")
        sys.exit(1)

    # 2. Fetch inline comments
    comments = fetch_gerrit_api(host, f"changes/{change_number}/comments")

    # Determine maximum review round (patch set) from messages
    messages = detail.get("messages", [])
    max_round = max([msg.get("_revision_number", 1) for msg in messages]) if messages else 1

    # 3. Format the Markdown Output
    # This acts as an indexed metadata file for an AI Agent to easily parse
    md_content = []
    
    # Metadata Frontmatter
    md_content.append("---")
    md_content.append(f"repo: {repo}")
    md_content.append(f"url: {url}")
    md_content.append(f"change_id: {detail.get('change_id', 'Unknown')}")
    md_content.append(f"owner: {detail.get('owner', {}).get('name', 'Unknown')}")
    md_content.append(f"rounds: {max_round}")
    md_content.append(f"status: {detail.get('status', 'NEW')}")
    md_content.append("---")
    md_content.append("")
    
    md_content.append(f"# CL: {detail.get('subject', 'Untitled')}")
    md_content.append("\n## Metadata Summary")
    md_content.append(f"- **Author**: {detail.get('owner', {}).get('name', 'Unknown')}")
    md_content.append(f"- **Branch**: {detail.get('branch', 'main')}")
    md_content.append(f"- **Current Patchset/Round**: {max_round}")
    
    md_content.append("\n## Review Content Index")
    
    if not comments:
        md_content.append("\n*No inline comments found on this CL.*")
    else:
        # Sort and index comments by file
        for file_path, file_comments in sorted(comments.items()):
            md_content.append(f"\n### File: `{file_path}`")
            file_comments.sort(key=lambda x: x.get("line", 0))
            
            for comment in file_comments:
                line_num = comment.get("line", "File-level")
                patch_set = comment.get("patch_set", "?")
                author = comment.get("author", {}).get("name", "Unknown")
                message = comment.get("message", "").strip()
                # Format updated date
                updated = comment.get("updated", "").split(".")[0] 
                
                # Format an indexed block for clear parsing
                md_content.append(f"**[{file_path}:{line_num}] (Round {patch_set})** - @{author} ({updated}):")
                
                # Blockquote the comment correctly
                indented_message = "\n".join([f"> {m}" for m in message.split("\n")])
                md_content.append(indented_message)
                md_content.append("") # Spacing

    # 4. Save to Disk
    output_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "reviews_data")
    os.makedirs(output_dir, exist_ok=True)
    
    safe_subject = re.sub(r'[^a-zA-Z0-9_\-]', '_', detail.get('subject', 'untitled'))[:40]
    filename = f"CL_{change_number}_{safe_subject}.md"
    filepath = os.path.join(output_dir, filename)
    
    with open(filepath, "w", encoding="utf-8") as f:
        f.write("\n".join(md_content))
        
    print(f"\nSuccessfully generated indexed review: {filepath}")

if __name__ == "__main__":
    main()
