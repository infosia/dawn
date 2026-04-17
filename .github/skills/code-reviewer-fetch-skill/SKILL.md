```skill
# Code Review Fetcher Skill

## Description
This skill provides the ability to fetch, format, and index code review comments from both **GitHub Pull Requests** and **Gerrit CLs**. It downloads the PR's metadata, general comments, and inline code comments, outputting a highly-structured Markdown file. This allows Claude (or any LLM agent) to easily reference, summarize, or analyze reviewer feedback.

It also supports **tracking** CLs/PRs over time and **syncing** them on demand to detect new comments, status changes, and new inline review feedback since the last check.

## When to use
Use this skill whenever the user asks you to:
- "Summarize the review comments for this PR"
- "Fetch the PR feedback for [GitHub/Gerrit URL]"
- "Check what the reviewers said on my commit"
- "Track this CL" / "Start tracking this CL/PR"
- "What's the new status of all tracked CLs?"
- "Sync all tracked CL status"
- "Any new comments on my CLs?"
- "Stop tracking CL 12345"
- "List my tracked CLs"

If the user provides a URL that looks like `https://github.com/<owner>/<repo>/pull/<number>` or `https://<host>/c/<repo>/+/<change_number>`, it's a strong indicator to use this skill.

## How to use

### One-time Fetch (existing behavior)
1. **Execute the Script**: Run the included python script `fetch_reviews.py` using the standard Python Execution Tool (or via shell command), providing the PR/CL URL as the argument.
   
   ```bash
   # For GitHub
   python fetch_reviews.py "https://github.com/microsoft/vscode/pull/12345"
   
   # For Gerrit
   python fetch_reviews.py "https://dawn-review.googlesource.com/c/dawn/+/224154"
   ```

2. **Wait for output**: The script will print out the absolute paths of two generated files:
   - `model/*.json`: A highly structured JSON version perfect for your internal processing. Read this one to ingest the metadata and comments!
   - `manual/*.md`: A readable markdown version intended for the human user.

3. **Read the generated file**: Read the content of the generated `.md` file to ingest the aggregated and indexed reviewer comments.

4. **Return Results to User**: Summarize the reviewers' comments, note formatting issues, highlight architectural concerns, and present an actionable list for the human developer.

### CL/PR Tracking & Sync

Use `track_cls.py` to track CLs/PRs over time and detect changes on demand.

#### Track a CL/PR
```bash
python track_cls.py track "https://dawn-review.googlesource.com/c/dawn/+/287075"
python track_cls.py track "https://github.com/user/repo/pull/123"
```
This fetches the current state as a baseline and stores it in `tracked_cls.json`.

#### List tracked CLs/PRs
```bash
python track_cls.py list
```
Prints a table of all tracked CLs/PRs with their ID, platform, status, and last sync time.

#### Sync all tracked CLs/PRs
```bash
python track_cls.py sync
```
Fetches the latest state of every tracked CL/PR and compares against the stored baseline. Reports:
- Status changes (e.g. NEW -> MERGED)
- Count of new general comments since last sync
- Count of new inline review comments since last sync
- The full content of all new comments

Outputs a sync report to `model/SyncReport_*.json` (for agent) and `manual/SyncReport_*.md` (for human).

#### Sync a single CL/PR
```bash
python track_cls.py sync 287075
```

#### Untrack a CL/PR
```bash
python track_cls.py untrack 287075
```

#### Agent workflow for sync
1. Run `python track_cls.py sync` in the terminal.
2. Read the generated `model/SyncReport_*.json` to ingest the structured diff.
3. Summarize the new activity for the user: which CLs have new comments, what reviewers said, any status changes.

## Requirements
- Access to execute python code.
- Internet access to query the Gerrit REST API endpoint.
```
