# BBMan wx3/GTK3 Port

This repository is a modern Linux port of BBMan, a classic Taiwan BBS
terminal client.

The current wxWidgets 3 / GTK3 port release is 3.2.0.

## Upstream

The original BBMan project was published on SourceForge:

https://sourceforge.net/projects/bbman/

This Git repository was started from a BBMan 3.0.1 CVS checkout, imported as
the first commit. It is not a GitHub fork of another repository.

## Current Scope

The active port targets modern Linux desktops with wxWidgets 3 / GTK3. The
main practical target is SSH access to PTT and similar Big5 BBS systems.

Supported in the active wx3 build:

- wxWidgets 3 / GTK3 UI
- Telnet connections
- SSH connections through the system OpenSSH client over a PTY
- Big5/HKSCS-oriented terminal rendering and input
- ANSI colors, BBS cursor behavior, selection, copy/paste, bookmarks
- Bundled BBS site list, themes, desktop file, and application icons
- libsecret-backed storage for saved site secrets

Not supported in the active wx3 build:

- The original embedded libssh implementation
- SFTP
- SSH password auto-login through BBMan's old libssh callback path

The old embedded SSH/SFTP code is kept under `legacy/ssh-libssh/` only for
historical reference.

## Build

Install build dependencies on Ubuntu/Debian:

```sh
sudo apt install build-essential pkg-config libwxgtk3.2-dev \
  libsecret-1-dev openssh-client
```

Build the wx3/GTK3 target:

```sh
cd unix
make -f Makefile.wx3
```

Run from the source tree:

```sh
./obj/bbman-wx3
```

## Install

Install under `/usr/local` by default:

```sh
cd unix
sudo make -f Makefile.wx3 install
```

For packaging or testing an install tree:

```sh
cd unix
make -f Makefile.wx3 install DESTDIR=/tmp/bbman-install PREFIX=/usr
```

The install target installs:

- `bbman-wx3`
- themes under `share/bbman/theme`
- bundled `sites.dat`
- desktop file
- hicolor application icons
- gettext message catalogs

## SSH Model

Modern SSH support is implemented by spawning the system `ssh` client through
`forkpty()`. BBMan talks to the PTY like a terminal; OpenSSH handles the SSH
protocol, host keys, authentication, and password prompts.

For PTT, quick connection fallback can try `bbs@ptt.cc` over SSH before older
telnet-style candidates. BBS account login remains a BBS-level interaction
after the SSH connection is established.

## Encoding Notes

The terminal still follows Big5 BBS byte semantics. The wx3 port decodes using
`BIG5HKSCS`, which covers more historic BBS characters than plain Big5.
Complete UAO 2.50 mapping is not implemented yet, so rare board-specific
characters may still display as replacement glyphs.

## Historical Files

Some original project files remain in the tree for reference, including old
Dev-C++ project files and wxGTK2-era Makefiles. For current Linux use, prefer:

```sh
unix/Makefile.wx3
```

## License

The original source declares GPL licensing through a GPL URL, but the imported
CVS tree does not name an exact GPL version. BBMan was originally released in
the GPLv2 era, before GPLv3 existed, so this maintained fork is conservatively
conveyed under GNU GPL version 2.

See `LICENSE` for the GPLv2 text, `license` for the original one-line upstream
file, and `COPYING` for the repository license note.
