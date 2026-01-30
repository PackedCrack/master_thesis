$POPUP_TIMEOUT_INFINITE = 0
# popup button choices
$POPUP_BUTTONS_OK = 0x00000000
$POPUP_BUTTONS_OK_CANCEL = 0x00000001
# popup icons
$POPUP_ICON_EXCLAMATION = 0x00000030   # 48
$POPUP_ICON_INFO        = 0x00000040   # 64
# popup return codes
$POPUP_RESULT_OK     = 1
$POPUP_RESULT_CANCEL = 2


function Show-Popup 
{
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]
        [string]$Message,

        [Parameter(Mandatory)]
        [string]$Title,

        [Parameter(Mandatory)]
        [int]$Buttons,

        [Parameter(Mandatory)]
        [int]$Icon
    )

    $ws = New-Object -ComObject WScript.Shell
    $style = ($Buttons -bor $Icon)
    return $ws.Popup($Message, $POPUP_TIMEOUT_INFINITE, $Title, $style)
}
function Show-Popup-Info
{
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]
        [string]$Message
    )

    return Show-Popup -Message $Message -Title "Information" -Buttons $POPUP_BUTTONS_OK -Icon $POPUP_ICON_INFO
}
function Show-Popup-Warning
{
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]
        [string]$Message
    )

    return Show-Popup -Message $Message -Title "Warning" -Buttons $POPUP_BUTTONS_OK -Icon $POPUP_ICON_EXCLAMATION
}
function Show-Popup-Confirmation
{
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]
        [string]$Message
    )

    return Show-Popup -Message $Message -Title "Confirm" -Buttons $POPUP_BUTTONS_OK_CANCEL -Icon $POPUP_ICON_EXCLAMATION
}


############################################################################

$desktop = [Environment]::GetFolderPath('Desktop')
$outputDir = Join-Path $desktop 'output'

if (-not (Test-Path -Path $outputDir -PathType Container))
{
    Show-Popup-Info -Message "The log directory '$outputDir' does not exist."
}
else 
{
    $confirmed = Show-Popup-Confirmation -Message "Delete the log directory '$outputDir' and all contents?"
    if ($confirmed -eq $POPUP_RESULT_OK) 
    {
        try
        {
            Remove-Item -Path $outputDir -Recurse -Force -ErrorAction Stop
            Show-Popup-Info -Message "Deleted the log directory: '$outputDir'"
        } 
        catch
        {
            Show-Popup-Warning -Message "Deletion failed: $($_.Exception.Message)"
        }
    } 
}

