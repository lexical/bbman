# Legacy embedded SSH/SFTP implementation

This directory contains BBMan's original embedded libssh-based SSH and SFTP code.
The wxWidgets 3 build does not compile it; modern SSH connections use the system
ssh client through a PTY.

The files are kept here for historical reference and for comparing with the old
wxWidgets 2 build, without keeping obsolete SSH code in the active source tree.
