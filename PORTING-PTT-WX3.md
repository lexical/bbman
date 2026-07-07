# BBMan PTT / wx3 Porting Plan

Date: 2026-06-15

This note records the practical porting direction for making BBMan useful on
modern Ubuntu with wxWidgets 3 / GTK3 and current PTT BBS access.

## Short Conclusion

Original BBMan is not directly suitable for current PTT:

- PTT port 23 is still reachable, but it no longer offers unencrypted telnet
  login. It returns a Big5 notice saying telnet has been disabled and users
  should use WebSocket or SSH.
- BBMan's original bundled SSH implementation was based on old vendored libssh
  0.1. It only advertised obsolete algorithms such as
  `diffie-hellman-group1-sha1`, `ssh-dss` / `ssh-rsa`, and CBC ciphers. It was
  not compatible with current PTT SSH, and it has been removed from this
  maintained tree.
- The old build uses `wxgtk2-2.4-config`. On the current Ubuntu machine,
  `wx-config` is `3.2.4` with `gtk3-unicode-3.2`.

However, BBMan is still a better starting point than PCManX for a focused
modern PTT client:

- Its BBS terminal logic is compact and relatively self-contained.
- It is written through wxWidgets rather than directly against GTK/Xft/X11.
- It already has BBS-specific behavior: ANSI color, Big5 double-byte handling,
  paste wrapping, selection, auto-login, NAWS-style sizing, and bookmarks.

The recommended approach is not to revive the old SSH code. Keep the BBS
terminal model, port the UI to wx3/GTK3, and add a new OpenSSH-backed transport.

## Target Architecture

The new runtime shape should be:

```text
PTT SSH
  <-> system OpenSSH process
  <-> forkpty() master fd
  <-> BBMan transport backend
  <-> ANSI / BBS byte parser
  <-> terminal cell grid
  <-> wx3/GTK3 Unicode rendering
```

Important rule:

The terminal core should not treat `wxString` as the network byte stream.
Network data is Big5/UAO-oriented bytes. wx3 is Unicode. The boundary between
the two must be explicit.

## Transport Plan: forkpty + exec OpenSSH

Add a new backend, tentatively named `SCD_OpenSSH`, and wire it into the existing
`SCD_Socket` abstraction.

The child process should be created with `forkpty()` and then `execvp("ssh",
argv)`. Do not invoke a shell.

Example argv for PTT:

```text
ssh -tt -e none -o StrictHostKeyChecking=accept-new -o ServerAliveInterval=60 -p 22 bbs@ptt.cc
```

Rationale:

- `-tt` forces a remote tty allocation.
- `-e none` disables OpenSSH's local escape handling so BBS input such as `~.`
  is not intercepted by the SSH client.
- `ServerAliveInterval=60` helps detect stale connections.
- `execvp` with argv avoids shell quoting and command injection problems.

Set a conservative terminal type in the child environment:

```c
setenv("TERM", "vt100", 1);
```

For resizing, do not send telnet NAWS bytes through SSH. Resize the local pty:

```c
struct winsize ws = {};
ws.ws_col = cols;
ws.ws_row = rows;
ioctl(master_fd, TIOCSWINSZ, &ws);
kill(child_pid, SIGWINCH);
```

Minimal `SCD_OpenSSH` responsibilities:

- Spawn child via `forkpty`.
- Put master fd in non-blocking mode.
- Integrate the fd into the wx event loop, preferably with `wxSocket`-like
  notifications or a small polling timer if necessary.
- Implement `Read`, `Write`, `Close`, `IsConnected`, `IsDisconnected`.
- Reap child with `waitpid`.
- Surface EOF and child exit as connection lost.

The old `src/scd_wxssh/` and vendored `src/libssh/` code should not be
revived. It has been removed from the maintained tree; use the OpenSSH/PTY
backend instead.

## Big5 / Unicode Handling Plan

The main wx3/GTK3 porting risk is not SSH. It is Big5 byte handling.

Existing BBMan code stores and parses terminal data as bytes, but it also
converts through `wxString` in many places. That was survivable in the old
wxGTK2-era code. It becomes dangerous under wx3 Unicode builds.

The safe model:

```text
wire bytes -> parser -> cells with raw bytes + decoded Unicode glyph -> wxDC draw
```

### Terminal Cell Model

Replace or evolve `TerminalChar` toward a cell model like:

```cpp
struct Cell {
    uint8_t raw[4];
    uint8_t raw_len;
    uint8_t width;      // 1 or 2 terminal columns
    char32_t ucs;       // Unicode code point for drawing
    Attr attr;
};
```

The parser may continue to understand Big5 byte structure for cursor movement
and BBS column semantics. Rendering should use Unicode glyphs.

For a Big5 full-width character:

- The first cell stores the raw Big5 bytes and decoded Unicode code point.
- The cell width is 2.
- The second cell is a continuation marker.
- Selection and cursor movement must avoid splitting the pair.

### Codec Layer

Introduce a single codec object, tentatively:

```cpp
class BbsCodec {
public:
    DecodeResult decodeOne(const uint8_t* p, size_t n);
    std::string encode(const wxString& text);
    wxString toWxString(const Cell& cell);
};
```

Initial implementation can use `iconv`:

- Decode: Big5 -> UTF-8 -> `wxString`
- Encode: `wxString` -> UTF-8 -> Big5

Better implementation should reuse PCManX's UAO tables for Taiwan BBS
compatibility. Plain system Big5 may not cover all historic BBS characters.

### Rendering

