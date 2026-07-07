# Project: BBMan
# Makefile created by Dev-C++ 4.9.9.0

PROG = bbman
SRCDIR = /root/BBMan-src
BIN  = obj/$(PROG)

INCS = -I../src/ -I/usr/local/include/
LIBS = `wxgtk2-2.4-config --libs` -lutil

CPP  = g++
CC = gcc $(INCS)
WXCPP = $(CPP) `wxgtk2-2.4-config --cxxflags` $(INCS)
CFLAGS = -fno-rtti -fno-exceptions
CXXFLAGS = $(CXXINCS) -fno-rtti -fno-exceptions

# RES  = BBMan_private.res
# WINDRES = windres.exe

COMMON_OBJ  = obj/scd_terminal.o obj/login.o obj/common.o obj/ds.o obj/editterm_win.o obj/frm_editterm.o obj/frm_telnet.o obj/main.o obj/scd_editterm.o obj/scd_gtk_textctrl.o obj/scd_tabctrl.o obj/scd_telnet.o obj/bookmark.o obj/telnet_panel.o obj/scd_socket.o obj/scd_pty_ssh.o

OBJ = $(COMMON_OBJ)

LINKOBJ  = $(OBJ)

.PHONY: all all-before all-after clean clean-custom

all: all-before obj/$(PROG) all-after

clean: clean-custom
	rm -f $(OBJ) $(BIN)

$(BIN): $(OBJ)
	$(CPP) $(LINKOBJ) -o obj/$(PROG) $(LIBS)

obj/scd_terminal.o: ../src/scd_terminal.cpp
	$(WXCPP) -c ../src/scd_terminal.cpp -o obj/scd_terminal.o $(CXXFLAGS)

obj/login.o: ../src/login.cpp
	$(WXCPP) -c ../src/login.cpp -o obj/login.o $(CXXFLAGS)

obj/common.o: ../src/common.cpp
	$(WXCPP) -c ../src/common.cpp -o obj/common.o $(CXXFLAGS)

obj/ds.o: ../src/ds.cpp
	$(WXCPP) -c ../src/ds.cpp -o obj/ds.o $(CXXFLAGS)

obj/editterm_win.o: ../src/editterm_win.cpp
	$(WXCPP) -c ../src/editterm_win.cpp -o obj/editterm_win.o $(CXXFLAGS)

obj/frm_editterm.o: ../src/frm_editterm.cpp
	$(WXCPP) -c ../src/frm_editterm.cpp -o obj/frm_editterm.o $(CXXFLAGS)

obj/frm_telnet.o: ../src/frm_telnet.cpp
	$(WXCPP) -c ../src/frm_telnet.cpp -o obj/frm_telnet.o $(CXXFLAGS)

obj/main.o: ../src/main.cpp
	$(WXCPP) -c ../src/main.cpp -o obj/main.o $(CXXFLAGS)

obj/scd_editterm.o: ../src/scd_editterm.cpp
	$(WXCPP) -c ../src/scd_editterm.cpp -o obj/scd_editterm.o $(CXXFLAGS)

obj/scd_gtk_textctrl.o: ../src/scd_gtk_textctrl.cpp
	$(WXCPP) -c ../src/scd_gtk_textctrl.cpp -o obj/scd_gtk_textctrl.o $(CXXFLAGS)

obj/scd_tabctrl.o: ../src/scd_tabctrl.cpp
	$(WXCPP) -c ../src/scd_tabctrl.cpp -o obj/scd_tabctrl.o $(CXXFLAGS)

obj/scd_telnet.o: ../src/scd_telnet.cpp
	$(WXCPP) -c ../src/scd_telnet.cpp -o obj/scd_telnet.o $(CXXFLAGS)

obj/bookmark.o: ../src/bookmark.cpp
	$(WXCPP) -c ../src/bookmark.cpp -o obj/bookmark.o $(CXXFLAGS)

obj/telnet_panel.o: ../src/telnet_panel.cpp
	$(WXCPP) -c ../src/telnet_panel.cpp -o obj/telnet_panel.o $(CXXFLAGS)

obj/scd_socket.o: ../src/scd_socket.cpp
	$(WXCPP) -c ../src/scd_socket.cpp -o obj/scd_socket.o $(CXXFLAGS)

obj/scd_pty_ssh.o: ../src/scd_pty_ssh.cpp
	$(WXCPP) -c ../src/scd_pty_ssh.cpp -o obj/scd_pty_ssh.o $(CXXFLAGS)

#BBMan_private.res: BBMan_private.rc 
#	$(WINDRES) -i BBMan_private.rc -I rc -o BBMan_private.res -O coff  --include-dir D:/Dev-Cpp/include
