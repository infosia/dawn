# Gerrit helper functions for Windows PowerShell
# Usage:
#   . "$PSScriptRoot/gerrit.ps1"   # dot-source to import functions

Set-StrictMode -Version Latest

function Get-PropValue {
  [CmdletBinding()] param(
    [Parameter(Mandatory)] [object]$Object,
    [Parameter(Mandatory)] [string]$Name
  )
  $prop = $Object.PSObject.Properties[$Name]
  if ($prop) { return $prop.Value }
  return $null
}

function Get-GerritRef {
  [CmdletBinding()] param(
    [Parameter(Mandatory)] [int]$Change,
    [Parameter(Mandatory)] [int]$Patchset
  )
  $two = '{0:d2}' -f ($Change % 100)
  "refs/changes/$two/$Change/$Patchset"
}

function Get-GerritPatchset {
  [CmdletBinding()] param(
    [Parameter(Mandatory)] [int]$Change,
    [Parameter(Mandatory)] [int]$Patchset,
    [string]$Remote = 'origin'
  )
  $ref = Get-GerritRef -Change $Change -Patchset $Patchset
  Write-Verbose "Fetching $ref from $Remote"
  git fetch $Remote $ref | Out-Null
  git checkout FETCH_HEAD
}

function Get-GerritAuthHeader {
  param(
    [string]$User = $env:GERRIT_USER,
    [string]$Password = $env:GERRIT_HTTP_PASSWORD
  )
  if (-not $User -or -not $Password) { return @{} }
  $pair = "{0}:{1}" -f $User, $Password
  @{ Authorization = 'Basic ' + [Convert]::ToBase64String([Text.Encoding]::ASCII.GetBytes($pair)) }
}

function Get-GerritJson {
  [CmdletBinding()] param(
    [Parameter(Mandatory)] [string]$Uri,
    [string]$User = $env:GERRIT_USER,
    [string]$Password = $env:GERRIT_HTTP_PASSWORD
  )
  $headers = Get-GerritAuthHeader -User $User -Password $Password
  $tryUris = @($Uri)
  if (-not $headers.ContainsKey('Authorization')) {
    # If no auth, prefer non-/a endpoint
    if ($Uri -match '/a/') {
      $tryUris = @( ($Uri -replace '/a/', '/'), $Uri )
    }
  }
  foreach ($u in $tryUris) {
    try {
      $resp = Invoke-WebRequest -UseBasicParsing -Uri $u -Headers $headers
      $json = $resp.Content -replace "^\)\]\}'\s*", ''
      return ($json | ConvertFrom-Json)
    } catch {
      # If first try was /a/ and unauthenticated, keep trying next
      continue
    }
  }
  throw "Failed to GET $Uri"
}

function Get-GerritCommentsAllPatchsets {
  [CmdletBinding()] param(
    [Parameter(Mandatory)] [string]$Change,
    [string]$GerritHost = 'https://dawn-review.googlesource.com'
  )
  $uri = "$GerritHost/a/changes/$Change/comments"
  Get-GerritJson -Uri $uri
}

function Get-GerritRevisionShaForPatchset {
  [CmdletBinding()] param(
    [Parameter(Mandatory)] [string]$Change,
    [Parameter(Mandatory)] [int]$Patchset,
    [string]$GerritHost = 'https://dawn-review.googlesource.com'
  )
  $uri = "$GerritHost/a/changes/$Change/revisions/"
  $revs = Get-GerritJson -Uri $uri
  foreach ($prop in $revs.PSObject.Properties) {
    if ($prop.Value._number -eq $Patchset) { return $prop.Name }
  }
  throw "Patchset $Patchset not found for change $Change"
}

function Get-GerritCommentsForPatchset {
  [CmdletBinding()] param(
    [Parameter(Mandatory)] [string]$Change,
    [Parameter(Mandatory)] [int]$Patchset,
    [string]$GerritHost = 'https://dawn-review.googlesource.com'
  )
  $sha = Get-GerritRevisionShaForPatchset -Change $Change -Patchset $Patchset -GerritHost $GerritHost
  $uri = "$GerritHost/a/changes/$Change/revisions/$sha/comments"
  Get-GerritJson -Uri $uri
}