Do not call `wxString(str, 2)` on Big5 bytes. Under wx3 Unicode this can be
misinterpreted.

Instead:

```cpp
wxString glyph = codec.toWxString(cell);
dc->DrawText(glyph, x, y);
```

Rendering code should draw one full-width glyph across two terminal columns.

### Input

Text input path:

```text
wx key/text event Unicode -> BbsCodec encode -> write bytes to pty
```

Special keys are not text and should stay as terminal escape sequences:

- Arrow keys
- PageUp / PageDown
- Home / End
- Enter / Backspace / Escape

### Clipboard

Plain copy:

```text
selected cells raw bytes -> decode Big5/UAO -> Unicode clipboard text
```

Paste:

```text
Unicode clipboard text -> encode Big5/UAO -> send bytes
```

ANSI copy for pasting back into BBS should preserve ANSI escape sequences and
encode text as BBS bytes. ANSI copy for modern applications can be a separate
future option.

## wx3 / GTK3 Porting Tasks

The first trial build against wx3 already showed typical old wx/C++ issues:

- Old `wxgtk2-2.4-config` build scripts need replacement with `wx-config`.
- Some old C++ declarations fail under modern compilers.
- `wxDC::BeginDrawing()` and `EndDrawing()` are gone.
- Many `char*` / `wxString` implicit conversions fail because wx3 is Unicode.

Expected mechanical changes:

- Replace build system with a small Makefile or CMake target using `wx-config`.
- Build an OpenSSH-only target first.
- Fix deprecated or removed wx APIs.
- Replace old event macros only where needed.
- Remove direct assumptions that `wxString::c_str()` is a byte string.

Expected semantic changes:

- Clarify byte-vs-Unicode boundaries.
- Use explicit Big5/UAO codec conversion.
- Update drawing to render Unicode glyphs from decoded cells.
- Update input and clipboard paths.

## Recommended Milestones

### Milestone 1: Build nossh UI on wx3

Goal: application starts on wx3/GTK3.

Tasks:

- Add modern build file.
- Keep old vendored libssh out of the wx3 target.
- Fix mechanical compiler errors.
- Keep terminal parser mostly unchanged.

Expected time: 3-7 days.

### Milestone 2: Add OpenSSH backend

Goal: connect to `bbs@ptt.cc -p 22` through system OpenSSH.

Tasks:

- Add `SCD_OpenSSH`.
- Integrate `forkpty`, non-blocking fd, read/write/close.
- Hook backend into `SCD_Socket`.
- Add pty resize handling.
- Disable old SSH menu/SFTP paths for this target.

Expected time: 4-8 days.

### Milestone 3: Make PTT display usable

Goal: PTT login screen and board UI render correctly enough for manual use.

Tasks:

- Add `BbsCodec`.
- Decode Big5/UAO for drawing.
- Preserve raw bytes for terminal state.
- Fix full-width cell drawing.
- Fix basic keyboard input encoding.

Expected time: 1-2 weeks.

### Milestone 4: Polish BBS behavior

Goal: daily use quality.

Tasks:

- Selection should not split full-width characters.
- Clipboard copy/paste should be Unicode-safe outside BBMan and Big5-safe when
  sending to BBS.
- Paste wrapping should operate on display columns, not UTF-8 byte length.
- Reconnect, idle prevention, and hot-call behavior should be retested.
- Font selection should default to a CJK monospace font.

Expected time: 1-2 additional weeks.

## Decisions

Recommended decisions:

- Do not port, restore, or update vendored libssh 0.1.
- Do not keep telnet as the main PTT path. PTT has disabled unencrypted telnet.
- Use OpenSSH through `forkpty` as the primary SSH path.
- Keep terminal wire data byte-oriented.
- Make Unicode a rendering/input/clipboard boundary, not the internal wire
  representation.
- Reuse PCManX UAO mapping if plain iconv Big5 is insufficient.

## Rough Estimate

Prototype usable on PTT:

- 2-3 weeks.

Daily-driver quality:

- 4-6 weeks.

Using a modern SSH library instead of OpenSSH/forkpty would add risk and is not
recommended for the first working port.


## 2026-06-15 porting progress

Current wx3/GTK3 build target:

```sh
cd unix
make -f Makefile.wx3
```

Output binary:

```text
unix/obj/bbman-wx3
```

Implemented so far:

- Added `unix/Makefile.wx3` using system `wx-config` (`gtk3-unicode-3.2` on this host).
- Fixed the first wx3 compile blockers in terminal, bookmark, tab, telnet frame, and editor code.
- Kept old bundled libssh out of the wx3 target.
- Later removed the vendored libssh/SFTP source from the maintained tree.
- Added `src/scd_pty_ssh.{h,cpp}`: a new SSH transport that uses `forkpty()` and execs system `ssh`.
- Rewired `SCD_Socket` so `SOCK_SSH` in the wx3 build uses `SCD_PtySSH` instead of rejecting SSH as a no-SSH build.

Important runtime notes:

- This has passed compile/link only; GUI runtime and an actual PTT login still need manual testing.
- The new SSH path delegates crypto and host-key policy to OpenSSH. Current exec args include `-tt`, `-e none`, `StrictHostKeyChecking=accept-new`, `ServerAliveInterval=60`, and `-p <port>`.
- Password prompting is now expected to happen inside the terminal through OpenSSH/PTT, not through the old libssh password callback.
- Big5/UAO byte-to-Unicode rendering remains the next major correctness task. Current compile fixes do not solve the terminal codec boundary yet.