function Select-GerritCommentsByReviewer {
  [CmdletBinding()] param(
    [Parameter(Mandatory)] $CommentsByFile,
    [string]$ReviewerEmail,
    [int]$ReviewerAccountId
  )
  $out = @()
  if (-not $CommentsByFile) { return $out }
  foreach ($prop in $CommentsByFile.PSObject.Properties) {
    $file = $prop.Name
    $list = $prop.Value
    if (-not $list) { continue }
    foreach ($c in $list) {
      $author = Get-PropValue -Object $c -Name 'author'
      $authorEmail = if ($author) { Get-PropValue -Object $author -Name 'email' } else { $null }
      $authorId = if ($author) { Get-PropValue -Object $author -Name '_account_id' } else { $null }
      if ($ReviewerEmail -and $authorEmail -ne $ReviewerEmail) { continue }
      if ($PSBoundParameters.ContainsKey('ReviewerAccountId') -and $authorId -ne $ReviewerAccountId) { continue }
      $line = Get-PropValue -Object $c -Name 'line'
      $msg = Get-PropValue -Object $c -Name 'message'
      $updated = Get-PropValue -Object $c -Name 'updated'
      $authorDisplay = $authorEmail
      if (-not $authorDisplay -and $author) {
        $authorName = Get-PropValue -Object $author -Name 'name'
        if ($authorName) { $authorDisplay = $authorName }
        elseif ($authorId) { $authorDisplay = "acct:$authorId" }
      }
      if (-not $authorDisplay) { $authorDisplay = 'unknown' }
      $out += [PSCustomObject]@{
        file    = $file
        line    = $line
        message = $msg
        updated = $updated
        author  = $authorDisplay
      }
    }
  }
  $out
}

# Convenience wrappers for common jobs

function Show-GerritReviewerComments {
  [CmdletBinding()] param(
    [Parameter(Mandatory)] [string]$Change,
    [int]$Patchset,
    [string]$ReviewerEmail,
    [int]$ReviewerAccountId,
  [string]$GerritHost = 'https://dawn-review.googlesource.com'
  )
  $comments = if ($PSBoundParameters.ContainsKey('Patchset')) {
  Get-GerritCommentsForPatchset -Change $Change -Patchset $Patchset -GerritHost $GerritHost
  } else {
  Get-GerritCommentsAllPatchsets -Change $Change -GerritHost $GerritHost
  }
  $filtered = Select-GerritCommentsByReviewer -CommentsByFile $comments -ReviewerEmail $ReviewerEmail -ReviewerAccountId $ReviewerAccountId
  $filtered | Sort-Object file, line | Format-Table -AutoSize
}

function Export-GerritReviewerCommentsCsv {
  [CmdletBinding()] param(
    [Parameter(Mandatory)] [string]$Change,
    [int]$Patchset,
    [string]$ReviewerEmail,
    [int]$ReviewerAccountId,
    [string]$OutFile = 'comments.csv',
  [string]$GerritHost = 'https://dawn-review.googlesource.com'
  )
  $comments = if ($PSBoundParameters.ContainsKey('Patchset')) {
  Get-GerritCommentsForPatchset -Change $Change -Patchset $Patchset -GerritHost $GerritHost
  } else {
  Get-GerritCommentsAllPatchsets -Change $Change -GerritHost $GerritHost
  }
  $filtered = Select-GerritCommentsByReviewer -CommentsByFile $comments -ReviewerEmail $ReviewerEmail -ReviewerAccountId $ReviewerAccountId
  $filtered | Sort-Object file, line | Export-Csv -NoTypeInformation -Path $OutFile
  Write-Host "Saved: $OutFile"
}

# --- Advanced: Query changes and build reports ---

function Get-GerritChangesByQuery {
  [CmdletBinding()] param(
    [Parameter(Mandatory)] [string]$Query,
  [string]$GerritHost = 'https://dawn-review.googlesource.com',
    [int]$PageSize = 200,
    [int]$MaxPages = 50
  )
  $all = @()
  $start = 0
  for ($page=0; $page -lt $MaxPages; $page++) {
    $q = [System.Uri]::EscapeDataString($Query)
  # Request detailed account info so owner.email is present
  $uri = "$GerritHost/a/changes/?q=$q&n=$PageSize&S=$start&o=DETAILED_ACCOUNTS"
    $batch = Get-GerritJson -Uri $uri
    if (-not $batch) { break }
    $all += $batch
    # StrictMode-safe check of pagination marker on the last element
    $last = if ($batch -is [array]) { $batch[-1] } else { $batch }
    $hasMore = $false
    if ($last) {
      $flag = Get-PropValue -Object $last -Name '_more_changes'
      if ($flag) { $hasMore = $true }
    }
    if (-not $hasMore) { break }
    $start += $PageSize
  }
  $all
}

function Get-GerritAllCommentsForChanges {
  [CmdletBinding()] param(
    [Parameter(Mandatory)] $Changes,  # array of ChangeInfo
    [string]$GerritHost = 'https://dawn-review.googlesource.com'
  )
  $rows = @()
  foreach ($chg in $Changes) {
    try {
      $commentsByFile = Get-GerritCommentsAllPatchsets -Change $chg._number -GerritHost $GerritHost
    } catch {
      Write-Warning "Failed to get comments for change $($chg._number): $_"
      continue
    }
    if (-not $commentsByFile) { continue }
    $owner = Get-PropValue -Object $chg -Name 'owner'
    $ownerEmail = if ($owner) { Get-PropValue -Object $owner -Name 'email' } else { $null }
    if (-not $ownerEmail -and $owner) {
      $ownerName = Get-PropValue -Object $owner -Name 'name'
      $ownerId = Get-PropValue -Object $owner -Name '_account_id'
      if ($ownerName) { $ownerEmail = $ownerName }
      elseif ($ownerId) { $ownerEmail = "acct:$ownerId" }
    }
    if (-not $ownerEmail) { $ownerEmail = 'unknown' }
    foreach ($prop in $commentsByFile.PSObject.Properties) {
      $file = $prop.Name
      $list = $prop.Value
      if (-not $list) { continue }
      foreach ($c in $list) {
        $line = Get-PropValue -Object $c -Name 'line'
        $msg = Get-PropValue -Object $c -Name 'message'
        $updated = Get-PropValue -Object $c -Name 'updated'
        $patchset = Get-PropValue -Object $c -Name 'patch_set'
        $author = Get-PropValue -Object $c -Name 'author'
        $authorEmail = if ($author) { Get-PropValue -Object $author -Name 'email' } else { $null }
        $authorName = if ($author) { Get-PropValue -Object $author -Name 'name' } else { $null }
        $authorId = if ($author) { Get-PropValue -Object $author -Name '_account_id' } else { $null }
        $authorDisplay = $authorEmail
        if (-not $authorDisplay) {
          if ($authorName) { $authorDisplay = $authorName }
          elseif ($authorId) { $authorDisplay = "acct:$authorId" }
        }
        if (-not $authorDisplay) { $authorDisplay = 'unknown' }
        $rows += [PSCustomObject]@{
          change_number = $chg._number
          change_id     = $chg.change_id
          project       = $chg.project
          subject       = $chg.subject
          owner_email   = $ownerEmail
          file          = $file
          line          = $line
          message       = $msg
          updated       = $updated
          author_email  = $authorDisplay
          author_name   = $authorName
          patch_set     = $patchset
        }
      }
    }
  }
  $rows
}

function Test-IsPureLGTMComment {
  [CmdletBinding()] param(
    [Parameter(Mandatory)] [string]$Message
  )
  $t = ($Message -as [string]).Trim()
  if (-not $t) { return $false }
  # Pure LGTM variants like: "LGTM", "lgtm", "LGTM!", "LGTM." (no extra text)
  if ($t -match '^(?i)lgtm[!\.]*$') { return $true }
  return $false
}

function New-GerritCommentsMarkdown {
  [CmdletBinding()] param(
  [Parameter(Mandatory)] $Comments, # flat array of PSCustomObject
  $Changes,
  [string[]]$Owners,
    [string]$GerritHost = 'https://dawn-review.googlesource.com'
  )
  $nl = "`r`n"
  $sb = New-Object System.Text.StringBuilder
  [void]$sb.Append("# Gerrit comments report$nl$nl")
  [void]$sb.Append("Host: $GerritHost$nl$nl")
  if ($Owners) { [void]$sb.Append("Owners: " + ($Owners -join ', ') + "$nl$nl") }

  $total = @($Comments).Count
  [void]$sb.Append("Total comments: $total$nl$nl")

  if ($Changes) {
    [void]$sb.Append("## Changes$nl$nl")
    [void]$sb.Append("| Change | Project | Owner | Subject |$nl")
    [void]$sb.Append("|---:|---|---|---|$nl")
    foreach ($chg in ($Changes | Sort-Object _number)) {
      $link = if ($chg.project) { "$GerritHost/c/$($chg.project)/+/$($chg._number)" } else { "$GerritHost/c/$($chg._number)" }
      $subj = ($chg.subject -replace '\r?\n', ' ') -replace '\|', '\\|'
      $ownerStr = 'unknown'
      if ($chg.owner) {
        if ($chg.owner.email) { $ownerStr = $chg.owner.email }
        elseif ($chg.owner.name) { $ownerStr = $chg.owner.name }
        elseif ($chg.owner._account_id) { $ownerStr = "acct:$($chg.owner._account_id)" }
      }
      [void]$sb.Append("| [$($chg._number)]($link) | $($chg.project) | $ownerStr | $subj |$nl")
    }
    [void]$sb.Append("$nl")
  }

  $byAuthor = $Comments | Group-Object author_email | Sort-Object Count -Descending
  [void]$sb.Append("## Comments by reviewer$nl$nl")
  foreach ($g in $byAuthor) {
    [void]$sb.Append("- $($g.Name): $($g.Count)$nl")
  }
  [void]$sb.Append("$nl")

  foreach ($g in $byAuthor) {
  $heading = if ([string]::IsNullOrWhiteSpace($g.Name)) { 'unknown' } else { $g.Name }
  [void]$sb.Append("## $heading$nl$nl")
    [void]$sb.Append("| Change | Project | File | Line | Updated | Message |$nl")
    [void]$sb.Append("|---|---|---|---:|---|---|$nl")
    foreach ($c in ($g.Group | Sort-Object change_number, file, line)) {
      $link = if ($c.project) { "$GerritHost/c/$($c.project)/+/$($c.change_number)" } else { "$GerritHost/c/$($c.change_number)" }
      $msg = ($c.message -replace '\r?\n', ' ') -replace '\|', '\\|'
      [void]$sb.Append("| [$($c.change_number)]($link) | $($c.project) | $($c.file) | $($c.line) | $($c.updated) | $msg |$nl")
    }
    [void]$sb.Append("$nl")
  }

  $sb.ToString()
}

function Invoke-GerritOwnersCommentsReport {
  [CmdletBinding()] param(
    [Parameter(Mandatory)] [string[]]$Owners,
  [string]$GerritHost = 'https://dawn-review.googlesource.com',
    [string]$OutFile = 'gerrit_comments_report.md',
    [switch]$ExcludeLGTM,
    [int]$PageSize = 200,
    [int]$MaxPages = 50
  )
  $allChanges = @()
  foreach ($o in $Owners) {
    Write-Host "Querying changes for owner: $o"
  $chs = Get-GerritChangesByQuery -Query "owner:$o" -GerritHost $GerritHost -PageSize $PageSize -MaxPages $MaxPages
    Write-Host "  Found $($chs.Count) changes"
    $allChanges += $chs
  }

  if (-not $allChanges) {
    Write-Warning "No changes found."
    return
  }

  $allComments = Get-GerritAllCommentsForChanges -Changes $allChanges -GerritHost $GerritHost
  if ($ExcludeLGTM) {
    $allComments = $allComments | Where-Object { -not (Test-IsPureLGTMComment -Message $_.message) }
  }

  if (-not $allComments) { $allComments = @() }
  $md = New-GerritCommentsMarkdown -Comments $allComments -Changes $allChanges -Owners $Owners -GerritHost $GerritHost
  $outPath = Join-Path -Path (Get-Location) -ChildPath $OutFile
  $md | Out-File -Encoding utf8 -FilePath $outPath
  Write-Host "Report saved to: $outPath"
}

